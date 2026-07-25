#include "Sensores.h"
#include "Config.h"
#include "Estado.h"
#include "Debug.h"
#include "driver/pcnt.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <esp_timer.h>

Adafruit_MPU6050 mpu;
static bool mpu_inicializado = false;
static bool mpu_calibrado = false;
static float anguloZ_acum = 0.0f;
static float gyro_z_offset_rad_s = 0.0f;
static unsigned long tiempoAnteriorIMU = 0;
static unsigned long ultimaLecturaIMU = 0;
static uint32_t contadorRecentradosYaw = 0;
static portMUX_TYPE muxOrientacionIMU = portMUX_INITIALIZER_UNLOCKED;
static SensorSnapshot snapshotControl = {};
static SensorSnapshot snapshotPublicado = {};
static portMUX_TYPE muxSnapshotSensores = portMUX_INITIALIZER_UNLOCKED;

template <size_t N>
class PromedioMovil {
public:
    float agregar(float valor) {
        if (cantidad_ < N) {
            valores_[indice_] = valor;
            suma_ += valor;
            ++cantidad_;
        } else {
            suma_ -= valores_[indice_];
            valores_[indice_] = valor;
            suma_ += valor;
        }
        indice_ = (indice_ + 1) % N;
        return suma_ / static_cast<float>(cantidad_);
    }

    void limpiar() {
        for (size_t i = 0; i < N; ++i) valores_[i] = 0.0f;
        indice_ = 0;
        cantidad_ = 0;
        suma_ = 0.0f;
    }

private:
    float valores_[N] = {};
    size_t indice_ = 0;
    size_t cantidad_ = 0;
    float suma_ = 0.0f;
};

static PromedioMovil<ENCODER_AVG_WINDOW> filtroEncoderFL;
static PromedioMovil<ENCODER_AVG_WINDOW> filtroEncoderFR;
static PromedioMovil<ENCODER_AVG_WINDOW> filtroEncoderBL;
static PromedioMovil<ENCODER_AVG_WINDOW> filtroEncoderBR;
static PromedioMovil<IMU_GYRO_AVG_WINDOW> filtroGyroZ;

static void setup_PCNT(int pin, pcnt_unit_t unit) {
    pcnt_config_t pcnt_config = {};
    pcnt_config.pulse_gpio_num = pin;
    pcnt_config.ctrl_gpio_num = PCNT_PIN_NOT_USED;
    pcnt_config.lctrl_mode = PCNT_MODE_KEEP;
    pcnt_config.hctrl_mode = PCNT_MODE_KEEP;
    pcnt_config.pos_mode = PCNT_COUNT_INC;
    // Un pulso por ranura: contar un solo flanco evita convertir 20 PPR en 40.
    pcnt_config.neg_mode = PCNT_COUNT_DIS;
    pcnt_config.counter_h_lim = 32767;
    pcnt_config.counter_l_lim = -32768;
    pcnt_config.unit = unit;
    pcnt_config.channel = PCNT_CHANNEL_0;
    pcnt_unit_config(&pcnt_config);
    pcnt_set_filter_value(unit, 100);
    pcnt_filter_enable(unit);
    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);
    pcnt_counter_resume(unit);
}

