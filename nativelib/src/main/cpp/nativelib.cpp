#include "nativelib.hpp"

jstring stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


void copyPixel4Mat(void *_Nonnull const pixels, const cv::Mat &mat) {
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
        throw std::runtime_error("Unsupported source image type for conversion to CV_8UC4.");
    }
    memcpy(pixels, dst.data, dst.total() * dst.elemSize());
}


jobject cvtColor(
        JNIEnv *env,
        jobject /* this */,
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


//注册函数映射
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
            {"cvtColor",      "(Landroid/graphics/Bitmap;)Landroid/graphics/Bitmap;", (jobject *) cvtColor}
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
    //返回java版本
    return JNI_VERSION_1_6;
}


