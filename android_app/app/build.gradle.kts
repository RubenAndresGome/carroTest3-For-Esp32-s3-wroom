import org.gradle.api.tasks.Sync

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("com.chaquo.python")
}

val generatedPythonDir = layout.buildDirectory.dir("generated/python/main").get().asFile

val syncPythonBackend by tasks.registering(Sync::class) {
    group = "robot s3"
    description = "Copia el backend y HMI canónicos de desktop_app al árbol generado Android."
    from(rootProject.file("../desktop_app/robot_app")) {
        into("robot_app")
        exclude("**/__pycache__/**", "static/vite-dist/**")
    }
    from(rootProject.file("../desktop_app/migrations")) {
        into("migrations")
        exclude("**/__pycache__/**")
    }
    into(generatedPythonDir)
}

android {
    namespace = "mx.ik.robots3"
    compileSdk = 36

    defaultConfig {
        applicationId = "mx.ik.robots3"
        minSdk = 24
        targetSdk = 36
        versionCode = 9
        versionName = "0.2.8"

        ndk {
            // La Galaxy Tab moderna es arm64. Un solo ABI reduce mucho el APK.
            abiFilters += "arm64-v8a"
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }

    buildFeatures {
        buildConfig = true
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

    packaging {
        resources.excludes += setOf("META-INF/LICENSE*", "META-INF/NOTICE*")
    }
}

chaquopy {
    defaultConfig {
        version = "3.13"
        pip {
            install("blinker==1.9.0")
            install("click==8.3.1")
            install("Flask==3.1.3")
            install("itsdangerous==2.2.0")
            install("Jinja2==3.1.6")
            install("MarkupSafe==3.0.3")
            install("platformdirs==4.10.0")
            install("waitress==3.0.2")
            install("websocket-client==1.9.0")
            install("Werkzeug==3.1.3")
        }
        // Flask accede al HMI y a las migraciones mediante pathlib.
        extractPackages("robot_app")
    }
    sourceSets {
        getByName("main") {
            srcDir(generatedPythonDir)
        }
    }
}

tasks.named("preBuild").configure {
    dependsOn(syncPythonBackend)
}

tasks.matching { it.name.startsWith("merge") && it.name.endsWith("PythonSources") }
    .configureEach {
        dependsOn(syncPythonBackend)
    }
