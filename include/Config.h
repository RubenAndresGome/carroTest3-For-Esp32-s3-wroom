#pragma once
#include <Arduino.h>

constexpr char FIRMWARE_VERSION[] = "robot-s3-v3";
constexpr char ROBOT_ID_PREFIX[] = "ESP32S3";
constexpr char PROTOCOL_NAME[] = "robot-s3-steps-v3";

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
// Corrección firmada de la distancia por pulso. La fórmula es la solicitada:
// constante efectiva = constante nominal + porcentaje * constante nominal.
// Calibración provisional de suelo: los recorridos físicos reportaron un
// sobreavance de 10 % en 1 m y de 15--20 % en 2 m. Al aumentar la distancia
// estimada por tick, el restante se reduce antes y el robot ordena el freno
// antes. Revalidar con tres corridas de 50 y 200 cm tras cada cambio mecánico.
constexpr float ENCODER_ERROR_PORCENTAJE = 0.15f;
constexpr float FACTOR_ESCALA_ENCODER = 1.0f + ENCODER_ERROR_PORCENTAJE;
static_assert(FACTOR_ESCALA_ENCODER > 0.0f,
              "La correccion del encoder debe conservar una distancia por pulso positiva.");
constexpr float WHEEL_DIAMETER_ODOMETRY_CM = WHEEL_DIAMETER_CM * FACTOR_ESCALA_ENCODER;
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
// En pruebas de suelo, un torque inicial cercano a 65 % evita que el avance
// fino quede zumbando por debajo de la fricción estática. Sigue por debajo del
// tope seguro de avance (230/255).
constexpr int VELOCIDAD_APROXIMACION = static_cast<int>(200 * PWM_SCALE_8_TO_10);
constexpr int VELOCIDAD_MINIMA_RECTO = static_cast<int>(165 * PWM_SCALE_8_TO_10);
// La reversa no entra directamente a crucero: después del interlock se rampa
// desde el torque mínimo para que el PID confirme yaw antes de potencia plena.
constexpr uint32_t RAMPA_REVERSA_MS = 900;
// Dentro del cierre se reduce el torque sin debilitar el arranque de crucero.
// Esto evita que un paso corto llegue al umbral con 65 % de PWM todavía activo.
constexpr int VELOCIDAD_PRECISION_RECTO = static_cast<int>(150 * PWM_SCALE_8_TO_10);
constexpr float TOLERANCIA_DISTANCIA_CM = 3.0f;
constexpr float DISTANCIA_APROXIMACION_CM = 40.0f;
// Modelo inicial de avance por inercia. La fase de asentamiento publica el
// resultado real por JSON para afinar estos valores con pruebas de piso.
constexpr float FRENO_RESIDUAL_BASE_CM = 1.5f;
constexpr float FRENO_RESIDUAL_POR_PWM_CM = 0.006f;
constexpr float FRENO_RESIDUAL_MAX_CM = 8.0f;
constexpr uint32_t ASENTAMIENTO_MIN_MS = 250;
constexpr uint32_t ASENTAMIENTO_SIN_PULSOS_MS = 300;
constexpr uint32_t ASENTAMIENTO_MAX_MS = 1500;
// Una misión con objetivo absoluto no puede terminar únicamente por el
// contador longitudinal: debe converger al punto planificado.
constexpr float TOLERANCIA_ENDPOINT_CM = 5.0f;
// Por debajo de este umbral el arrastre de frenado puede ser mayor que la
// corrección. No se gira ni se avanza automáticamente: se reporta calibración.
constexpr float DISTANCIA_MINIMA_RECUPERACION_ENDPOINT_CM =
    TOLERANCIA_ENDPOINT_CM + FRENO_RESIDUAL_MAX_CM;
constexpr uint8_t INTENTOS_RECUPERACION_ENDPOINT_MAX = 2;
constexpr float GYRO_MOVEMENT_RAD_S = 0.12f;
constexpr uint32_t DRIVE_STALL_MS = 6000;
constexpr uint32_t DRIVE_BASE_TIMEOUT_MS = 15000;
constexpr uint32_t DRIVE_TIMEOUT_PER_CM_MS = 400;
// La corrección PID continua absorbe el desvío moderado. El pivote se arma
// sólo ante una pérdida clara y sostenida de rumbo, evitando ciclos de
// avance/giro durante segmentos cortos.
constexpr float ERROR_RUMBO_RECUPERAR_DEG = 15.0f;
constexpr uint32_t ERROR_RUMBO_RECUPERAR_MS = 600;
constexpr float GIRO_RECUPERACION_MAX_DEG = 25.0f;
// Un error persistente debe terminar en parada segura; no mantener PWM en un
// ciclo de recuperación indefinido si el robot no responde.
constexpr uint8_t INTENTOS_RECUPERACION_MAX = 3;
constexpr uint32_t PAUSA_ENTRE_PASOS_MS = 600;

