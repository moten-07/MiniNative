#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/bitmap.h>
#include <opencv2/opencv.hpp>

jstring stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


jobject getJNIHeight(
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
    cv::Mat src(srcInfo.height, srcInfo.width, CV_8UC4, pixels);

    cv::cvtColor(src, src, cv::ColorConversionCodes::COLOR_BGRA2GRAY);

    AndroidBitmap_unlockPixels(env, bitmap);
    return bitmap;
}


//注册函数映射
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *pEnv = NULL;
    //获取环境
    jint ret = vm->GetEnv((void **) &pEnv, JNI_VERSION_1_6);
    if (ret != JNI_OK) {
        __android_log_write(ANDROID_LOG_ERROR, "jni_replace", "JVM ERROR:GetEnv");
        return -1;
    }
    //在{}里面进行方法映射编写，第一个是java端方法名，第二个是方法签名，第三个是c语言形式签名（括号内表示方法返回值）
    JNINativeMethod g_Methods[] = {
            {"stringFromJNI", "()Ljava/lang/String;",                                 (jstring *) stringFromJNI},
            {"getJNIHeight",  "(Landroid/graphics/Bitmap;)Landroid/graphics/Bitmap;", (jobject *) getJNIHeight}
    };
    jclass cls = pEnv->FindClass("com/example/myapplication/MainActivity");
    if (cls == NULL) {
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

