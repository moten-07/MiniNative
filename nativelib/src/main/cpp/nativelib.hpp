//
// Created by demo on 2024/11/12.
//

#ifndef MY_APPLICATION_NATIVELIB_HPP
#define MY_APPLICATION_NATIVELIB_HPP

#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/bitmap.h>
#include <opencv2/opencv.hpp>

jstring stringFromJNI(
        JNIEnv *env,
        jobject /* this */);


jobject cvtColor(
        JNIEnv *env,
        jobject /* this */,
        jobject bitmap);

void copyPixel4Mat(void* _Nonnull const pixels,const cv::Mat &mat);

#endif //MY_APPLICATION_NATIVELIB_HPP
