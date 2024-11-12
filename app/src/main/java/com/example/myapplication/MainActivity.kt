package com.example.myapplication

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.os.Bundle
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import com.example.myapplication.databinding.ActivityMainBinding
import com.example.nativelib.NativeLib

class MainActivity : AppCompatActivity() {


    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        enableEdgeToEdge()
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Example of a call to a native method
        val nativeLib = NativeLib()
        binding.sampleText.text = nativeLib.stringFromJNI()

        val inputStream = assets.open("08182119_02.jpg")
        val bitmap = BitmapFactory.decodeStream(inputStream)

        val cvtBitmap = nativeLib.cvtColor(bitmap.copy(Bitmap.Config.ARGB_8888, true))
        binding.image.setImageBitmap(cvtBitmap)
    }
}