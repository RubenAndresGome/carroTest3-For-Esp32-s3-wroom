package mx.ik.robots3

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.net.wifi.WifiManager
import android.os.Build
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.os.PowerManager
import android.util.Log
import com.chaquo.python.Python
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class RobotBackendService : Service() {
    private var executor: ExecutorService? = null
    private var wakeLock: PowerManager.WakeLock? = null
    private var wifiLock: WifiManager.WifiLock? = null
    private val lockHandler = Handler(Looper.getMainLooper())
    private val renewWakeLock = object : Runnable {
        override fun run() {
            wakeLock?.let { lock ->
                if (!lock.isHeld) lock.acquire(WAKE_LOCK_TIMEOUT_MS)
                lockHandler.postDelayed(this, WAKE_LOCK_RENEW_MS)
            }
        }
    }

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
        startForeground(NOTIFICATION_ID, buildNotification())
        acquireLocks()
        executor = Executors.newSingleThreadExecutor().also { worker ->
            worker.execute {
                try {
                    Python.getInstance()
                        .getModule("mobile_entry")
                        .callAttr("run", filesDir.absolutePath, BACKEND_PORT)
                } catch (error: Throwable) {
                    Log.e(TAG, "El backend Python terminó inesperadamente", error)
                    updateNotification("Error del backend; revise Logcat")
                }
            }
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_STOP) {
            stopSelf()
            return START_NOT_STICKY
        }
        return START_STICKY
    }

    override fun onDestroy() {
        try {
            if (Python.isStarted()) {
                Python.getInstance().getModule("mobile_entry").callAttr("stop")
            }
        } catch (error: Throwable) {
            Log.w(TAG, "No fue posible solicitar el cierre de Waitress", error)
        }
        executor?.shutdownNow()
        lockHandler.removeCallbacks(renewWakeLock)
        wifiLock?.takeIf { it.isHeld }?.release()
        wakeLock?.takeIf { it.isHeld }?.release()
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun acquireLocks() {
        val powerManager = getSystemService(Context.POWER_SERVICE) as PowerManager
        wakeLock = powerManager.newWakeLock(
            PowerManager.PARTIAL_WAKE_LOCK,
            "$packageName:robot-backend",
        ).apply { acquire(WAKE_LOCK_TIMEOUT_MS) }
        lockHandler.postDelayed(renewWakeLock, WAKE_LOCK_RENEW_MS)

        val wifiManager = applicationContext.getSystemService(Context.WIFI_SERVICE) as WifiManager
        @Suppress("DEPRECATION")
        wifiLock = wifiManager.createWifiLock(
            WifiManager.WIFI_MODE_FULL_HIGH_PERF,
            "$packageName:robot-wifi",
        ).apply { acquire() }
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return
        val channel = NotificationChannel(
            CHANNEL_ID,
            getString(R.string.backend_channel_name),
            NotificationManager.IMPORTANCE_LOW,
        )
        channel.description = getString(R.string.backend_notification_text)
        getSystemService(NotificationManager::class.java).createNotificationChannel(channel)
    }

    private fun buildNotification(text: String = getString(R.string.backend_notification_text)): Notification {
        val openIntent = PendingIntent.getActivity(
            this,
            0,
            Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
        )
        val stopIntent = PendingIntent.getService(
            this,
            1,
            Intent(this, RobotBackendService::class.java).setAction(ACTION_STOP),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
        )
        val builder = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            Notification.Builder(this, CHANNEL_ID)
        } else {
            @Suppress("DEPRECATION")
            Notification.Builder(this)
        }
        return builder
            .setSmallIcon(R.drawable.ic_robot_notification)
            .setContentTitle(getString(R.string.backend_notification_title))
            .setContentText(text)
            .setContentIntent(openIntent)
            .setOngoing(true)
            .addAction(
                Notification.Action.Builder(
                    null,
                    getString(R.string.backend_notification_stop),
                    stopIntent,
                ).build(),
            )
            .build()
    }

    private fun updateNotification(text: String) {
        getSystemService(NotificationManager::class.java)
            .notify(NOTIFICATION_ID, buildNotification(text))
    }

    companion object {
        private const val TAG = "RobotBackendService"
        private const val CHANNEL_ID = "robot_s3_backend"
        private const val NOTIFICATION_ID = 3201
        private const val ACTION_STOP = "mx.ik.robots3.STOP_BACKEND"
        private const val WAKE_LOCK_TIMEOUT_MS = 10 * 60 * 1000L
        private const val WAKE_LOCK_RENEW_MS = 9 * 60 * 1000L
        const val BACKEND_PORT = 8080
    }
}
