#include "nativelib.hpp"

void copyPixel4Mat(
        void *_Nonnull const pixels,
        const cv::Mat &mat) {
    cv::Mat dst;
    int type = mat.type();
    if (type == CV_8UC1) {
        // 对于单通道图像，使用cvtColor进行转换
        cv::cvtColor(mat, dst, cv::COLOR_GRAY2BGRA);
    } else if (type == CV_8UC3) {
        // 对于三通道图像，手动创建一个Alpha通道并合并
        std::vector<cv::Mat> bgrChannels(3);
        // 分割BGR通道
        cv::split(mat, bgrChannels);
        // 添加Alpha通道
        bgrChannels.push_back(cv::Mat::ones(mat.size(), CV_8U) * 255);
        // 合并通道
        cv::merge(bgrChannels, dst);
    } else if (type == CV_8UC4) {
        dst = mat.clone();
    } else {
        // 对于其他类型，这里不处理或抛出异常
//        throw std::runtime_error("Unsupported source image type for conversion to CV_8UC4.");
    }
    memcpy(pixels, dst.data, dst.total() * dst.elemSize());
}


static std::vector<std::string> split_string(
        const std::string &str,
        const std::string &delimiter) {
    std::vector<std::string> strings;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos) {
        strings.push_back(str.substr(prev, pos - prev));
        prev = pos + 1;
    }

    // To get the last substring (or only, if delimiter is not found)
    strings.push_back(str.substr(prev));

    return strings;
}


jstring _Nullable stringFromJNI(
        JNIEnv *_Nullable env,
        jobject _Nullable /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


jobject _Nullable cvtColor(
        JNIEnv *_Nullable env,
        jobject _Nullable /* this */,
        jobject bitmap) {
    AndroidBitmapInfo srcInfo;
    if (AndroidBitmap_getInfo(env, bitmap, &srcInfo) != ANDROID_BITMAP_RESULT_SUCCESS) {
        return bitmap;
    }
    void *pixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) != ANDROID_BITMAP_RESULT_SUCCESS) {
        return bitmap;
    }
    cv::Mat src((int) srcInfo.height, (int) srcInfo.width, CV_8UC4, pixels);
    cv::Mat gray = src.clone();

    cv::cvtColor(src, gray, cv::ColorConversionCodes::COLOR_BGRA2GRAY);

    copyPixel4Mat(pixels, gray);
    AndroidBitmap_unlockPixels(env, bitmap);
    return bitmap;
}

jboolean ncnnInit(
        JNIEnv *_Nullable env,
        jobject _Nullable /* this */,
        jobject _Nullable assetManager) {
    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);

    // init param
    {
        int ret = squeezenet.load_param_bin(mgr, "squeezenet_v1.1.param.bin");
        if (ret != 0) {
            __android_log_print(ANDROID_LOG_DEBUG, "SqueezeNcnn", "load_param_bin failed");
            return JNI_FALSE;
        }
    }

    // init bin
    {
        int ret = squeezenet.load_model(mgr, "squeezenet_v1.1.bin");
        if (ret != 0) {
            __android_log_print(ANDROID_LOG_DEBUG, "SqueezeNcnn", "load_model failed");
            return JNI_FALSE;
        }
    }

    // use vulkan compute
    if (ncnn::get_gpu_count() != 0) {
        squeezenet_gpu.opt.use_vulkan_compute = true;

        {
            int ret = squeezenet_gpu.load_param_bin(mgr, "squeezenet_v1.1.param.bin");
            if (ret != 0) {
                __android_log_print(ANDROID_LOG_DEBUG, "SqueezeNcnn", "load_param_bin failed");
                return JNI_FALSE;
            }
        }
        {
            int ret = squeezenet_gpu.load_model(mgr, "squeezenet_v1.1.bin");
            if (ret != 0) {
                __android_log_print(ANDROID_LOG_DEBUG, "SqueezeNcnn", "load_model failed");
                return JNI_FALSE;
            }
        }
    }

    // init words
    {
        AAsset *asset = AAssetManager_open(mgr, "synset_words.txt", AASSET_MODE_BUFFER);
        if (!asset) {
            __android_log_print(ANDROID_LOG_DEBUG, "SqueezeNcnn", "open synset_words.txt failed");
            return JNI_FALSE;
        }

        int len = AAsset_getLength(asset);

        std::string words_buffer;
        words_buffer.resize(len);
        int ret = AAsset_read(asset, (void *) words_buffer.data(), len);

        AAsset_close(asset);

        if (ret != len) {
            __android_log_print(ANDROID_LOG_DEBUG, "SqueezeNcnn", "read synset_words.txt failed");
            return JNI_FALSE;
        }

        squeezenet_words = split_string(words_buffer, "\n");
    }

    return JNI_TRUE;
}

