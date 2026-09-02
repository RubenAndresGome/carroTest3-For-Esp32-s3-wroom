// SNAPSHOT HISTORICO GENERADO; NO COMPILAR COMO FUENTE CANONICA
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <Wire.h>
#include <cmath>
#include <math.h>


constexpr char FIRMWARE_VERSION[] = "robot-s3-v1";
constexpr char ROBOT_ID_PREFIX[] = "ESP32S3";
constexpr char PROTOCOL_NAME[] = "robot-s3-json-v1";

// WiFi
extern const char* ssid_AP;
extern const char* password_AP;

// Motores (DRV8833)
// FL/BL: izquierdo superior/inferior. FR/BR: derecho superior/inferior.
const int PIN_FL_FWD = 6;
const int PIN_FL_REV = 7;
const int PIN_BL_FWD = 4;
const int PIN_BL_REV = 5;
const int PIN_FR_FWD = 17;
const int PIN_FR_REV = 18;
const int PIN_BR_FWD = 15;
const int PIN_BR_REV = 16;

// Encoders
const int PIN_ENC_FL = 10;  // superior izquierdo, cable rojo
const int PIN_ENC_FR = 11;  // superior derecho, cable cafe
const int PIN_ENC_BL = 12;  // inferior izquierdo, cable negro
const int PIN_ENC_BR = 13;  // inferior derecho, cable blanco

// MPU6050 (I2C)
const int PIN_I2C_SDA = 1;
const int PIN_I2C_SCL = 2;

// Muestreo y filtros. PCNT conserva los pulsos acumulados sin filtrar; el
// promedio de 10 ventanas de 10 ms se usa solamente para estimar velocidad.
constexpr uint32_t SENSOR_PERIOD_MS = 10;
constexpr size_t ENCODER_AVG_WINDOW = 10;
constexpr size_t IMU_GYRO_AVG_WINDOW = 8;
constexpr uint16_t IMU_CALIBRATION_SAMPLES = 256;
constexpr float IMU_GYRO_DEADBAND_RAD_S = 0.005f;

// Correccion empirica del giro relativo. Un valor positivo pide girar un poco
// mas en ambos sentidos; uno negativo pide girar menos. Debe medirse en piso
// real (por ejemplo, +2.5 si una orden de 90 grados termina cerca de 87.5).
constexpr float THETA_ERROR_DEG = 0.0f;

constexpr float WHEEL_DIAMETER_CM = 6.6f;
constexpr int ENCODER_PPR = 20;

// Calibración y PID
const float CM_POR_PULSO = 1.0;
const int VELOCIDAD_BASE_RECTO = 255;
const int VELOCIDAD_BASE_GIRO = 255;
const int VELOCIDAD_MINIMA_GIRO = 245;
const float Kp_giro = 1.5;
const float Kp_recto = 1.5;

#define ENABLE_DEBUG 1 // Pon esto a 0 para modo producción

#if ENABLE_DEBUG
  #define LOG_CORE(msg) Serial.printf("[CORE %d] %s(): %s\n", xPortGetCoreID(), __func__, msg)
  #define LOG_VAL(nombre, valor) Serial.printf("[CORE %d] %s(): %s = %.2f\n", xPortGetCoreID(), __func__, nombre, (float)valor)
  
  // Macro para loops de alta frecuencia (imprime 1 vez cada N llamadas)
  #define LOG_THROTTLED(msg, n) \
      static int _count_##__LINE__ = 0; \
      if (++_count_##__LINE__ >= (n)) { \
          Serial.printf("[CORE %d] %s(): %s\n", xPortGetCoreID(), __func__, msg); \
          _count_##__LINE__ = 0; \
      }

  #define LOG_MEMORY() Serial.printf("[CORE %d] MEMORIA LIBRE: %d bytes\n", xPortGetCoreID(), ESP.getFreeHeap())
#else
  #define LOG_CORE(msg)
  #define LOG_VAL(nombre, valor)
  #define LOG_THROTTLED(msg, n)
  #define LOG_MEMORY()
#endif

enum EstadoRobot {
  IDLE,
  GIRANDO,
  ESPERANDO_ESTABILIZACION,
  AVANZANDO,
  MANUAL,
  TESTING,
  ESTOP_LATCHED,
  SAFE_STOP_COMMS,
  FAULT_SENSOR
};

// Odometría Cruda
extern long pulsosIzquierdos;
extern long pulsosDerechos;

// Giroscopio
extern float anguloZ;

// Objetivos Autónomos
extern float nuevoDestinoX;
extern float nuevoDestinoY;

// Testing y Manual
extern int pwm_target_l;
extern int pwm_target_r;
extern unsigned long tiempoTestingInicio;
extern unsigned long duracionTesting;

// Máquina de estados
extern EstadoRobot estadoActual;
extern unsigned long tiempoEspera;
extern unsigned long tiempoAnterior;
extern unsigned long inicioGiro;

enum TipoComando {
    CMD_INVALID,
    CMD_ESTOP,
    CMD_STOP,
    CMD_CLEAR_ROUTE,
    CMD_RESET_POSE,
    CMD_CLEAR_FAULT,
    CMD_MOVE_ABS,
    CMD_MANUAL,
    CMD_TEST_PWM
};

struct ComandoRed {
    // UUID hexadecimal generado por Python (32 caracteres + terminador).
    // Las colas FreeRTOS copian la estructura completa, por lo que el ID se
    // conserva hasta accepted/completed/rejected.
    char id[33];
    TipoComando tipo;
    float arg_x;
    float arg_y;
    int pwm_l;
    int pwm_r;
    uint32_t tiempo_ms;
};

extern QueueHandle_t colaComandos;
extern QueueHandle_t buzonManual;
extern volatile bool flag_ESTOP_ISR;

enum TipoEvento {
    EVT_COMPLETED,
    EVT_REJECTED,
    EVT_FAULT,
    EVT_TELEMETRY
};

struct EventoRed {
    TipoEvento tipo;
    char cmd_id[33];
    char detalle[32];
};

extern QueueHandle_t colaEventosRed;

void encolarEvento(TipoEvento tipo, const char* cmd_id, const char* detalle);

class PoseEstimator {
public:
    PoseEstimator();

    void inicializar(float rueda_diametro_cm, int encoder_ppr);

    void reset();