void setup_Sensores() {
    LOG_CORE("Inicializando PCNT (4 encoders) y MPU6050...");
    setup_PCNT(PIN_ENC_FL, PCNT_UNIT_0);
    setup_PCNT(PIN_ENC_FR, PCNT_UNIT_1);
    setup_PCNT(PIN_ENC_BL, PCNT_UNIT_2);
    setup_PCNT(PIN_ENC_BR, PCNT_UNIT_3);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setTimeout(10);

    if (!mpu.begin()) {
        Serial.println("ERROR: MPU6050 no detectado. Modo autonomo deshabilitado.");
        mpu_inicializado = false;
    } else {
        mpu_inicializado = true;
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
        delay(250);
        // Calibracion rustica pero determinista: el carro debe permanecer
        // quieto mientras se suman las muestras de reposo del eje Z.
        float suma_z = 0.0f;
        uint16_t muestras_validas = 0;
        for (uint16_t i = 0; i < IMU_CALIBRATION_SAMPLES; ++i) {
            sensors_event_t a, g, temp;
            if (mpu.getEvent(&a, &g, &temp)) {
                suma_z += g.gyro.z;
                ++muestras_validas;
            }
            delay(2);
        }
        if (muestras_validas >= IMU_CALIBRATION_SAMPLES * 3 / 4) {
            gyro_z_offset_rad_s = suma_z / static_cast<float>(muestras_validas);
            mpu_calibrado = true;
            Serial.printf("MPU6050 calibrado: offset GZ=%.6f rad/s (%u muestras).\n",
                          gyro_z_offset_rad_s, muestras_validas);
        } else {
            mpu_calibrado = false;
            Serial.println("ERROR: MPU6050 sin suficientes muestras validas de calibracion.");
        }
        tiempoAnteriorIMU = millis();
        ultimaLecturaIMU = tiempoAnteriorIMU;
        anguloZ_acum = 0.0f;
        Serial.println("MPU6050 inicializado correctamente.");
    }
}

static void leerEncoders(SensorSnapshot &snap) {
    int16_t current_FL = 0, current_FR = 0, current_BL = 0, current_BR = 0;
    const pcnt_unit_t units[4] = {PCNT_UNIT_0, PCNT_UNIT_1, PCNT_UNIT_2, PCNT_UNIT_3};
    int16_t* values[4] = {&current_FL, &current_FR, &current_BL, &current_BR};
    for (int i = 0; i < 4; ++i) {
        pcnt_counter_pause(units[i]);
        pcnt_get_counter_value(units[i], values[i]);
        pcnt_counter_clear(units[i]);
        pcnt_counter_resume(units[i]);
    }
    snap.pulsosFL += current_FL;
    snap.pulsosFR += current_FR;
    snap.pulsosBL += current_BL;
    snap.pulsosBR += current_BR;

    snap.delta_pulsos_filtrado_FL = filtroEncoderFL.agregar(current_FL);
    snap.delta_pulsos_filtrado_FR = filtroEncoderFR.agregar(current_FR);
    snap.delta_pulsos_filtrado_BL = filtroEncoderBL.agregar(current_BL);
    snap.delta_pulsos_filtrado_BR = filtroEncoderBR.agregar(current_BR);

    const float cm_por_pulso = (PI * WHEEL_DIAMETER_ODOMETRY_CM) /
                               static_cast<float>(ENCODER_PPR);
    const float ventanas_por_segundo = 1000.0f / static_cast<float>(SENSOR_PERIOD_MS);
    const float delta_l = (snap.delta_pulsos_filtrado_FL + snap.delta_pulsos_filtrado_BL) * 0.5f;
    const float delta_r = (snap.delta_pulsos_filtrado_FR + snap.delta_pulsos_filtrado_BR) * 0.5f;
    snap.velocidad_filtrada_L_cm_s = delta_l * cm_por_pulso * ventanas_por_segundo;
    snap.velocidad_filtrada_R_cm_s = delta_r * cm_por_pulso * ventanas_por_segundo;
}

void resetFiltrosEncoder() {
    filtroEncoderFL.limpiar();
    filtroEncoderFR.limpiar();
    filtroEncoderBL.limpiar();
    filtroEncoderBR.limpiar();
}

void resetOrientacionIMU() {
    portENTER_CRITICAL(&muxOrientacionIMU);
    anguloZ_acum = 0.0f;
    anguloZ = 0.0f;
    filtroGyroZ.limpiar();
    tiempoAnteriorIMU = millis();
    portEXIT_CRITICAL(&muxOrientacionIMU);
}

float obtenerYawIMUDeg() {
    portENTER_CRITICAL(&muxOrientacionIMU);
    const float copia = anguloZ;
    portEXIT_CRITICAL(&muxOrientacionIMU);
    return copia;
}

