#pragma once
#include <Arduino.h>

constexpr char FIRMWARE_VERSION[] = "robot-s3-v2";
constexpr char ROBOT_ID_PREFIX[] = "ESP32S3";
constexpr char PROTOCOL_NAME[] = "robot-s3-steps-v2";

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
// Cableado físico actual del robot: SDA=GPIO8 y SCL=GPIO9.
// No confundirlos con RX/TX, que son pines UART distintos.
const int PIN_I2C_SDA = 8;
const int PIN_I2C_SCL = 9;

// Muestreo y filtros. PCNT conserva los pulsos acumulados sin filtrar; el
// promedio de 10 ventanas de 10 ms se usa solamente para estimar velocidad.
constexpr uint32_t SENSOR_PERIOD_MS = 10;
constexpr size_t ENCODER_AVG_WINDOW = 10;
constexpr size_t IMU_GYRO_AVG_WINDOW = 8;
constexpr uint16_t IMU_CALIBRATION_SAMPLES = 256;
constexpr float IMU_GYRO_DEADBAND_RAD_S = 0.005f;

constexpr float WHEEL_DIAMETER_CM = 6.6f;
constexpr float WHEEL_DIAMETER_ODOMETRY_CM = WHEEL_DIAMETER_CM;
constexpr int ENCODER_PPR = 20;
constexpr float MPU_YAW_POLARITY = -1.0f;
constexpr float YAW_RECENTER_THRESHOLD_DEG = 720.0f;

// Calibración y PID
constexpr uint8_t PWM_RESOLUTION_BITS = 10;
constexpr int PWM_MAX = (1 << PWM_RESOLUTION_BITS) - 1;
constexpr float PWM_SCALE_8_TO_10 = static_cast<float>(PWM_MAX) / 255.0f;
constexpr int PWM_FORWARD_POLARITY = -1;
constexpr int PWM_SAFE_HARD_LIMIT = static_cast<int>(230 * PWM_SCALE_8_TO_10); // Límite global seguro 90%

// --- PARÁMETROS DE AVANCE RECTO (DRIVE) ---
constexpr int VELOCIDAD_BASE_RECTO = PWM_SAFE_HARD_LIMIT;
constexpr int VELOCIDAD_APROXIMACION = static_cast<int>(180 * PWM_SCALE_8_TO_10);
constexpr int VELOCIDAD_MINIMA_RECTO = static_cast<int>(140 * PWM_SCALE_8_TO_10);
constexpr float TOLERANCIA_DISTANCIA_CM = 3.0f;
constexpr float GYRO_MOVEMENT_RAD_S = 0.12f;
constexpr uint32_t DRIVE_STALL_MS = 6000;
constexpr uint32_t DRIVE_BASE_TIMEOUT_MS = 15000;
constexpr uint32_t DRIVE_TIMEOUT_PER_CM_MS = 400;
constexpr float ERROR_RUMBO_RECUPERAR_DEG = 20.0f;
constexpr uint32_t ERROR_RUMBO_RECUPERAR_MS = 300;
constexpr float GIRO_RECUPERACION_MAX_DEG = 25.0f;
constexpr uint8_t INTENTOS_RECUPERACION_MAX = 20;
constexpr uint32_t PAUSA_ENTRE_PASOS_MS = 600;