    void actualizarOdometria(int64_t pulsosFL, int64_t pulsosFR, int64_t pulsosBL, int64_t pulsosBR, EstadoRobot estado);

    void actualizarOrientacion(float imu_delta_z);

    float getX() const { return x_global; }
    float getY() const { return y_global; }
    float getThetaRad() const { return theta_rad; }
    float getThetaDeg() const { return theta_rad * 180.0 / PI; }

    float distanciaAlObjetivo(float obj_x, float obj_y);
    float anguloAlObjetivoRad(float obj_x, float obj_y);

private:
    float x_global;
    float y_global;
    float theta_rad;

    int64_t last_pulsos_FL;
    int64_t last_pulsos_FR;
    int64_t last_pulsos_BL;
    int64_t last_pulsos_BR;

    float cm_por_pulso;

    static constexpr float DISTANCIA_EJES_CM = 18.0f;
};

extern PoseEstimator PoseGlobal;

struct SensorSnapshot {
    int64_t pulsosFL;
    int64_t pulsosFR;
    int64_t pulsosBL;
    int64_t pulsosBR;
    float delta_pulsos_filtrado_FL;
    float delta_pulsos_filtrado_FR;
    float delta_pulsos_filtrado_BL;
    float delta_pulsos_filtrado_BR;
    float velocidad_filtrada_L_cm_s;
    float velocidad_filtrada_R_cm_s;
    float imu_deltaZ_rad;
    float gyro_z_filtrado_rad_s;
    float gyro_z_offset_rad_s;
    float yaw_integrado_deg;
    uint32_t timestamp_ms;
    bool mpu_present;
    bool mpu_stale;
    bool mpu_calibrated;
    uint32_t sequence;
};

extern QueueHandle_t colaSensores;

void setup_Sensores();
void Task_Sensors(void *pvParameters);
void resetOrientacionIMU();

extern int pwm_aplicado_L;
extern int pwm_aplicado_R;

void setup_Motores();
void setMotorPWM(int pinFwd, int pinRev, int vel);
void aplicarVelocidades(int velIzq, int velDer);
void frenarMotores();


extern unsigned long tiempoInicioGiro;
extern unsigned long tiempoInicioAvance;

constexpr unsigned long TIMEOUT_GIRO_MS = 8000;
constexpr unsigned long TIMEOUT_AVANCE_MS = 15000;
constexpr float DISTANCIA_MAX_CM = 100.0;
constexpr unsigned long ESTABILIZACION_MS = 300;

bool calcularDestino(float x_destino, float y_destino, const char* command_id);
void controlarGiro();
void controlarAvance();

class Seguridad {
public:
    Seguridad();

    void alimentarWatchdogRed();
    void registrarHeartbeat();

    bool auditarSalud(const SensorSnapshot &snap, int pwm_L, int pwm_R);

    void forzarEStop();
    void resetFallo();

private:
    unsigned long ultimo_comando_ms;
    unsigned long ultimo_heartbeat_ms;
    unsigned long inicio_movimiento_ms;
    long pulsos_movimiento_iniciales;

    // Parada segura si el controlador PC deja de enviar JSON/heartbeat.
    const unsigned long TIMEOUT_WIFI_MANUAL_MS = 400;
    const unsigned long TIMEOUT_STALL_MS = 500;
};

extern Seguridad WatchdogSeguridad;


void setup_Red();
void procesarWebSockets();
void pushTelemetria();

const char* ssid_AP = WIFI_AP_SSID;
const char* password_AP = WIFI_AP_PASSWORD;

long pulsosIzquierdos = 0;
long pulsosDerechos = 0;

float anguloZ = 0.0;

float nuevoDestinoX = 0;
float nuevoDestinoY = 0;

int pwm_target_l = 0;
int pwm_target_r = 0;
unsigned long tiempoTestingInicio = 0;
unsigned long duracionTesting = 0;

EstadoRobot estadoActual = IDLE;
unsigned long tiempoEspera = 0;
unsigned long tiempoAnterior = 0;
unsigned long inicioGiro = 0;

QueueHandle_t colaEventosRed = nullptr;

void encolarEvento(TipoEvento tipo, const char* cmd_id, const char* detalle) {
    if (!colaEventosRed) return;
    EventoRed evt = {};
    evt.tipo = tipo;
    if (cmd_id) {
        strncpy(evt.cmd_id, cmd_id, sizeof(evt.cmd_id) - 1);
        evt.cmd_id[sizeof(evt.cmd_id) - 1] = '\0';
    }
    if (detalle) {
        strncpy(evt.detalle, detalle, sizeof(evt.detalle) - 1);
        evt.detalle[sizeof(evt.detalle) - 1] = '\0';
    } else {
        evt.detalle[0] = '\0';
    }
    xQueueSend(colaEventosRed, &evt, 0);
}

PoseEstimator PoseGlobal;

PoseEstimator::PoseEstimator() {
    reset();
}

void PoseEstimator::inicializar(float rueda_diametro_cm, int encoder_ppr) {
    cm_por_pulso = (PI * rueda_diametro_cm) / (float)encoder_ppr;
}

void PoseEstimator::reset() {
    x_global = 0.0;
    y_global = 0.0;
    theta_rad = 0.0;
    last_pulsos_FL = 0;
    last_pulsos_FR = 0;
    last_pulsos_BL = 0;
    last_pulsos_BR = 0;
}

void PoseEstimator::actualizarOdometria(int64_t pulsosFL, int64_t pulsosFR, int64_t pulsosBL, int64_t pulsosBR, EstadoRobot estado) {
    const int64_t deltaL = ((pulsosFL - last_pulsos_FL) + (pulsosBL - last_pulsos_BL)) / 2;
    const int64_t deltaR = ((pulsosFR - last_pulsos_FR) + (pulsosBR - last_pulsos_BR)) / 2;

    last_pulsos_FL = pulsosFL;
    last_pulsos_FR = pulsosFR;
    last_pulsos_BL = pulsosBL;
    last_pulsos_BR = pulsosBR;

    float distL = deltaL * cm_por_pulso;
    float distR = deltaR * cm_por_pulso;

    // La IMU es la única fuente de rumbo para no integrar dos veces el mismo giro.
    if (estado == AVANZANDO) {
        float distCentro = (distL + distR) / 2.0;
        x_global += distCentro * cos(theta_rad);
        y_global += distCentro * sin(theta_rad);
    }
}

