package com.example.myapplication

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Typeface
import android.os.Bundle
import android.os.Environment
import android.util.Log
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import com.example.myapplication.databinding.ActivityMainBinding
import java.io.File
import java.io.FileOutputStream

class MainActivity : AppCompatActivity() {


    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        enableEdgeToEdge()
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Example of a call to a native method
        binding.sampleText.text = stringFromJNI()

        binding.sampleText.setOnClickListener {
            test()
        }
//        val bitmap = BitmapFactory.decodeStream(assets.open("08182119_02.jpg"))
//        var copy = bitmap.copy(bitmap.config, true)
//        copy = getJNIHeight(copy)
//        binding.image.setImageBitmap(copy)
    }

    private fun test() {
        val bitmap = Bitmap.createBitmap(104, 32, Bitmap.Config.ARGB_8888)
        bitmap.eraseColor(Color.WHITE)
        val canvas = Canvas()
        canvas.setBitmap(bitmap)
        val paint = Paint()
        paint.color = Color.BLACK
        paint.textSize = 22F
        paint.textAlign = Paint.Align.CENTER
        paint.typeface = Typeface.create(Typeface.DEFAULT, Typeface.BOLD)
        val offset = (paint.ascent() + paint.descent()) / 2F
        canvas.drawText("ABCDEFG", bitmap.width.toFloat(), bitmap.height - offset, paint)


        val bytes = showBmp2model(bitmap)
        val hex = bytes.joinToString(prefix = "[", postfix = "]") {
            "0x${"%02X".format(it)}"
        }

        val file = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)
        val textFile = File(file, "test.txt")
        val bitmapFile = File(file, "bitmap.png")
        bitmap.compress(Bitmap.CompressFormat.PNG, 100, FileOutputStream(bitmapFile))
        textFile.writeText(hex)
        Log.d(TAG, "useAppContext: ${textFile.path}")
    }

    /**
     * A native method that is implemented by the 'myapplication' native library,
     * which is packaged with this application.
     */
    private external fun stringFromJNI(): String

    private external fun getJNIHeight(bitmap: Bitmap): Bitmap

    companion object {
        private const val TAG = "MainActivity"

        // Used to load the 'myapplication' library on application startup.
        init {
            System.loadLibrary("myapplication")
        }
    }

    /**
     * 根据图片取模,用于显示在屏幕上
     *
     * @param bitmap 显示图
     * @return 取模结果
     */
    private fun showBmp2model(bitmap: Bitmap): ByteArray {
        // 8个像素取一次点
        val row = bitmap.height / 8
        // 预留4位作为尺寸
        val model = ByteArray(bitmap.width * row + 4)

        // 单位点低位
        model[0] = (row % 256).toByte()
        // 单位点高位
        model[1] = (row / 256).toByte()
        // 行数低位
        model[2] = (bitmap.width % 256).toByte()
        // 行数高位
        model[3] = (bitmap.width / 256).toByte()

        // 起始位
        var startIndex = 3
        // 和打印的有区别,先从右到左再从上到下遍历点
        for (i in 0 until row) {
            for (x in bitmap.width - 1 downTo 0) {
                var byteValue: Byte = 0
                for (y in (i + 1) * 8 - 1 downTo 8 * i) {
                    val pixel = bitmap.getPixel(x, y)
                    val r = Color.red(pixel)
                    val g = Color.green(pixel)
                    val b = Color.blue(pixel)
                    // 计算灰度
                    val gray = r * 0.114f + g * 0.587f + b * 0.299f
                    // 上一个点进一位
                    byteValue = (byteValue.toInt() shl 1).toByte()
                    // 黑点为1,白点为0
                    byteValue = (byteValue.toInt() or (if (gray > 0x80) 0x00 else 0x01).toByte()
                        .toInt()).toByte()
                }
                // 将二进制值存储到数据中
                model[++startIndex] = byteValue
            }
        }
        return model
    }

}