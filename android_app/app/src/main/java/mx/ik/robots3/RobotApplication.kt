package mx.ik.robots3

import android.app.Application
import com.chaquo.python.Python
import com.chaquo.python.android.AndroidPlatform

class RobotApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        if (!Python.isStarted()) {
            Python.start(AndroidPlatform(this))
        }
    }
}
