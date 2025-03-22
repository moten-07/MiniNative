# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}

# Uncomment this to preserve the line number information for
# debugging stack traces.
#-keepattributes SourceFile,LineNumberTable

# If you keep the line number information, uncomment this to
# hide the original source file name.
#-renamesourcefileattribute SourceFile


# 混淆部分主要保留原代码的有（不混淆部分）：

# 避免混淆泛型
-keepattributes Signature
# 保留内部类
-keepattributes InnerClasses
# 保留行号
-keepattributes SourceFile, LineNumberTable

-printconfiguration
-keepattributes *Annotation*
# 序列化的类不混淆
-keep class * implements android.os.Parcelable
-keep,allowobfuscation @interface androidx.annotation.Keep
# 使用keep注解的类及方法不进行混淆
-keep @androidx.annotation.Keep class **{
    @androidx.annotation.Keep <fields>;
    @androidx.annotation.Keep <methods>;
}