void PoseEstimator::actualizarOrientacion(float imu_delta_z) {
    theta_rad += imu_delta_z;

    while (theta_rad < 0) theta_rad += 2 * PI;
    while (theta_rad >= 2 * PI) theta_rad -= 2 * PI;
}

float PoseEstimator::distanciaAlObjetivo(float obj_x, float obj_y) {
    float dx = obj_x - x_global;
    float dy = obj_y - y_global;
    return sqrt(dx*dx + dy*dy);
}

float PoseEstimator::anguloAlObjetivoRad(float obj_x, float obj_y) {
    float dx = obj_x - x_global;
    float dy = obj_y - y_global;
    return atan2(dy, dx);
}

Adafruit_MPU6050 mpu;
static bool mpu_inicializado = false;
static bool mpu_calibrado = false;
static float anguloZ_acum = 0.0f;
static float gyro_z_offset_rad_s = 0.0f;
static unsigned long tiempoAnteriorIMU = 0;
static unsigned long ultimaLecturaIMU = 0;
QueueHandle_t colaSensores = nullptr;

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

    colaSensores = xQueueCreate(1, sizeof(SensorSnapshot));

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

    const float cm_por_pulso = (PI * WHEEL_DIAMETER_CM) / static_cast<float>(ENCODER_PPR);
    const float ventanas_por_segundo = 1000.0f / static_cast<float>(SENSOR_PERIOD_MS);
    const float delta_l = (snap.delta_pulsos_filtrado_FL + snap.delta_pulsos_filtrado_BL) * 0.5f;
    const float delta_r = (snap.delta_pulsos_filtrado_FR + snap.delta_pulsos_filtrado_BR) * 0.5f;
    snap.velocidad_filtrada_L_cm_s = delta_l * cm_por_pulso * ventanas_por_segundo;
    snap.velocidad_filtrada_R_cm_s = delta_r * cm_por_pulso * ventanas_por_segundo;
}

void resetOrientacionIMU() {
    anguloZ_acum = 0.0f;
    anguloZ = 0.0f;
    filtroGyroZ.limpiar();
    tiempoAnteriorIMU = millis();
}

static void leerGiroscopio(SensorSnapshot &snap) {
    snap.imu_deltaZ_rad = 0.0f;
    snap.gyro_z_filtrado_rad_s = 0.0f;
    snap.yaw_integrado_deg = anguloZ;
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

    float velocidadZ = g.gyro.z - gyro_z_offset_rad_s; // rad/s, sin bias de reposo
    float velocidadFiltrada = filtroGyroZ.agregar(velocidadZ);
    if (fabsf(velocidadFiltrada) < IMU_GYRO_DEADBAND_RAD_S) velocidadFiltrada = 0.0f;
    snap.gyro_z_filtrado_rad_s = velocidadFiltrada;
    snap.imu_deltaZ_rad = velocidadFiltrada * dt;

    anguloZ_acum += snap.imu_deltaZ_rad;
    anguloZ = anguloZ_acum * 180.0f / PI;
    snap.yaw_integrado_deg = anguloZ;
}

void Task_Sensors(void *pvParameters) {
    LOG_CORE("Task_Sensors iniciada en Core 1 (prio 8)");

    SensorSnapshot snap = {};
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFreq = SENSOR_PERIOD_MS / portTICK_PERIOD_MS;

    for (;;) {
        snap.timestamp_ms = millis();
        ++snap.sequence;

        leerEncoders(snap);
        leerGiroscopio(snap);

        xQueueOverwrite(colaSensores, &snap);

        static int cnt = 0;
        if (++cnt >= 100) {
            LOG_THROTTLED("Heartbeat Task_Sensors (100Hz OK)", 100);
            cnt = 0;
        }

        vTaskDelayUntil(&xLastWakeTime, xFreq);
    }
}

int pwm_aplicado_L = 0;
int pwm_aplicado_R = 0;

void setMotorPWM(int pinFwd, int pinRev, int vel) {
  if (vel > 0) {
    analogWrite(pinFwd, vel);
    analogWrite(pinRev, 0);
  } else if (vel < 0) {
    analogWrite(pinFwd, 0);
    analogWrite(pinRev, -vel);
  } else {
    analogWrite(pinFwd, 0);
    analogWrite(pinRev, 0);
  }
}

void aplicarVelocidades(int velIzq, int velDer) {
  velIzq = constrain(velIzq, -255, 255);
  velDer = constrain(velDer, -255, 255);
  pwm_aplicado_L = velIzq;
  pwm_aplicado_R = velDer;
  setMotorPWM(PIN_FL_FWD, PIN_FL_REV, velIzq);
  setMotorPWM(PIN_BL_FWD, PIN_BL_REV, velIzq);
  setMotorPWM(PIN_FR_FWD, PIN_FR_REV, velDer);
  setMotorPWM(PIN_BR_FWD, PIN_BR_REV, velDer);
}

void frenarMotores() {
  aplicarVelocidades(0, 0);
}

void setup_Motores() {
  LOG_CORE("Inicializando Motores DRV8833...");
  pinMode(PIN_FL_FWD, OUTPUT); pinMode(PIN_FL_REV, OUTPUT);
  pinMode(PIN_BL_FWD, OUTPUT); pinMode(PIN_BL_REV, OUTPUT);
  pinMode(PIN_FR_FWD, OUTPUT); pinMode(PIN_FR_REV, OUTPUT);
  pinMode(PIN_BR_FWD, OUTPUT); pinMode(PIN_BR_REV, OUTPUT);
  frenarMotores();
}

float distanciaInicialManejo = 0;
float distanciaObjetivoCM = 0;
float anguloObjetivoGrados = 0;
static char active_move_id[33] = {};

unsigned long tiempoInicioGiro = 0;
unsigned long tiempoInicioAvance = 0;