bool recentrarYawIMUEnReposo() {
    bool recentrado = false;
    portENTER_CRITICAL(&muxOrientacionIMU);
    if (fabsf(anguloZ) >= YAW_RECENTER_THRESHOLD_DEG) {
        // Truncar conserva el residuo con su signo. Solo se llama sin maniobra
        // activa, por lo que ninguna referencia de progreso puede invalidarse.
        const float vueltas = truncf(anguloZ / 360.0f);
        const float grados = vueltas * 360.0f;
        anguloZ_acum -= grados * PI / 180.0f;
        anguloZ -= grados;
        ++contadorRecentradosYaw;
        recentrado = true;
    }
    portEXIT_CRITICAL(&muxOrientacionIMU);
    return recentrado;
}

uint32_t cantidadRecentradosYawIMU() {
    portENTER_CRITICAL(&muxOrientacionIMU);
    const uint32_t copia = contadorRecentradosYaw;
    portEXIT_CRITICAL(&muxOrientacionIMU);
    return copia;
}

static void leerGiroscopio(SensorSnapshot &snap) {
    snap.imu_deltaZ_rad = 0.0f;
    snap.gyro_z_filtrado_rad_s = 0.0f;
    snap.yaw_integrado_deg = obtenerYawIMUDeg();
    snap.mpu_present = mpu_inicializado;
    snap.mpu_calibrated = mpu_calibrado;
    snap.gyro_z_offset_rad_s = gyro_z_offset_rad_s;
    snap.mpu_stale = !mpu_calibrado;

    if (!mpu_inicializado) {
        snap.mpu_stale = true;
        return;
    }
    if (!mpu_calibrado) return;

    sensors_event_t a, g, temp;
    if (!mpu.getEvent(&a, &g, &temp)) {
        snap.mpu_stale = (millis() - ultimaLecturaIMU) > 100;
        return;
    }

    unsigned long t_ahora = millis();
    float dt = (t_ahora - tiempoAnteriorIMU) / 1000.0f;
    if (dt <= 0.0f || dt > 0.5f) {
        tiempoAnteriorIMU = t_ahora;
        return;
    }
    tiempoAnteriorIMU = t_ahora;
    ultimaLecturaIMU = t_ahora;
    snap.mpu_stale = false;

    // El offset se resta en el marco crudo del sensor y después se transforma
    // al marco canónico del robot. No se corrige sólo la gráfica: control,
    // odometría y telemetría consumen todos el mismo signo normalizado.
    const float velocidadZ =
        (g.gyro.z - gyro_z_offset_rad_s) * MPU_YAW_POLARITY;
    portENTER_CRITICAL(&muxOrientacionIMU);
    float velocidadFiltrada = filtroGyroZ.agregar(velocidadZ);
    if (fabsf(velocidadFiltrada) < IMU_GYRO_DEADBAND_RAD_S) velocidadFiltrada = 0.0f;
    snap.gyro_z_filtrado_rad_s = velocidadFiltrada;
    snap.imu_deltaZ_rad = velocidadFiltrada * dt;

    anguloZ_acum += snap.imu_deltaZ_rad;
    anguloZ = anguloZ_acum * 180.0f / PI;
    snap.yaw_integrado_deg = anguloZ;
    portEXIT_CRITICAL(&muxOrientacionIMU);
}

SensorSnapshot leerSensoresSincrono() {
    snapshotControl.timestamp_us = static_cast<uint64_t>(esp_timer_get_time());
    snapshotControl.timestamp_ms = static_cast<uint32_t>(snapshotControl.timestamp_us / 1000ULL);
    ++snapshotControl.sequence;

    leerEncoders(snapshotControl);
    leerGiroscopio(snapshotControl);

    portENTER_CRITICAL(&muxSnapshotSensores);
    snapshotPublicado = snapshotControl;
    portEXIT_CRITICAL(&muxSnapshotSensores);
    return snapshotControl;
}

const SensorSnapshot& snapshotSensoresControl() {
    return snapshotControl;
}

bool obtenerUltimoSnapshotSensores(SensorSnapshot &destino) {
    portENTER_CRITICAL(&muxSnapshotSensores);
    destino = snapshotPublicado;
    portEXIT_CRITICAL(&muxSnapshotSensores);
    return destino.sequence != 0;
}
