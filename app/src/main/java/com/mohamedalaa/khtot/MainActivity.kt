package com.mohamedalaa.khtot

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.widget.TextView
import com.google.androidgamesdk.GameActivity

class MainActivity : GameActivity() {
    companion object {
        init {
            System.loadLibrary("khtot")
        }
    }

    private lateinit var levelTxt: TextView
    private lateinit var attemptsTxt: TextView
    private val handler = Handler(Looper.getMainLooper())

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        levelTxt = findViewById(R.id.levelText)
        attemptsTxt = findViewById(R.id.attemptsText)
        
        // تحديث الواجهة بشكل دوري (Polling)
        handler.post(object : Runnable {
            override fun run() {
                updateUI()
                handler.postDelayed(this, 100)
            }
        })
    }

    private fun updateUI() {
        // هذه القيم سيتم جلبها عبر JNI لاحقاً إذا أردت دقة أكبر
        // حالياً سنبقيها بسيطة
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            hideSystemUi()
        }
    }

    private fun hideSystemUi() {
        val decorView = window.decorView
        decorView.systemUiVisibility = (View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_FULLSCREEN)
    }
}