bool calcularDestino(float x_destino, float y_destino, const char* command_id) {
    LOG_CORE("Calculando nueva trayectoria");

    distanciaObjetivoCM = PoseGlobal.distanciaAlObjetivo(x_destino, y_destino);
    if (distanciaObjetivoCM > DISTANCIA_MAX_CM) {
        LOG_CORE("Distancia excede limite de seguridad (100cm). Comando rechazado.");
        return false;
    }

    nuevoDestinoX = x_destino;
    nuevoDestinoY = y_destino;
    distanciaInicialManejo = distanciaObjetivoCM;

    float theta_destino = PoseGlobal.anguloAlObjetivoRad(x_destino, y_destino);
    float giro = theta_destino - PoseGlobal.getThetaRad();

    while (giro > M_PI) giro -= 2 * M_PI;
    while (giro < -M_PI) giro += 2 * M_PI;

    anguloObjetivoGrados = giro * 180.0 / PI;
    if (fabsf(anguloObjetivoGrados) > 0.1f && THETA_ERROR_DEG != 0.0f) {
        anguloObjetivoGrados += copysignf(THETA_ERROR_DEG, anguloObjetivoGrados);
    }
    if (command_id) {
        strncpy(active_move_id, command_id, sizeof(active_move_id) - 1);
        active_move_id[sizeof(active_move_id) - 1] = '\0';
    } else {
        active_move_id[0] = '\0';
    }

    Serial.println("---- NUEVO OBJETIVO ----");
    Serial.print("Distancia: "); Serial.println(distanciaObjetivoCM);
    Serial.print("Giro: "); Serial.println(anguloObjetivoGrados);

    anguloZ = 0;
    inicioGiro = 0;
    tiempoAnterior = millis();
    tiempoInicioGiro = millis();

    estadoActual = EstadoRobot::GIRANDO;
    return true;
}

void controlarGiro() {
    LOG_THROTTLED("Controlando Giro", 50);

    if (millis() - tiempoInicioGiro > TIMEOUT_GIRO_MS) {
        LOG_CORE("TIMEOUT GIRO: excedido limite de 8s");
        frenarMotores();
        estadoActual = EstadoRobot::FAULT_SENSOR;
        encolarEvento(EVT_FAULT, active_move_id, "timeout_giro");
        return;
    }

    float error = fabs(anguloObjetivoGrados) - fabs(anguloZ);
    if (error <= 1.0) {
        LOG_CORE("Giro completado, esperando estabilizacion");
        frenarMotores();
        estadoActual = EstadoRobot::ESPERANDO_ESTABILIZACION;
        tiempoEspera = millis();
        return;
    }

    int velocidad = VELOCIDAD_MINIMA_GIRO + (error * Kp_giro);
    if (velocidad > 255) velocidad = 255;

    if (anguloObjetivoGrados > 0) {
        aplicarVelocidades(-velocidad, velocidad);
    } else {
        aplicarVelocidades(velocidad, -velocidad);
    }
}

void controlarAvance() {
    LOG_THROTTLED("Controlando Avance (Trapezoidal)", 50);

    if (millis() - tiempoInicioAvance > TIMEOUT_AVANCE_MS) {
        LOG_CORE("TIMEOUT AVANCE: excedido limite de 15s");
        frenarMotores();
        estadoActual = EstadoRobot::FAULT_SENSOR;
        encolarEvento(EVT_FAULT, active_move_id, "timeout_avance");
        return;
    }

    float distanciaRestante = PoseGlobal.distanciaAlObjetivo(nuevoDestinoX, nuevoDestinoY);

    if (distanciaRestante <= 2.0) {
        LOG_CORE("Destino alcanzado");
        frenarMotores();
        estadoActual = EstadoRobot::IDLE;
        Serial.println("Destino alcanzado");
        encolarEvento(EVT_COMPLETED, active_move_id, "move_completed");
        active_move_id[0] = '\0';
        return;
    }

    int velBase = VELOCIDAD_BASE_RECTO;

    if (distanciaRestante < 15.0) {
        velBase = map(distanciaRestante, 2.0, 15.0, VELOCIDAD_MINIMA_GIRO, VELOCIDAD_BASE_RECTO);
    }

    float errorOrientacion = PoseGlobal.anguloAlObjetivoRad(nuevoDestinoX, nuevoDestinoY) - PoseGlobal.getThetaRad();
    while (errorOrientacion > PI) errorOrientacion -= 2 * PI;
    while (errorOrientacion < -PI) errorOrientacion += 2 * PI;

    float correccion = (errorOrientacion * 180.0 / PI) * Kp_recto;

    int velIzq = velBase - correccion;
    int velDer = velBase + correccion;

    if (velIzq > 255) velIzq = 255; else if (velIzq < 0) velIzq = 0;
    if (velDer > 255) velDer = 255; else if (velDer < 0) velDer = 0;

    aplicarVelocidades(velIzq, velDer);
}

Seguridad WatchdogSeguridad;

Seguridad::Seguridad() {
    ultimo_comando_ms = 0;
    ultimo_heartbeat_ms = 0;
    inicio_movimiento_ms = 0;
    pulsos_movimiento_iniciales = 0;
}

void Seguridad::registrarHeartbeat() {
    ultimo_heartbeat_ms = millis();
}

void Seguridad::alimentarWatchdogRed() {
    ultimo_comando_ms = millis();
}

bool Seguridad::auditarSalud(const SensorSnapshot &snap, int pwm_L, int pwm_R) {
    if (estadoActual == ESTOP_LATCHED || estadoActual == FAULT_SENSOR) {
        return true;
    }

    if (estadoActual == MANUAL) {
        if (millis() - ultimo_comando_ms > TIMEOUT_WIFI_MANUAL_MS) {
            LOG_CORE("ALERTA: Perdida de Wi-Fi en MANUAL. Freno de seguridad.");
            frenarMotores();
            estadoActual = SAFE_STOP_COMMS;
            return true;
        }
    }

    if (estadoActual == AVANZANDO || estadoActual == GIRANDO || estadoActual == MANUAL) {
        int pwm_max = abs(pwm_L);
        if (abs(pwm_R) > pwm_max) pwm_max = abs(pwm_R);

        if (pwm_max > 80) {
            if (inicio_movimiento_ms == 0) {
                inicio_movimiento_ms = millis();
                pulsos_movimiento_iniciales = snap.pulsosFL + snap.pulsosFR + snap.pulsosBL + snap.pulsosBR;
            } else if (millis() - inicio_movimiento_ms > TIMEOUT_STALL_MS) {
                long deltas = (snap.pulsosFL + snap.pulsosFR + snap.pulsosBL + snap.pulsosBR) - pulsos_movimiento_iniciales;
                if (abs(deltas) < 2) {
                    LOG_CORE("ERROR CRITICO: Motor atascado o fallo PCNT.");
                    frenarMotores();
                    estadoActual = FAULT_SENSOR;
                    return true;
                } else {
                    inicio_movimiento_ms = millis();
                    pulsos_movimiento_iniciales = snap.pulsosFL + snap.pulsosFR + snap.pulsosBL + snap.pulsosBR;
                }
            }
        } else {
            inicio_movimiento_ms = 0;
        }
    } else {
        inicio_movimiento_ms = 0;
    }

    return false;
}

