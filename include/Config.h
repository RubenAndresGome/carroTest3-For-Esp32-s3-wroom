#pragma once
#include <Arduino.h>

constexpr char FIRMWARE_VERSION[] = "robot-s3-v3.7";
constexpr char ROBOT_ID_PREFIX[] = "ESP32S3";
constexpr char PROTOCOL_NAME[] = "robot-s3-steps-v3";
constexpr uint32_t MANUAL_LEASE_MS = 300;
constexpr uint32_t MANUAL_FIRST_FRAME_GRACE_MS = 500;
constexpr uint32_t MANUAL_STALL_TIMEOUT_MS = 450;

// WiFi
extern const char* ssid_AP;
extern const char* password_AP;

// Motores (DRV8833)
// FL/BL: izquierdo superior/inferior. FR/BR: derecho superior/inferior.
// Asignados segun polaridad fisica OUT (+) naranja / (-) negro en borneras:
const int PIN_FL_FWD = 7;   // IN3 -> OUT3 (naranja +)
const int PIN_FL_REV = 6;   // IN4 -> OUT4 (negro -)
const int PIN_BL_FWD = 4;   // IN2 -> OUT2 (naranja +)
const int PIN_BL_REV = 5;   // IN1 -> OUT1 (rojo/negro -)
const int PIN_FR_FWD = 18;  // IN4 -> OUT4 (naranja +)
const int PIN_FR_REV = 17;  // IN3 -> OUT3 (negro -)
const int PIN_BR_FWD = 16;  // IN2 -> OUT2 (naranja +)
const int PIN_BR_REV = 15;  // IN1 -> OUT1 (negro -)

// Encoders (Cableado fisico real verificado por hardware)
const int PIN_ENC_FL = 11;  // superior izquierdo, cable verde (TXS B5->A5)
const int PIN_ENC_FR = 10;  // superior derecho, cable blanco (TXS B6->A6)
const int PIN_ENC_BL = 12;  // inferior izquierdo, cable negro (TXS B2->A2)
const int PIN_ENC_BR = 13;  // inferior derecho, cable rojo (TXS B1->A1)

// MPU6050 (I2C)
// Cableado físico actual del robot: SDA=GPIO8 y SCL=GPIO9.
// No confundirlos con RX/TX, que son pines UART distintos.
const int PIN_I2C_SDA = 8;
const int PIN_I2C_SCL = 9;

// Muestreo y filtros. PCNT conserva los pulsos acumulados; antes de sumarlos
// se descarta cualquier salto físicamente imposible para impedir que una
// lectura corrupta contamine pose, distancia y las misiones siguientes. El
// promedio de 10 ventanas de 10 ms se usa solamente para estimar velocidad.
constexpr uint32_t SENSOR_PERIOD_MS = 10;
constexpr size_t ENCODER_AVG_WINDOW = 10;
// Con la rueda efectiva actual, 64 pulsos en 10 ms equivaldrían a más de
// 70 m/s. Es una barrera contra corrupción PCNT, no un filtro de movimiento.
constexpr int64_t ENCODER_MAX_PULSES_PER_SAMPLE = 64;
constexpr size_t IMU_GYRO_AVG_WINDOW = 8;
constexpr uint16_t IMU_CALIBRATION_SAMPLES = 256;
constexpr float IMU_GYRO_DEADBAND_RAD_S = 0.005f;

constexpr float WHEEL_DIAMETER_CM = 6.6f;
// Factor de corrección de distancia por pulso calibrado con telemetría de suelo real:
// A 228 ticks con factor 0.842 el chasis recorrió 120 cm reales (sobrepaso de 20 cm medido en cinta).
// Relación física real: 120 cm / 228 ticks = 0.5263 cm/tick.
// Para 100 cm exactos se requieren 190 ticks (factor 1.015f, corrección de +1.5% sobre diámetro nominal 6.6 cm).
constexpr float ENCODER_ERROR_PORCENTAJE = 0.015f;
constexpr float FACTOR_ESCALA_ENCODER = 1.0f + ENCODER_ERROR_PORCENTAJE;
static_assert(FACTOR_ESCALA_ENCODER > 0.0f,
              "La correccion del encoder debe conservar una distancia por pulso positiva.");