// --- PARÁMETROS DE GIRO PIVOTE (TURN) ---
constexpr uint32_t TURN_CONTROL_PERIOD_MS = 20;
constexpr int PWM_TURN_MAX_LIMIT = static_cast<int>(247 * PWM_SCALE_8_TO_10); // Límite seguro para giros (247/255 = ~97%)
// Giro inicial a ~65 % para vencer fricción. La rampa posterior conserva el
// límite de 247/255 y el interlock universal al invertir polaridad.
constexpr int PWM_TURN_START = static_cast<int>(165 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_FAR_MARGIN = static_cast<int>(10 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_NEAR_MARGIN = static_cast<int>(4 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_SLEW_STEP = static_cast<int>(2 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_START_SLEW_STEP = static_cast<int>(2 * PWM_SCALE_8_TO_10);
constexpr float TOLERANCIA_GIRO_DEG = 3.0f;
constexpr float TOLERANCIA_CALIBRACION_DEG = 1.0f;
constexpr float TURN_BRAKING_ZONE_DEG = 25.0f;
constexpr float TURN_HYBRID_THRESHOLD_DEG = 5.0f;
constexpr uint32_t TURN_RAMP_ADAPTIVE_INTERVAL_MS = 150;
constexpr float TURN_REACTIVATION_DEG = 4.0f;
constexpr uint8_t TURN_MAX_ATTEMPTS = 15;
constexpr uint32_t TURN_RETRY_PAUSE_MS = 1500;
// Exigir medio segundo largo de yaw estable antes de abandonar el giro y
// volver al avance; evita reanudar cuando el chasis aún está asentándose.
constexpr uint32_t TURN_SETTLE_MS = 600;
constexpr uint32_t TURN_STALL_MS = 4000;
constexpr uint32_t TURN_TIMEOUT_MS = 60000;
constexpr uint32_t TURN_ATTEMPT_TIMEOUT_MS = 15000;

// PID y Correcciones en marcha
constexpr int PWM_CALIBRATION_MARGIN = static_cast<int>(8 * PWM_SCALE_8_TO_10);
constexpr int PWM_CORRECCION_RUMBO_MAX = static_cast<int>(80 * PWM_SCALE_8_TO_10);
constexpr int PWM_CORRECCION_ENCODER_MAX = static_cast<int>(15 * PWM_SCALE_8_TO_10);
constexpr float KP_RUMBO_PWM_POR_GRADO = 4.0f * PWM_SCALE_8_TO_10;
constexpr float KI_RUMBO_PWM_POR_GRADO_S = 0.35f * PWM_SCALE_8_TO_10;
constexpr float KD_RUMBO_PWM_POR_RAD_S = 12.0f * PWM_SCALE_8_TO_10;
constexpr float KP_ENCODER_PWM_POR_TICK = 1.5f * PWM_SCALE_8_TO_10;
constexpr float ERROR_INTEGRAL_RUMBO_MAX_GRADO_S = 35.0f;
constexpr float ERROR_ENCODER_AUX_MAX_DEG = 8.0f;
constexpr float KP_LATERAL_RUMBO_DEG_POR_CM = 0.8f;
constexpr float CORRECCION_LATERAL_RUMBO_MAX_DEG = 8.0f;
constexpr float UMBRAL_REVERSA_AUTOMATICA_DEG = 135.0f;

// Calibración y rampa incremental por búsqueda continua de torque (140 a 230/255).
constexpr uint32_t CUENTA_CALIBRACION_MS = 5000;
constexpr uint32_t PAUSA_CALIBRACION_MS = 1000;
constexpr uint32_t PAUSA_RETORNO_CAL_MS = 2500;
constexpr int      CALIBRATION_PWM_START = static_cast<int>(140 * PWM_SCALE_8_TO_10);
constexpr int      CALIBRATION_PWM_END   = PWM_TURN_MAX_LIMIT;
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
constexpr float STEP_TARGET_MAX_ABS_CM = 10000.0f;
constexpr float COMP_FACTOR_MIN = 0.80f;
constexpr float COMP_FACTOR_MAX = 1.00f;

// Diagnóstico RTOS
constexpr uint32_t RTOS_STACK_MIN_ACCEPTABLE_BYTES = 1024;
constexpr uint32_t CONTROL_LOOP_PERIOD_US = 10000;
constexpr int CONTROL_LOOP_PRIORITY = 10;