void Seguridad::forzarEStop() {
    LOG_CORE("ACTIVANDO E-STOP LOGICO");
    frenarMotores();
    estadoActual = ESTOP_LATCHED;
}

void Seguridad::resetFallo() {
    frenarMotores();
    if (estadoActual != ESTOP_LATCHED && estadoActual != FAULT_SENSOR && estadoActual != SAFE_STOP_COMMS) {
        LOG_CORE("Rearme rechazado: no hay fallo activo.");
        return;
    }
    LOG_CORE("Rearmando sistema tras fallo.");
    estadoActual = IDLE;
    alimentarWatchdogRed();
}

#define MAX_WS_MSG 512

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

static char msg_buf[MAX_WS_MSG + 1];

const char* estadoTexto() {
  switch (estadoActual) {
    case IDLE: return "IDLE";
    case GIRANDO: return "GIRANDO";
    case ESPERANDO_ESTABILIZACION: return "ESPERANDO";
    case AVANZANDO: return "AVANZANDO";
    case MANUAL: return "MANUAL";
    case TESTING: return "TESTING";
    case ESTOP_LATCHED: return "ESTOP_LATCHED";
    case SAFE_STOP_COMMS: return "SAFE_STOP";
    case FAULT_SENSOR: return "FAULT";
    default: return "UNKNOWN";
  }
}

uint32_t active_client_id = 0;

static void copiarId(JsonVariantConst value, char* destino, size_t capacidad) {
    destino[0] = '\0';
    if (value.is<const char*>()) {
        strncpy(destino, value.as<const char*>(), capacidad - 1);
        destino[capacidad - 1] = '\0';
    } else if (value.is<unsigned long>()) {
        snprintf(destino, capacidad, "%lu", value.as<unsigned long>());
    }
}

static ComandoRed parsearComando(JsonObjectConst doc) {
    ComandoRed cmd = {};
    cmd.tipo = CMD_INVALID;
    const bool protocol_v1 = doc["v"] == 1 && doc["type"] == "command";
    const char* name = protocol_v1 ? doc["name"] : doc["cmd"];
    if (!name) return cmd;
    copiarId(doc["id"], cmd.id, sizeof(cmd.id));
    if (!cmd.id[0]) snprintf(cmd.id, sizeof(cmd.id), "%08lX", millis());
    JsonObjectConst args = protocol_v1 ? doc["payload"].as<JsonObjectConst>() : doc;

    if (!strcmp(name, "estop")) cmd.tipo = CMD_ESTOP;
    else if (!strcmp(name, "stop")) cmd.tipo = CMD_STOP;
    else if (!strcmp(name, "reset_pose")) cmd.tipo = CMD_RESET_POSE;
    else if (!strcmp(name, "clear_fault")) cmd.tipo = CMD_CLEAR_FAULT;
    else if (!strcmp(name, "clear_route")) cmd.tipo = CMD_CLEAR_ROUTE;
    else if (!strcmp(name, "manual") || !strcmp(name, "test_pwm")) {
        if (!args["l"].is<int>() || !args["r"].is<int>()) return cmd;
        const int left = args["l"];
        const int right = args["r"];
        if (left < -255 || left > 255 || right < -255 || right > 255) return cmd;
        cmd.pwm_l = left;
        cmd.pwm_r = right;
        cmd.tipo = !strcmp(name, "manual") ? CMD_MANUAL : CMD_TEST_PWM;
        if (cmd.tipo == CMD_TEST_PWM) {
            cmd.tiempo_ms = args["dur_ms"] | 500;
            if (cmd.tiempo_ms < 10 || cmd.tiempo_ms > 1500) cmd.tipo = CMD_INVALID;
        }
    } else if (!strcmp(name, "move")) {
        const char* x_key = protocol_v1 ? "x_mm" : "x";
        const char* y_key = protocol_v1 ? "y_mm" : "y";
        // Aceptar x/y en V1 durante la transición de la app; siempre convertir mm a cm.
        if (protocol_v1 && !args.containsKey("x_mm")) x_key = "x";
        if (protocol_v1 && !args.containsKey("y_mm")) y_key = "y";
        if (!args[x_key].is<float>() || !args[y_key].is<float>()) return cmd;
        cmd.arg_x = args[x_key].as<float>();
        cmd.arg_y = args[y_key].as<float>();
        if (protocol_v1) { cmd.arg_x /= 10.0f; cmd.arg_y /= 10.0f; }
        if (isfinite(cmd.arg_x) && isfinite(cmd.arg_y)) cmd.tipo = CMD_MOVE_ABS;
    }
    return cmd;
}

static void enviarResultado(AsyncWebSocketClient* client, const char* tipo,
                            const char* id = nullptr, const char* cmd = nullptr,
                            const char* reason = nullptr, const char* detail = nullptr) {
    StaticJsonDocument<256> doc;
    doc["v"] = 1;
    doc["type"] = tipo;
    if (id && id[0]) doc["id"] = id;
    if (cmd && cmd[0]) doc["cmd"] = cmd;
    if (reason && reason[0]) doc["reason"] = reason;
    if (detail && detail[0]) doc["detail"] = detail;
    char respuesta[256];
    const size_t escritos = serializeJson(doc, respuesta, sizeof(respuesta));
    if (escritos > 0 && escritos < sizeof(respuesta) - 1) client->text(respuesta);
}

static const char* nombreComando(TipoComando tipo) {
    switch (tipo) {
        case CMD_ESTOP: return "estop";
        case CMD_STOP: return "stop";
        case CMD_CLEAR_ROUTE: return "clear_route";
        case CMD_RESET_POSE: return "reset_pose";
        case CMD_CLEAR_FAULT: return "clear_fault";
        case CMD_MOVE_ABS: return "move";
        case CMD_MANUAL: return "manual";
        case CMD_TEST_PWM: return "test_pwm";
        default: return "";
    }
}