constexpr float FACTOR_ESCALA_ODOMETRIA_SUELO = 1.0f;
static_assert(FACTOR_ESCALA_ODOMETRIA_SUELO > 0.0f && FACTOR_ESCALA_ODOMETRIA_SUELO <= 1.0f,
              "La escala de suelo debe ser positiva y no amplificar la odometria teorica.");
constexpr float WHEEL_DIAMETER_ODOMETRY_CM =
    WHEEL_DIAMETER_CM * FACTOR_ESCALA_ENCODER * FACTOR_ESCALA_ODOMETRIA_SUELO;
// Conteo de ambos flancos: 20 ranuras x 2 transiciones = 40 ticks/vuelta (ENCODER_PPR = 40).
// Compatible con comparador LM393 + level shifter TXS0108E verificado en suelo.
constexpr int ENCODER_PPR = 40;
constexpr float MPU_YAW_POLARITY = -1.0f;
constexpr float YAW_RECENTER_THRESHOLD_DEG = 720.0f;

// Calibración y PID
constexpr uint8_t PWM_RESOLUTION_BITS = 10;
constexpr int PWM_MAX = (1 << PWM_RESOLUTION_BITS) - 1;
constexpr float PWM_SCALE_8_TO_10 = static_cast<float>(PWM_MAX) / 255.0f;
constexpr int PWM_FORWARD_POLARITY = -1;
constexpr int PWM_POLARITY_FL = PWM_FORWARD_POLARITY;
constexpr int PWM_POLARITY_FR = PWM_FORWARD_POLARITY;
constexpr int PWM_POLARITY_BL = PWM_FORWARD_POLARITY;
constexpr int PWM_POLARITY_BR = PWM_FORWARD_POLARITY;
static_assert((PWM_POLARITY_FL == 1 || PWM_POLARITY_FL == -1) &&
              (PWM_POLARITY_FR == 1 || PWM_POLARITY_FR == -1) &&
              (PWM_POLARITY_BL == 1 || PWM_POLARITY_BL == -1) &&
              (PWM_POLARITY_BR == 1 || PWM_POLARITY_BR == -1), "Polaridad de motor invalida");
constexpr int PWM_MANUAL_MAX_LIMIT = static_cast<int>(230 * PWM_SCALE_8_TO_10);
constexpr int PWM_MANUAL_RAMP_STEP = static_cast<int>(8 * PWM_SCALE_8_TO_10);
constexpr int PWM_SAFE_HARD_LIMIT = static_cast<int>(242 * PWM_SCALE_8_TO_10); // Límite de avance 94.9%

// --- NIVELES DISCRETOS DE CRUCERO ADAPTATIVO (70% - 85% - 90% - 100%) ---
constexpr int PWM_CRUCERO_70 = static_cast<int>(179 * PWM_SCALE_8_TO_10);  // 70.2% de 255 (~35-50 ticks/s objetivo)
constexpr int PWM_CRUCERO_80 = static_cast<int>(204 * PWM_SCALE_8_TO_10);  // 80.0% de 255
constexpr int PWM_CRUCERO_85 = static_cast<int>(217 * PWM_SCALE_8_TO_10);  // 85.1% de 255 (crucero nominal con torque firme)
constexpr int PWM_CRUCERO_90 = static_cast<int>(230 * PWM_SCALE_8_TO_10);  // 90.2% de 255
constexpr int PWM_CRUCERO_100 = PWM_SAFE_HARD_LIMIT;                        // 94.9% de 255 (242)

