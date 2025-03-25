package com.example.nativelib

import android.content.res.AssetManager
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

    @Keep
    external fun ncnnInit(assetManager: AssetManager): Boolean

    @Keep
    external fun ncnnDetect(bitmap: Bitmap?, boolean: Boolean): String

    companion object {
        // Used to load the 'nativelib' library on application startup.
        init {
            System.loadLibrary("nativelib")
        }
    }
}
