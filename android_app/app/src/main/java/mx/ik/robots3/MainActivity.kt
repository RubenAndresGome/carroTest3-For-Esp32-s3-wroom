package mx.ik.robots3

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.view.WindowManager
import android.webkit.CookieManager
import android.webkit.JsResult
import android.webkit.JavascriptInterface
import android.webkit.WebChromeClient
import android.webkit.WebResourceRequest
import android.webkit.WebSettings
import android.webkit.WebView
import android.webkit.WebViewClient
import android.app.AlertDialog
import android.widget.FrameLayout
import android.widget.ProgressBar
import android.widget.TextView
import android.widget.Toast
import android.window.OnBackInvokedCallback
import android.window.OnBackInvokedDispatcher
import java.io.File
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class MainActivity : Activity() {
    private lateinit var webView: WebView
    private lateinit var loadingView: FrameLayout
    private val worker: ExecutorService = Executors.newSingleThreadExecutor()
    private val mainHandler = Handler(Looper.getMainLooper())
    private var destroyed = false
    private var backCallback: OnBackInvokedCallback? = null

    private inner class RobotHostBridge {
        @JavascriptInterface
        fun closeApp() {
            runOnUiThread {
                stopService(Intent(this@MainActivity, RobotBackendService::class.java))
                finishAndRemoveTask()
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        requestNotificationPermission()
        registerBackNavigation()
        startRobotBackend()
        buildLoadingView()
        waitForBackend()
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            @Suppress("DEPRECATION")
            window.decorView.systemUiVisibility = (
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_FULLSCREEN
            )
        }
    }

    override fun onDestroy() {
        destroyed = true
        mainHandler.removeCallbacksAndMessages(null)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            backCallback?.let(onBackInvokedDispatcher::unregisterOnBackInvokedCallback)
        }
        worker.shutdownNow()
        if (::webView.isInitialized) webView.destroy()
        super.onDestroy()
    }

    @SuppressLint("GestureBackNavigation")
    @Deprecated("Fallback requerido únicamente antes de Android 13")
    override fun onBackPressed() {
        handleBackNavigation()
    }

    private fun registerBackNavigation() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) return
        backCallback = OnBackInvokedCallback { handleBackNavigation() }.also { callback ->
            onBackInvokedDispatcher.registerOnBackInvokedCallback(
                OnBackInvokedDispatcher.PRIORITY_DEFAULT,
                callback,
            )
        }
    }

    private fun handleBackNavigation() {
        if (::webView.isInitialized && webView.canGoBack()) webView.goBack()
        else moveTaskToBack(true)
    }

    private fun requestNotificationPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
            checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED
        ) {
            requestPermissions(arrayOf(Manifest.permission.POST_NOTIFICATIONS), 100)
        }
    }

    private fun startRobotBackend() {
        val intent = Intent(this, RobotBackendService::class.java)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) startForegroundService(intent)
        else startService(intent)
    }

    private fun buildLoadingView() {
        loadingView = FrameLayout(this).apply { setBackgroundColor(Color.rgb(7, 16, 28)) }
        applySystemInsets(loadingView)
        val progress = ProgressBar(this)
        val progressParams = FrameLayout.LayoutParams(80, 80, Gravity.CENTER).apply {
            bottomMargin = 60
        }
        loadingView.addView(progress, progressParams)
        val label = TextView(this).apply {
            text = getString(R.string.loading_backend)
            setTextColor(Color.WHITE)
            textSize = 18f
            gravity = Gravity.CENTER
        }
        val labelParams = FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
            Gravity.CENTER,
        ).apply {
            topMargin = 100
            marginStart = 32
            marginEnd = 32
        }
        loadingView.addView(label, labelParams)
        setContentView(loadingView)
    }

    private fun waitForBackend(attempt: Int = 0) {
        worker.execute {
            val ready = try {
                (URL(BACKEND_URL).openConnection() as HttpURLConnection).run {
                    connectTimeout = 350
                    readTimeout = 350
                    requestMethod = "GET"
                    connect()
                    val ok = responseCode in 200..399
                    disconnect()
                    ok
                }
            } catch (_: Exception) {
                false
            }
            mainHandler.post {
                if (destroyed) return@post
                if (ready) showHmi()
                else if (attempt < 120) mainHandler.postDelayed({ waitForBackend(attempt + 1) }, 250)
                else showStartupError()
            }
        }
    }

    @SuppressLint("SetJavaScriptEnabled")
    private fun showHmi() {
        if (::webView.isInitialized) return
        webView = WebView(this).apply {
            setBackgroundColor(Color.rgb(7, 16, 28))
            settings.javaScriptEnabled = true
            settings.domStorageEnabled = true
            settings.cacheMode = WebSettings.LOAD_NO_CACHE
            settings.allowFileAccess = false
            settings.allowContentAccess = false
            settings.mixedContentMode = WebSettings.MIXED_CONTENT_NEVER_ALLOW
            settings.setSupportZoom(true)
            settings.builtInZoomControls = true
            settings.displayZoomControls = false
            settings.useWideViewPort = true
            addJavascriptInterface(RobotHostBridge(), "RobotHost")
            webViewClient = object : WebViewClient() {
                override fun shouldOverrideUrlLoading(view: WebView, request: WebResourceRequest): Boolean {
                    return request.url.host !in setOf("127.0.0.1", "localhost")
                }
            }
            webChromeClient = object : WebChromeClient() {
                override fun onJsConfirm(view: WebView?, url: String?, message: String?, result: JsResult?): Boolean {
                    AlertDialog.Builder(this@MainActivity)
                        .setMessage(message)
                        .setPositiveButton(android.R.string.ok) { _, _ -> result?.confirm() }
                        .setNegativeButton(android.R.string.cancel) { _, _ -> result?.cancel() }
                        .setOnCancelListener { result?.cancel() }
                        .show()
                    return true
                }
            }
            setDownloadListener { url, _, contentDisposition, mimeType, _ ->
                downloadExport(url, contentDisposition, mimeType)
            }
        }
        if (BuildConfig.DEBUG) WebView.setWebContentsDebuggingEnabled(true)
        applySystemInsets(webView)
        CookieManager.getInstance().setAcceptCookie(false)
        setContentView(webView)
        webView.loadUrl(BACKEND_URL)
    }

    private fun downloadExport(url: String, contentDisposition: String?, mimeType: String?) {
        worker.execute {
            try {
                val suggested = Regex("filename=\"?([^\";]+)")
                    .find(contentDisposition.orEmpty())?.groupValues?.get(1)
                    ?: if (mimeType == "text/csv") "robot-telemetry.csv" else "robot-session.json"
                val safeName = suggested.replace(Regex("[^A-Za-z0-9._-]"), "_")
                val directory = getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS) ?: filesDir
                val destination = File(directory, safeName)
                URL(url).openStream().use { input -> destination.outputStream().use(input::copyTo) }
                runOnUiThread {
                    Toast.makeText(this, "Exportado en ${destination.absolutePath}", Toast.LENGTH_LONG).show()
                }
            } catch (error: Exception) {
                runOnUiThread {
                    Toast.makeText(this, "No se pudo exportar: ${error.message}", Toast.LENGTH_LONG).show()
                }
            }
        }
    }

    private fun showStartupError() {
        val message = TextView(this).apply {
            setBackgroundColor(Color.rgb(7, 16, 28))
            setTextColor(Color.rgb(255, 120, 120))
            textSize = 18f
            gravity = Gravity.CENTER
            text = getString(R.string.backend_start_error)
            setPadding(40, 40, 40, 40)
        }
        applySystemInsets(message)
        setContentView(message)
    }

    @Suppress("DEPRECATION")
    private fun applySystemInsets(view: View) {
        view.setOnApplyWindowInsetsListener { target, insets ->
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                val safe = insets.getInsets(
                    WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout(),
                )
                target.setPadding(safe.left, safe.top, safe.right, safe.bottom)
            } else {
                target.setPadding(
                    insets.systemWindowInsetLeft,
                    insets.systemWindowInsetTop,
                    insets.systemWindowInsetRight,
                    insets.systemWindowInsetBottom,
                )
            }
            insets
        }
        view.requestApplyInsets()
    }

    companion object {
        private const val BACKEND_URL = "http://127.0.0.1:8080/"
    }
}