// --- PARÁMETROS DE AVANCE RECTO (DRIVE) ---
constexpr int VELOCIDAD_BASE_RECTO = PWM_CRUCERO_85; // Crucero nominal 85% PWM para evitar atascos y pérdida de tracción
constexpr int VELOCIDAD_APROXIMACION = static_cast<int>(200 * PWM_SCALE_8_TO_10);
constexpr int VELOCIDAD_MINIMA_RECTO = static_cast<int>(175 * PWM_SCALE_8_TO_10);
// La reversa no entra directamente a crucero: después del interlock se rampa
// desde el torque mínimo para que el PID confirme yaw antes de potencia plena.
constexpr uint32_t RAMPA_REVERSA_MS = 900;
// Dentro del cierre se reduce el torque sin debilitar el arranque de crucero.
constexpr int VELOCIDAD_PRECISION_RECTO = static_cast<int>(180 * PWM_SCALE_8_TO_10);
// Piso inferior para reducción diferencial en avance recto (garantiza tracción continua de ruedas internas sin bloqueo)
constexpr int VELOCIDAD_MINIMA_DIFERENCIAL = static_cast<int>(165 * PWM_SCALE_8_TO_10);
constexpr float TOLERANCIA_DISTANCIA_CM = 1.0f;
constexpr float DISTANCIA_APROXIMACION_CM = 15.0f;          // Zona de desaceleración acotada para sostener 85% de crucero hasta 85 cm
constexpr float DISTANCIA_APROXIMACION_REVERSA_CM = 18.0f;
constexpr float DISTANCIA_MICRO_PULSOS_CM = 3.0f;
constexpr uint32_t APPROACH_PULSE_ON_MS = 250;
constexpr uint32_t APPROACH_PULSE_OFF_MS = 120;
constexpr int APPROACH_PULSE_PWM = static_cast<int>(180 * PWM_SCALE_8_TO_10);
constexpr uint32_t DURACION_FRENO_ACTIVO_MS = 150; // Pulso activo seguro en DRV8833 (IN1=1, IN2=1) antes de reposo LOW

// --- DIMENSIONES FÍSICAS DEL CHASIS 4WD ---
constexpr float CHASSIS_LENGTH_CM = 28.0f;
constexpr float CHASSIS_WIDTH_CM = 17.0f;
constexpr float CHASSIS_HALF_LENGTH_CM = 14.0f;