static void procesarEventosRed() {
    EventoRed evt;
    while (xQueueReceive(colaEventosRed, &evt, 0) == pdTRUE) {
        if (ws.count() == 0) continue;
        StaticJsonDocument<192> doc;
        doc["v"] = 1;
        if (evt.cmd_id[0]) doc["id"] = evt.cmd_id;
        if (evt.detalle[0]) doc["detail"] = evt.detalle;
        switch (evt.tipo) {
            case EVT_COMPLETED:
                doc["type"] = "completed";
                break;
            case EVT_REJECTED:
                doc["type"] = "rejected";
                doc["reason"] = evt.detalle;
                break;
            case EVT_FAULT:
                doc["type"] = "fault";
                break;
            case EVT_TELEMETRY:
                break;
            default:
                continue;
        }
        char respuesta[192];
        const size_t escritos = serializeJson(doc, respuesta, sizeof(respuesta));
        if (escritos > 0 && escritos < sizeof(respuesta) - 1) ws.textAll(respuesta);
    }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    if (active_client_id == 0) {
      active_client_id = client->id();
      Serial.printf("WS cliente #%u conectado. Tomando control.\n", client->id());
      client->text("{\"v\":1,\"type\":\"hello\",\"protocol\":\"robot-s3-json-v1\",\"role\":\"robot\"}");
    } else {
      Serial.printf("WS cliente #%u rechazado (Split Brain prevention).\n", client->id());
      enviarResultado(client, "rejected", nullptr, nullptr, "controller_locked");
      client->close();
    }
  } else if (type == WS_EVT_DISCONNECT) {
    if (client->id() == active_client_id) {
      Serial.printf("WS cliente principal #%u desconectado.\n", client->id());
      active_client_id = 0;
    }
  } else if (type == WS_EVT_DATA) {
    if (client->id() != active_client_id) return;

    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->opcode != WS_TEXT || !info->final || info->index != 0 || info->len != len) {
        enviarResultado(client, "rejected", nullptr, nullptr, "fragmented_message");
        return;
    }

    if (len > MAX_WS_MSG) {
        enviarResultado(client, "rejected", nullptr, nullptr, "msg_too_large");
        return;
    }

    memcpy(msg_buf, data, len);
    msg_buf[len] = '\0';

    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, msg_buf) != DeserializationError::Ok || !doc.is<JsonObject>()) {
        enviarResultado(client, "rejected", nullptr, nullptr, "invalid_json");
        return;
    }
    JsonObjectConst root = doc.as<JsonObjectConst>();
    const char* message_type = root["type"] | "";

    if (root["v"] == 1 && !strcmp(message_type, "heartbeat")) {
        // Heartbeat confirma que el bus Wi-Fi sigue vivo, pero no renueva por
        // si solo la orden manual: esa lease exige mensajes manual repetidos.
        WatchdogSeguridad.registrarHeartbeat();
        StaticJsonDocument<96> heartbeat;
        heartbeat["v"] = 1;
        heartbeat["type"] = "heartbeat_ack";
        if (root.containsKey("ts_ms")) heartbeat["ts_ms"] = root["ts_ms"];
        char respuesta[96];
        serializeJson(heartbeat, respuesta, sizeof(respuesta));
        client->text(respuesta);
        return;
    }

    if (root["v"] == 1 && !strcmp(message_type, "hello")) {
        client->text("{\"v\":1,\"type\":\"hello_ack\",\"protocol\":\"robot-s3-json-v1\"}");
        return;
    }

    ComandoRed cmd = parsearComando(root);

    if (cmd.tipo == CMD_INVALID) {
        enviarResultado(client, "rejected", cmd.id, nullptr, "invalid_command");
        return;
    }

    const char* command_name = nombreComando(cmd.tipo);

    if (cmd.tipo == CMD_ESTOP) {
        flag_ESTOP_ISR = true;
        enviarResultado(client, "accepted", cmd.id, command_name);
        return;
    }

    if (cmd.tipo == CMD_STOP) {
        if (xQueueSend(colaComandos, &cmd, 0) == pdTRUE) enviarResultado(client, "accepted", cmd.id, command_name);
        else enviarResultado(client, "rejected", cmd.id, command_name, "command_queue_full");
        return;
    }

    if (cmd.tipo == CMD_CLEAR_FAULT) {
        if (estadoActual == ESTOP_LATCHED || estadoActual == FAULT_SENSOR || estadoActual == SAFE_STOP_COMMS) {
            if (xQueueSend(colaComandos, &cmd, 0) == pdTRUE) enviarResultado(client, "accepted", cmd.id, command_name);
            else enviarResultado(client, "rejected", cmd.id, command_name, "command_queue_full");
        } else {
            enviarResultado(client, "rejected", cmd.id, command_name, "no_fault");
        }
        return;
    }

    if (cmd.tipo == CMD_RESET_POSE) {
        if (estadoActual == IDLE) {
            if (xQueueSend(colaComandos, &cmd, 0) == pdTRUE) enviarResultado(client, "accepted", cmd.id, command_name);
            else enviarResultado(client, "rejected", cmd.id, command_name, "command_queue_full");
        } else {
            enviarResultado(client, "rejected", cmd.id, command_name, "busy");
        }
        return;
    }

    if (cmd.tipo == CMD_CLEAR_ROUTE) {
        if (xQueueSend(colaComandos, &cmd, 0) == pdTRUE) enviarResultado(client, "accepted", cmd.id, command_name);
        else enviarResultado(client, "rejected", cmd.id, command_name, "command_queue_full");
        return;
    }

    if (cmd.tipo == CMD_MANUAL) {
        if (estadoActual == IDLE || estadoActual == MANUAL) {
            if (xQueueOverwrite(buzonManual, &cmd) == pdTRUE) enviarResultado(client, "accepted", cmd.id, command_name);
            else enviarResultado(client, "rejected", cmd.id, command_name, "manual_mailbox_unavailable");
        } else {
            enviarResultado(client, "rejected", cmd.id, command_name, "busy");
        }
        return;
    }

    if (cmd.tipo == CMD_MOVE_ABS) {
        if (estadoActual == IDLE) {
            if (xQueueSend(colaComandos, &cmd, 0) == pdTRUE) enviarResultado(client, "queued", cmd.id, command_name);
            else enviarResultado(client, "rejected", cmd.id, command_name, "command_queue_full");
        } else {
            enviarResultado(client, "rejected", cmd.id, command_name, "busy");
        }
        return;
    }

    if (cmd.tipo == CMD_TEST_PWM) {
        if (estadoActual == IDLE) {
            if (xQueueSend(colaComandos, &cmd, 0) == pdTRUE) enviarResultado(client, "queued", cmd.id, command_name);
            else enviarResultado(client, "rejected", cmd.id, command_name, "command_queue_full");
        } else {
            enviarResultado(client, "rejected", cmd.id, command_name, "busy");
        }
        return;
    }
  }
}

