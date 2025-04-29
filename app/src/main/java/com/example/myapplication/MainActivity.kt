package com.example.myapplication

import android.app.Activity
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.os.Bundle
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.PickVisualMediaRequest
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.graphics.scale
import androidx.lifecycle.lifecycleScope
import com.example.myapplication.databinding.ActivityMainBinding
import com.example.nativelib.NativeLib
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.launch

class MainActivity : AppCompatActivity() {

    private val binding by lazy { ActivityMainBinding.inflate(layoutInflater) }
    private val nativeLib get() = NativeLib()

    private val bitmapFlow = MutableSharedFlow<Bitmap>(replay = 1)
    private val detectResult = MutableSharedFlow<String>()

    private val register =
        registerForActivityResult(ActivityResultContracts.PickVisualMedia()) {
            if (it == null) {
                return@registerForActivityResult
            }
            val inputStream = contentResolver.openInputStream(it)
            val bitmap = BitmapFactory.decodeStream(inputStream)
            val copy = bitmap.copy(Bitmap.Config.ARGB_8888, true)
            val cvtColor = copy.scale(227, 227)

            val cpuDetect = nativeLib.ncnnDetect(cvtColor, false)
            val gpuDetect = nativeLib.ncnnDetect(cvtColor, true)
            val message = """
                CPU:$cpuDetect
                GPU: $gpuDetect
            """.trimIndent()
            lifecycleScope.launch {
                bitmapFlow.emit(bitmap)
                detectResult.emit(message)
            }
        }

    private val register30 =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
            if (it.resultCode != Activity.RESULT_OK) {
                return@registerForActivityResult
            }
            val uri = it.data?.data ?: return@registerForActivityResult
            val inputStream = contentResolver.openInputStream(uri)
            val bitmap = BitmapFactory.decodeStream(inputStream)
            val copy = bitmap.copy(Bitmap.Config.ARGB_8888, true)
            val cvtColor = copy.scale(227, 227)

            val color = nativeLib.cvtColor(bitmap)

            val cpuDetect = nativeLib.ncnnDetect(cvtColor, false)
            val gpuDetect = nativeLib.ncnnDetect(cvtColor, true)
            val message = """
                CPU:$cpuDetect
                GPU: $gpuDetect
            """.trimIndent()
            lifecycleScope.launch {
                bitmapFlow.emit(color)
                detectResult.emit(message)
            }
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        enableEdgeToEdge()
        setContentView(binding.root)

        nativeLib.ncnnInit(assets)

        binding.btnSelect.setOnClickListener {
            val intent = Intent(Intent.ACTION_PICK)
                .setType("image/*")
            register30.launch(intent)
        }
        binding.btnSelectSystem.setOnClickListener {
            register.launch(PickVisualMediaRequest(ActivityResultContracts.PickVisualMedia.ImageOnly))
        }

        lifecycleScope.launch {
            bitmapFlow.collect {
                binding.image.setImageBitmap(it)
            }
        }

        lifecycleScope.launch {
            detectResult.collect {
                binding.sampleText.text = it
            }
        }
    }
}