// Modelo de avance por inercia ajustado con telemetría real (arrastre real medio ~1.2 cm por reducción TT).
constexpr float FRENO_RESIDUAL_BASE_CM = 0.3f;
constexpr float FRENO_RESIDUAL_POR_PWM_CM = 0.001f;
constexpr float FRENO_RESIDUAL_MAX_CM = 1.0f;
constexpr uint32_t ASENTAMIENTO_MIN_MS = 250;
constexpr uint32_t ASENTAMIENTO_SIN_PULSOS_MS = 300;
constexpr uint32_t ASENTAMIENTO_MAX_MS = 1500;
// Una misión con objetivo absoluto no puede terminar únicamente por el
// contador longitudinal: debe converger al punto planificado.
constexpr float TOLERANCIA_ENDPOINT_CM = 3.5f;
// Por debajo de este umbral el residual es considerado aceptable o recuperable mediante micro-pulso.
constexpr float DISTANCIA_MINIMA_RECUPERACION_ENDPOINT_CM = 3.5f;
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
constexpr int PWM_TURN_MAX_LIMIT = static_cast<int>(255 * PWM_SCALE_8_TO_10); // Techo al 100% (255/255 = 1023/1023) exclusivo para giros
constexpr int PWM_TURN_KICKSTART = static_cast<int>(247 * PWM_SCALE_8_TO_10); // ~97% arranque instantáneo para romper esticción 4WD
// Piso de potencia mínima para giros (~70.6%) para vencer fricción estática y arrastre lateral 4WD
constexpr int PWM_TURN_FLOOR_MIN = static_cast<int>(180 * PWM_SCALE_8_TO_10);
// Giro inicial a ~97% para vencer fricción estática del chasis 4WD de inmediato
constexpr int PWM_TURN_START = static_cast<int>(247 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_FAR_MARGIN = static_cast<int>(10 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_NEAR_MARGIN = static_cast<int>(4 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_START_FLOOR_OFFSET = static_cast<int>(15 * PWM_SCALE_8_TO_10);
constexpr float KP_GIRO_BALANCE_PWM_POR_CM = 15.0f * PWM_SCALE_8_TO_10;
constexpr int PWM_GIRO_BALANCE_MAX = static_cast<int>(45 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_SLEW_STEP = static_cast<int>(2 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_START_SLEW_STEP = static_cast<int>(6 * PWM_SCALE_8_TO_10);
constexpr uint32_t TURN_RAMP_DOWN_INTERVAL_MS = 30; // 30 ms por escalón de descenso en zona de frenado
constexpr int PWM_TURN_RAMP_DOWN_STEP = static_cast<int>(3 * PWM_SCALE_8_TO_10); // ~1.2% por escalón (~12 unidades)
constexpr float TOLERANCIA_GIRO_DEG = 2.5f;
constexpr float TOLERANCIA_CALIBRACION_DEG = 2.5f;
constexpr float CALIBRACION_GIRO_TEST_DEG = 25.0f;
constexpr float TURN_BRAKING_ZONE_DEG = 15.0f;
constexpr float TURN_HYBRID_THRESHOLD_DEG = 4.0f;
constexpr uint32_t TURN_RAMP_ADAPTIVE_INTERVAL_MS = 150;
constexpr uint32_t TURN_PULSE_ON_MS = 250;
constexpr uint32_t TURN_BRAKE_ACTIVE_MS = 80;
constexpr uint32_t TURN_PULSE_OFF_MS = 120;
constexpr float TURN_REACTIVATION_DEG = 1.0f;
constexpr uint8_t TURN_MAX_ATTEMPTS = 121;
constexpr uint32_t TURN_RETRY_PAUSE_MS = 300;
// Exigir 250 ms de yaw estable en reposo absoluto antes de completar el giro
constexpr uint32_t TURN_SETTLE_MS = 250;
constexpr uint32_t PAUSA_ESTABILIZACION_POST_PASO_MS = 600; // Reposo total del MPU tras frenar avance
constexpr uint32_t PAUSA_ESTABILIZACION_POST_GIRO_MS = 400; // Reposo y verificación tras completar giro
constexpr float TOLERANCIA_CARDINAL_ESTRICTA_DEG = 2.5f; // Unificado con TOLERANCIA_GIRO_DEG
constexpr float UMBRAL_RECORRECCION_POST_FRENO_DEG = 3.0f;
constexpr uint32_t TURN_STALL_MS = 4000;
constexpr uint32_t TURN_TIMEOUT_MS = 90000;
constexpr uint32_t TURN_ATTEMPT_TIMEOUT_MS = 15000;
// Guarda activa de divergencia angular: detecta giro inverso o trompo descontrolado
constexpr float TURN_DIVERGENCE_THRESHOLD_DEG = 8.0f;
constexpr uint32_t TURN_DIVERGENCE_TIMEOUT_MS = 400;
// En calibración (mayor inercia por exploración a alta potencia y retorno), cotas adaptadas
constexpr float TURN_DIVERGENCE_CALIBRATION_THRESHOLD_DEG = 12.0f;
constexpr uint32_t TURN_DIVERGENCE_CALIBRATION_TIMEOUT_MS = 500;

// PID y Correcciones en marcha (MPU como autoridad angular única; encoders no interfieren en rumbo)
constexpr int PWM_CALIBRATION_MARGIN = static_cast<int>(8 * PWM_SCALE_8_TO_10);
constexpr int PWM_CORRECCION_RUMBO_MAX = static_cast<int>(60 * PWM_SCALE_8_TO_10);
constexpr int PWM_CORRECCION_ENCODER_MAX = 0;
constexpr float KP_RUMBO_PWM_POR_GRADO = 10.0f * PWM_SCALE_8_TO_10;
constexpr float KI_RUMBO_PWM_POR_GRADO_S = 1.8f * PWM_SCALE_8_TO_10;
constexpr float KD_RUMBO_PWM_POR_RAD_S = 18.0f * PWM_SCALE_8_TO_10;
constexpr float KP_ENCODER_PWM_POR_TICK = 0.0f;
constexpr float ERROR_INTEGRAL_RUMBO_MAX_GRADO_S = 40.0f;
constexpr float ERROR_ENCODER_AUX_MAX_DEG = 8.0f;
constexpr float KP_LATERAL_RUMBO_DEG_POR_CM = 2.25f;
constexpr float CORRECCION_LATERAL_RUMBO_MAX_DEG = 18.0f;
// Regla de minima rotacion del marco unificado: ante un cambio de rumbo
// |delta theta| >= 105 grados el chasis conmuta a reversa y solo necesita un
// micro-giro de <= 75 grados en vez de un pivote destructivo cercano a 180
// grados (arrastre transversal enorme en chasis 4WD de goma).
constexpr float UMBRAL_REVERSA_AUTOMATICA_DEG = 105.0f;

// Calibración por búsqueda de torque en dos polaridades y retorno por MPU.
constexpr uint32_t CUENTA_CALIBRACION_MS = 5000;
constexpr uint32_t PAUSA_RETORNO_CAL_MS = 2500;
constexpr int      CALIBRATION_PWM_START = static_cast<int>(140 * PWM_SCALE_8_TO_10);
constexpr int      CALIBRATION_PWM_RETRY_START = static_cast<int>(180 * PWM_SCALE_8_TO_10);
constexpr int      CALIBRATION_PWM_END   = PWM_TURN_MAX_LIMIT;
constexpr int      CALIBRATION_PWM_STEP  = static_cast<int>(5 * PWM_SCALE_8_TO_10);
constexpr uint32_t CAL_RAMP_INTERVAL_MS = 350;
constexpr uint32_t CAL_BASE_SLEW_INTERVAL_MS = 20;
constexpr uint32_t CAL_MOVE_SUSTAINED_MS = 100;
constexpr int64_t  CAL_TICKS_MOVIMIENTO = 2;
constexpr uint32_t CAL_GIRO_SIN_ENCODERS_MS = 500;
constexpr uint32_t CAL_RETRY_PAUSE_MS = 750;
constexpr uint32_t CAL_MAX_PWM_STALL_MS = 3000;
constexpr float    CAL_RETURN_PROGRESS_DEG = 0.5f;
constexpr uint32_t CAL_RETURN_NO_PROGRESS_MS = 15000;
constexpr uint32_t CAL_RETURN_TIMEOUT_MS = 60000;
constexpr float DESACUERDO_MAXIMO_PAR = 0.25f;
constexpr uint32_t DESACUERDO_ENCODER_PERSISTENTE_MS = 500;
constexpr uint32_t PAUSA_REEVALUACION_MS = 500;
constexpr int64_t TICKS_MINIMOS_AUDITORIA = 20;
constexpr float ERROR_MAX_CLASIFICAR_DEG = 3.0f;

// Salud persistente de encoders. La ventana no depende del tramo activo para
// que una recuperacion de endpoint no vuelva a declarar sano un cable abierto.
constexpr uint32_t ENCODER_HEALTH_WINDOW_MS = 250;
constexpr uint32_t ENCODER_SUSPECT_MS = 250;
constexpr uint32_t ENCODER_EXCLUDE_MS = 500;
constexpr uint8_t ENCODER_REJOIN_WINDOWS = 8;
constexpr int ENCODER_HEALTH_PWM_MIN = static_cast<int>(60 * PWM_SCALE_8_TO_10);

// Recuperacion de friccion estatica: siete pulsos breves, separados por una
// ventana sin PWM para observar pulsos e inercia. El ultimo nivel conserva el
// limite de avance (242/255, ~95 %).
constexpr uint32_t ANTIFRICTION_TRIGGER_MS = 750;
constexpr uint32_t ANTIFRICTION_PULSE_ON_MS = 300;
constexpr uint32_t ANTIFRICTION_PULSE_OFF_MS = 250;
constexpr uint8_t ANTIFRICTION_PULSE_COUNT = 7;
constexpr int64_t ANTIFRICTION_SUCCESS_TICKS = 2;

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