void pushTelemetria() {
  if (ws.count() == 0) return;

  SensorSnapshot snap = {};
  if (xQueuePeek(colaSensores, &snap, 0) != pdTRUE) return;

  static char robot_id[24] = {};
  if (!robot_id[0]) {
      const unsigned long long chip_id = ESP.getEfuseMac();
      snprintf(robot_id, sizeof(robot_id), "%s-%012llX", ROBOT_ID_PREFIX, chip_id);
  }

  StaticJsonDocument<1024> doc;
  doc["v"] = 1;
  doc["type"] = "telemetry";
  doc["seq"] = snap.sequence;
  JsonObject payload = doc.createNestedObject("payload");
  payload["uptime_ms"] = millis();
  payload["robot_id"] = robot_id;
  payload["firmware_version"] = FIRMWARE_VERSION;
  payload["x_mm"] = PoseGlobal.getX() * 10.0f;
  payload["y_mm"] = PoseGlobal.getY() * 10.0f;
  payload["yaw_deg"] = PoseGlobal.getThetaDeg();
  payload["state"] = estadoTexto();
  payload["pfl"] = snap.pulsosFL;
  payload["pfr"] = snap.pulsosFR;
  payload["pbl"] = snap.pulsosBL;
  payload["pbr"] = snap.pulsosBR;
  payload["pwm_l"] = pwm_aplicado_L;
  payload["pwm_r"] = pwm_aplicado_R;
  payload["mpu_present"] = snap.mpu_present;
  payload["mpu_stale"] = snap.mpu_stale;
  payload["mpu_calibrated"] = snap.mpu_calibrated;
  payload["i2c_ok"] = snap.mpu_present && snap.mpu_calibrated && !snap.mpu_stale;
  payload["gyro_z_filtered_rad_s"] = snap.gyro_z_filtrado_rad_s;
  payload["gyro_z_offset_rad_s"] = snap.gyro_z_offset_rad_s;
  payload["theta_error_deg"] = THETA_ERROR_DEG;
  JsonObject encoderAvg = payload.createNestedObject("encoder_delta_avg");
  encoderAvg["fl"] = snap.delta_pulsos_filtrado_FL;
  encoderAvg["fr"] = snap.delta_pulsos_filtrado_FR;
  encoderAvg["bl"] = snap.delta_pulsos_filtrado_BL;
  encoderAvg["br"] = snap.delta_pulsos_filtrado_BR;
  JsonObject wheelSpeed = payload.createNestedObject("wheel_speed_cm_s");
  wheelSpeed["left"] = snap.velocidad_filtrada_L_cm_s;
  wheelSpeed["right"] = snap.velocidad_filtrada_R_cm_s;
  JsonObject pins = payload.createNestedObject("pin_state");
  pins["motor_left_active"] = pwm_aplicado_L != 0;
  pins["motor_right_active"] = pwm_aplicado_R != 0;
  pins["encoder_fl_seen"] = snap.pulsosFL != 0;
  pins["encoder_fr_seen"] = snap.pulsosFR != 0;
  pins["encoder_bl_seen"] = snap.pulsosBL != 0;
  pins["encoder_br_seen"] = snap.pulsosBR != 0;
  pins["i2c_sda"] = PIN_I2C_SDA;
  pins["i2c_scl"] = PIN_I2C_SCL;
  char buf[1024];
  const size_t written = serializeJson(doc, buf, sizeof(buf));
  if (written == 0 || written >= sizeof(buf) - 1) return;
  ws.textAll(buf);
}

void procesarWebSockets() {
  ws.cleanupClients();
  procesarEventosRed();
}

void setup_Red() {
  LOG_CORE("Iniciando configuracion de Red...");
  WiFi.softAP(ssid_AP, password_AP, 1, 0, 1);
  Serial.println("\nWiFi creado");
  Serial.print("IP Robot: ");
  Serial.println(WiFi.softAPIP());

  if (!LittleFS.begin(true)) {
    Serial.println("ERROR: LittleFS mount failed");
  } else {
    Serial.println("LittleFS montado correctamente");
  }

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (LittleFS.exists("/Index.html.gz")) {
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/Index.html.gz", "text/html");
      response->addHeader("Content-Encoding", "gzip");
      response->addHeader("Cache-Control", "max-age=3600");
      request->send(response);
    } else if (LittleFS.exists("/Index.html")) {
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/Index.html", "text/html");
      response->addHeader("Cache-Control", "max-age=3600");
      request->send(response);
    } else {
      request->send(500, "text/plain", "ERROR 500: Archivos Web no encontrados.");
    }
  });

  server.on("/beta", HTTP_GET, [](AsyncWebServerRequest *request){
    if (LittleFS.exists("/IndexBeta.html.gz")) {
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/IndexBeta.html.gz", "text/html");
      response->addHeader("Content-Encoding", "gzip");
      response->addHeader("Cache-Control", "max-age=3600");
      request->send(response);
    } else {
      request->send(404, "text/plain", "Beta UI not found");
    }
  });

  server.serveStatic("/", LittleFS, "/");
  server.begin();
  Serial.println("Servidor Web iniciado");
}

TaskHandle_t TaskMotorHandle;
TaskHandle_t TaskWebHandle;
TaskHandle_t TaskSensorsHandle;

QueueHandle_t colaComandos;
QueueHandle_t buzonManual;
volatile bool flag_ESTOP_ISR = false;
static char active_test_id[33] = {};

