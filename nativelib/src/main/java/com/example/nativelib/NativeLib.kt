package com.example.nativelib

import android.graphics.Bitmap
import androidx.annotation.Keep

@Keep
class NativeLib {

    /**
     * A native method that is implemented by the 'nativelib' native library,
     * which is packaged with this application.
     */
    @Keep
    external fun stringFromJNI(): String

    @Keep
    external fun cvtColor(bitmap: Bitmap): Bitmap

    companion object {
        // Used to load the 'nativelib' library on application startup.
        init {
            System.loadLibrary("nativelib")
        }
    }
}