// --- PARÁMETROS DE GIRO PIVOTE (TURN) ---
constexpr uint32_t TURN_CONTROL_PERIOD_MS = 20;
constexpr int PWM_TURN_MAX_LIMIT = PWM_SAFE_HARD_LIMIT; // Límite seguro global para giros (230/255 = ~90%)
constexpr int PWM_TURN_START = static_cast<int>(130 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_FAR_MARGIN = static_cast<int>(10 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_NEAR_MARGIN = static_cast<int>(4 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_SLEW_STEP = static_cast<int>(2 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_START_SLEW_STEP = static_cast<int>(2 * PWM_SCALE_8_TO_10);
constexpr float TOLERANCIA_GIRO_DEG = 3.0f;
constexpr float TOLERANCIA_CALIBRACION_DEG = 1.0f;
constexpr float TURN_BRAKING_ZONE_DEG = 25.0f;
constexpr float TURN_HYBRID_THRESHOLD_DEG = 5.0f;
constexpr uint32_t TURN_STALL_FINE_MS = 600;
constexpr uint32_t TURN_RAMP_ADAPTIVE_INTERVAL_MS = 150;
constexpr uint32_t TURN_PULSE_ON_MS = 50;
constexpr uint32_t TURN_PULSE_OFF_MS = 100;
constexpr float TURN_REACTIVATION_DEG = 4.0f;
constexpr uint8_t TURN_MAX_ATTEMPTS = 15;
constexpr uint32_t TURN_RETRY_PAUSE_MS = 1500;
constexpr uint32_t TURN_SETTLE_MS = 300;
constexpr uint32_t TURN_STALL_MS = 4000;
constexpr uint32_t TURN_TIMEOUT_MS = 60000;
constexpr uint32_t TURN_ATTEMPT_TIMEOUT_MS = 15000;

// PID y Correcciones en marcha
constexpr int PWM_CALIBRATION_MARGIN = static_cast<int>(8 * PWM_SCALE_8_TO_10);
constexpr int PWM_CORRECCION_RUMBO_MAX = static_cast<int>(80 * PWM_SCALE_8_TO_10);
constexpr int PWM_CORRECCION_ENCODER_MAX = static_cast<int>(15 * PWM_SCALE_8_TO_10);
constexpr float KP_RUMBO_PWM_POR_GRADO = 4.0f * PWM_SCALE_8_TO_10;
constexpr float KD_RUMBO_PWM_POR_RAD_S = 12.0f * PWM_SCALE_8_TO_10;
constexpr float KP_ENCODER_PWM_POR_TICK = 1.5f * PWM_SCALE_8_TO_10;

// Calibración y rampa incremental por búsqueda continua de torque (140 a 230/255).
constexpr uint32_t CUENTA_CALIBRACION_MS = 5000;
constexpr uint32_t PAUSA_CALIBRACION_MS = 1000;
constexpr uint32_t PAUSA_RETORNO_CAL_MS = 2500;
constexpr int      CALIBRATION_PWM_START = static_cast<int>(140 * PWM_SCALE_8_TO_10);
constexpr int      CALIBRATION_PWM_END   = PWM_SAFE_HARD_LIMIT;
constexpr int      CALIBRATION_PWM_STEP  = static_cast<int>(13 * PWM_SCALE_8_TO_10); // ~5% de incremento por paso
constexpr uint32_t CAL_RAMP_INTERVAL_MS = 500; // 500 ms de delay por paso de rampa
constexpr uint32_t CAL_MOVE_SUSTAINED_MS = 100;
constexpr int64_t  CAL_TICKS_MOVIMIENTO = 2;
constexpr uint8_t  CAL_MAX_ATTEMPTS = 7;
constexpr uint32_t CAL_RETRY_PAUSE_MS = 750;
constexpr uint32_t CAL_MAX_PWM_STALL_MS = 1000;
constexpr float DESACUERDO_MAXIMO_PAR = 0.25f;
constexpr uint32_t DESACUERDO_ENCODER_PERSISTENTE_MS = 500;
constexpr uint32_t PAUSA_REEVALUACION_MS = 500;
constexpr int64_t TICKS_MINIMOS_AUDITORIA = 20;
constexpr float ERROR_MAX_CLASIFICAR_DEG = 3.0f;

// Timeouts del Watchdog de Seguridad por fase (Seguridad.cpp)
constexpr uint32_t TIMEOUT_STALL_CALIBRANDO_MS = 20000; // 20 segundos para calibración
constexpr uint32_t TIMEOUT_STALL_GIRO_MS       = 12000; // 12 segundos para maniobras de giro
constexpr uint32_t TIMEOUT_STALL_AVANCE_MS     = 7000;  // 7 segundos para avance recto

// Tiempo muerto universal al invertir sentido (Motores.cpp).
constexpr int PWM_DIRECTION_PAUSE_MS = 250;

// Límites de validación de comandos.
constexpr float STEP_MAX_DISTANCE_CM = 200.0f;
constexpr float COMP_FACTOR_MIN = 0.80f;
constexpr float COMP_FACTOR_MAX = 1.00f;

// Diagnóstico RTOS
constexpr uint32_t RTOS_STACK_MIN_ACCEPTABLE_BYTES = 1024;
constexpr uint32_t CONTROL_LOOP_PERIOD_US = 10000;
constexpr int CONTROL_LOOP_PRIORITY = 10;