jstring _Nullable ncnnDetect(
        JNIEnv *_Nullable env,
        jobject _Nullable /* this */,
        jobject _Nullable bitmap,
        jboolean use_gpu
) {
    if (bitmap == nullptr) {
        return nullptr;
    }
    if (use_gpu == JNI_TRUE && ncnn::get_gpu_count() == 0) {
        return env->NewStringUTF("no vulkan capable gpu");
    }

    double start_time = ncnn::get_current_time();

    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);
    int width = info.width;
    int height = info.height;
    if (width != 227 || height != 227)
        return nullptr;
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
        return nullptr;

    // ncnn from bitmap
    ncnn::Mat in = ncnn::Mat::from_android_bitmap(env, bitmap, ncnn::Mat::PIXEL_BGR);

    // squeezenet
    std::vector<float> cls_scores;
    {
        const float mean_vals[3] = {104.f, 117.f, 123.f};
        in.substract_mean_normalize(mean_vals, 0);

        ncnn::Extractor ex = use_gpu ? squeezenet_gpu.create_extractor()
                                     : squeezenet.create_extractor();

        ex.input(squeezenet_v1_1_param_id::BLOB_data, in);

        ncnn::Mat out;
        ex.extract(squeezenet_v1_1_param_id::BLOB_prob, out);

        cls_scores.resize(out.w);
        for (int j = 0; j < out.w; j++) {
            cls_scores[j] = out[j];
        }
    }

    // return top class
    int top_class = 0;
    float max_score = 0.f;
    for (size_t i = 0; i < cls_scores.size(); i++) {
        float s = cls_scores[i];
//         __android_log_print(ANDROID_LOG_DEBUG, "SqueezeNcnn", "%d %f", i, s);
        if (s > max_score) {
            top_class = i;
            max_score = s;
        }
    }

    const std::string &word = squeezenet_words[top_class];
    char tmp[32];
    sprintf(tmp, "%.3f", max_score);
    std::string result_str = std::string(word.c_str() + 10) + " = " + tmp;

    // +10 to skip leading n03179701
    jstring result = env->NewStringUTF(result_str.c_str());

    double elasped = ncnn::get_current_time() - start_time;
    __android_log_print(ANDROID_LOG_DEBUG, "SqueezeNcnn", "%.2fms   detect", elasped);

    return result;
}


/**
 * 注册函数映射
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *pEnv = nullptr;
    //获取环境
    jint ret = vm->GetEnv((void **) &pEnv, JNI_VERSION_1_6);
    if (ret != JNI_OK) {
        __android_log_write(ANDROID_LOG_ERROR, "jni_replace", "JVM ERROR:GetEnv");
        return -1;
    }
    //在{}里面进行方法映射编写，第一个是java端方法名，第二个是方法签名，第三个是c语言形式签名（括号内表示方法返回值）
    JNINativeMethod g_Methods[] = {
            {"stringFromJNI", "()Ljava/lang/String;",                                 (jstring *) stringFromJNI},
            {"cvtColor",      "(Landroid/graphics/Bitmap;)Landroid/graphics/Bitmap;", (jobject *) cvtColor},
            {"ncnnInit",      "(Landroid/content/res/AssetManager;)Z",                (jboolean *) ncnnInit},
            {"ncnnDetect",    "(Landroid/graphics/Bitmap;Z)Ljava/lang/String;",       (jstring *) ncnnDetect}
    };
    jclass cls = pEnv->FindClass("com/example/nativelib/NativeLib");
    if (cls == nullptr) {
        __android_log_write(ANDROID_LOG_ERROR, "jni_replace", "FindClass Error");
        return -1;
    }
    //动态注册本地方法
    ret = pEnv->RegisterNatives(cls, g_Methods, sizeof(g_Methods) / sizeof(g_Methods[0]));
    if (ret != JNI_OK) {
        __android_log_write(ANDROID_LOG_ERROR, "jni_replace", "Register Error");
        return -1;
    }
    ncnn::create_gpu_instance();
    //返回java版本
    return JNI_VERSION_1_6;
}


JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    ncnn::destroy_gpu_instance();
}