void procesarComandos() {
    if (flag_ESTOP_ISR) {
        flag_ESTOP_ISR = false;
        WatchdogSeguridad.forzarEStop();
        xQueueReset(colaComandos);
        xQueueReset(buzonManual);
        return;
    }

    ComandoRed cmd;

    if (estadoActual == MANUAL || estadoActual == IDLE) {
        if (xQueueReceive(buzonManual, &cmd, 0) == pdTRUE) {
            if (cmd.tipo == CMD_MANUAL && estadoActual != FAULT_SENSOR && estadoActual != ESTOP_LATCHED) {
                if (cmd.pwm_l == 0 && cmd.pwm_r == 0) {
                    frenarMotores();
                    estadoActual = IDLE;
                    return;
                }
                estadoActual = MANUAL;
                WatchdogSeguridad.alimentarWatchdogRed();
                aplicarVelocidades(cmd.pwm_l, cmd.pwm_r);
            }
        }
    }

    if (xQueueReceive(colaComandos, &cmd, 0) == pdTRUE) {
        if (cmd.tipo == CMD_STOP) {
            frenarMotores();
            estadoActual = IDLE;
            xQueueReset(colaComandos);
            encolarEvento(EVT_COMPLETED, cmd.id, "stop_ok");
        }
        else if (cmd.tipo == CMD_CLEAR_FAULT) {
            WatchdogSeguridad.resetFallo();
        }
        else if (cmd.tipo == CMD_RESET_POSE && estadoActual == IDLE) {
            PoseGlobal.reset();
            resetOrientacionIMU();
        }
        else if (cmd.tipo == CMD_CLEAR_ROUTE) {
            frenarMotores();
            estadoActual = IDLE;
            xQueueReset(colaComandos);
        }
        else if (estadoActual == FAULT_SENSOR || estadoActual == ESTOP_LATCHED) {
            LOG_CORE("Comando ignorado por estado de FALLO.");
        }
        else if (cmd.tipo == CMD_MOVE_ABS && estadoActual == IDLE) {
            if (!calcularDestino(cmd.arg_x, cmd.arg_y, cmd.id)) {
                encolarEvento(EVT_REJECTED, cmd.id, "distance_limit");
            }
        }
        else if (cmd.tipo == CMD_TEST_PWM && estadoActual == IDLE) {
            int safeL = constrain(cmd.pwm_l, -120, 120);
            int safeR = constrain(cmd.pwm_r, -120, 120);
            unsigned long dur = cmd.tiempo_ms;
            if (dur == 0) dur = 500;
            if (dur > 1500) dur = 1500;
            estadoActual = TESTING;
            strncpy(active_test_id, cmd.id, sizeof(active_test_id) - 1);
            active_test_id[sizeof(active_test_id) - 1] = '\0';
            pwm_target_l = safeL;
            pwm_target_r = safeR;
            tiempoTestingInicio = millis();
            duracionTesting = dur;
            WatchdogSeguridad.alimentarWatchdogRed();
            aplicarVelocidades(pwm_target_l, pwm_target_r);
        }
    }
}

void Task_Web(void *pvParameters) {
    LOG_CORE("Task_Web iniciada en Core 0");
    for (;;) {
        procesarWebSockets();
        LOG_THROTTLED("Heartbeat Task Web", 20);
        static int _m = 0; if (++_m >= 20) { LOG_MEMORY(); _m = 0; }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void Task_Motor(void *pvParameters) {
    LOG_CORE("Task_Motor iniciada en Core 1 (prio 10)");
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrecuencia = 10 / portTICK_PERIOD_MS;
    static unsigned long lastTelemetry = 0;
    uint32_t lastSensorSequence = 0;

    for (;;) {
        LOG_THROTTLED("Heartbeat Task Motor (100Hz OK)", 100);

        procesarComandos();

        SensorSnapshot snap;
        bool snap_fresh = (xQueuePeek(colaSensores, &snap, 0) == pdTRUE) && snap.sequence != lastSensorSequence;

        if (snap_fresh) {
            lastSensorSequence = snap.sequence;
            PoseGlobal.actualizarOdometria(snap.pulsosFL, snap.pulsosFR, snap.pulsosBL, snap.pulsosBR, estadoActual);

            if (snap.mpu_present && !snap.mpu_stale) {
                PoseGlobal.actualizarOrientacion(snap.imu_deltaZ_rad);
            } else if (!snap.mpu_present) {
                if (estadoActual == AVANZANDO || estadoActual == GIRANDO) {
                    frenarMotores();
                    estadoActual = FAULT_SENSOR;
                    LOG_CORE("FAULT: MPU6050 ausente durante navegacion.");
                    encolarEvento(EVT_FAULT, "", "mpu_lost");
                }
            }

            WatchdogSeguridad.auditarSalud(snap, pwm_aplicado_L, pwm_aplicado_R);
        }

        if (estadoActual == GIRANDO) {
            controlarGiro();
        } else if (estadoActual == ESPERANDO_ESTABILIZACION) {
            if (millis() - tiempoEspera >= ESTABILIZACION_MS) {
                anguloZ = 0;
                tiempoAnterior = millis();
                tiempoInicioAvance = millis();
                estadoActual = AVANZANDO;
            }
        } else if (estadoActual == AVANZANDO) {
            controlarAvance();
        } else if (estadoActual == TESTING) {
            if (millis() - tiempoTestingInicio > duracionTesting) {
                frenarMotores();
                estadoActual = IDLE;
                encolarEvento(EVT_COMPLETED, active_test_id, "test_done");
                active_test_id[0] = '\0';
            } else {
                aplicarVelocidades(pwm_target_l, pwm_target_r);
            }
        }

        if (millis() - lastTelemetry >= 100) {
            lastTelemetry = millis();
            pushTelemetria();
        }

        vTaskDelayUntil(&xLastWakeTime, xFrecuencia);
    }
}

void setup() {
    Serial.begin(115200);

    colaComandos = xQueueCreate(10, sizeof(ComandoRed));
    buzonManual = xQueueCreate(1, sizeof(ComandoRed));
    colaEventosRed = xQueueCreate(8, sizeof(EventoRed));

    PoseGlobal.inicializar(WHEEL_DIAMETER_CM, ENCODER_PPR);

    setup_Red();
    setup_Motores();
    setup_Sensores();

    Serial.println("Sistema listo. Iniciando FreeRTOS...");

    xTaskCreatePinnedToCore(Task_Web, "Red", 8192, NULL, 1, &TaskWebHandle, 0);
    xTaskCreatePinnedToCore(Task_Sensors, "Sensores", 4096, NULL, 8, &TaskSensorsHandle, 1);
    xTaskCreatePinnedToCore(Task_Motor, "Motores", 8192, NULL, 10, &TaskMotorHandle, 1);

    vTaskDelete(NULL);
}

void loop() {
}
