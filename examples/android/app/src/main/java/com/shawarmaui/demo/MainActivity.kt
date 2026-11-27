package com.shawarmaui.demo

import android.app.Activity
import android.content.Context
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.inputmethod.BaseInputConnection
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputConnection
import android.view.inputmethod.InputMethodManager
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MainActivity : Activity() {
    companion object {
        init { System.loadLibrary("shawarmaui_demo") }
    }

    private external fun nativeInit(density: Float)
    private external fun nativeResize(w: Int, h: Int, topInset: Int, bottomInset: Int)
    private external fun nativeRender(): Int
    private external fun nativeTouch(action: Int, x: Int, y: Int)
    private external fun nativeTextCommit(text: String)
    private external fun nativeKeySpecial(code: Int)

    private lateinit var glView: ShawarmaGLView
    private var topInset = 0
    private var bottomInset = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        glView = ShawarmaGLView(this)
        setContentView(glView)

        window.decorView.setOnApplyWindowInsetsListener { v, insets ->
            topInset = insets.systemWindowInsetTop
            bottomInset = insets.systemWindowInsetBottom
            if (glView.width > 0 && glView.height > 0) {
                glView.queueEvent { nativeResize(glView.width, glView.height, topInset, bottomInset) }
            }
            v.onApplyWindowInsets(insets)
        }
    }

    override fun onPause() {
        super.onPause()
        glView.mainHandler.removeCallbacks(glView.renderRunnable) // <-- AQUI
        glView.onPause()
    }

    override fun onResume() {
        super.onResume()
        glView.onResume()
    }

    fun requestRenderFromNative() {
        glView.mainHandler.removeCallbacks(glView.renderRunnable)
        glView.requestRender()
    }

    fun setKeyboardVisible(show: Boolean) {
        runOnUiThread {
            val imm = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
            if (show) {
                glView.requestFocus()
                imm.showSoftInput(glView, InputMethodManager.SHOW_IMPLICIT)
            } else {
                imm.hideSoftInputFromWindow(glView.windowToken, 0)
            }
        }
    }

    inner class ShawarmaGLView(ctx: Context) : GLSurfaceView(ctx), GLSurfaceView.Renderer {
        init {
            setEGLContextClientVersion(3)
            setEGLConfigChooser(8, 8, 8, 8, 16, 8)
            setRenderer(this)
            renderMode = RENDERMODE_WHEN_DIRTY
            isFocusable = true
            isFocusableInTouchMode = true
        }

        override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
            nativeInit(resources.displayMetrics.density)
        }

        override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
            nativeResize(width, height, topInset, bottomInset)
        }

        val mainHandler = Handler(Looper.getMainLooper())
        val renderRunnable = Runnable { requestRender() }

        override fun onDrawFrame(gl: GL10?) {
            mainHandler.removeCallbacks(renderRunnable)
            val delayMs = nativeRender()
            if (delayMs > 0) {
                mainHandler.postDelayed(renderRunnable, delayMs.toLong())
            }
        }

        override fun onTouchEvent(event: MotionEvent): Boolean {
            val act = when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> 0
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> 1
                MotionEvent.ACTION_MOVE -> 2
                else -> return true
            }
            val x = event.x.toInt()
            val y = event.y.toInt()
            queueEvent { nativeTouch(act, x, y) }
            return true
        }

        override fun onCheckIsTextEditor(): Boolean = true

        override fun onCreateInputConnection(outAttrs: EditorInfo): InputConnection {
            outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_EXTRACT_UI
            return object : BaseInputConnection(this, false) {
                override fun commitText(text: CharSequence?, newCursorPosition: Int): Boolean {
                    val str = text?.toString() ?: return true
                    queueEvent { nativeTextCommit(str) }
                    return true
                }

                override fun deleteSurroundingText(beforeLength: Int, afterLength: Int): Boolean {
                    if (beforeLength > 0) queueEvent { nativeKeySpecial(1) }
                    return true
                }

                override fun sendKeyEvent(event: KeyEvent): Boolean {
                    if (event.action == KeyEvent.ACTION_DOWN) {
                        if (event.keyCode == KeyEvent.KEYCODE_DEL) queueEvent { nativeKeySpecial(1) }
                        else if (event.keyCode == KeyEvent.KEYCODE_ENTER) queueEvent { nativeKeySpecial(2) }
                    }
                    return super.sendKeyEvent(event)
                }
            }
        }
    }
}
