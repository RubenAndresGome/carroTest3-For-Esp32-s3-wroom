#pragma once
#include <Arduino.h>

constexpr char FIRMWARE_VERSION[] = "robot-s3-v1.3-recovery-ladder";
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
// Verificado físicamente el 2026-07-22: con el montaje actual del MPU6050, el
// signo Z publicado por el módulo es opuesto a la convención del mapa/HMI.
// Normalizarlo aquí mantiene +Y=0°, +X=+90° y -X=-90° en todo el sistema.
constexpr float MPU_YAW_POLARITY = -1.0f;
// Con el cableado aprobado, candidato +1 genera yaw crudo positivo. Como el
// yaw canónico se invierte arriba, el candidato -1 produce yaw canónico +.
constexpr int TURN_CANDIDATE_POSITIVE_YAW = -1;
constexpr int TURN_CANDIDATE_NEGATIVE_YAW = 1;

// Correccion empirica del giro relativo. Un valor positivo pide girar un poco
// mas en ambos sentidos; uno negativo pide girar menos. Debe medirse en piso
// real (por ejemplo, +2.5 si una orden de 90 grados termina cerca de 87.5).
constexpr float THETA_ERROR_DEG = 0.0f;

constexpr float WHEEL_DIAMETER_CM = 6.6f;
constexpr int ENCODER_PPR = 20;
// Calibracion empirica del odometro. En piso, el robot recorria al menos 10 %
// mas de lo indicado por los ticks. Mantener este factor separado del diametro
// fisico permite afinarlo sin falsear la geometria documentada de la rueda.
constexpr float ENCODER_DISTANCE_SCALE = 1.10f;
constexpr float WHEEL_DIAMETER_ODOMETRY_CM =
    WHEEL_DIAMETER_CM * ENCODER_DISTANCE_SCALE;

// Calibración y PID
const float CM_POR_PULSO = 1.0;
constexpr uint8_t PWM_RESOLUTION_BITS = 10;
constexpr int PWM_RESOLUTION_MAX = (1 << PWM_RESOLUTION_BITS) - 1;
constexpr float PWM_SCALE_8_TO_10 = static_cast<float>(PWM_RESOLUTION_MAX) / 255.0f;
constexpr int PWM_FORWARD_POLARITY = -1;
// 230/255 = 90.2 %. El tiempo muerto y los watchdogs siguen siendo obligatorios.
constexpr int PWM_SAFE_LIMIT_8BIT = 230;
constexpr int PWM_SAFE_HARD_LIMIT = static_cast<int>(PWM_SAFE_LIMIT_8BIT * PWM_SCALE_8_TO_10);
constexpr int VELOCIDAD_BASE_RECTO = PWM_SAFE_HARD_LIMIT;
constexpr int VELOCIDAD_APROXIMACION = static_cast<int>(200 * PWM_SCALE_8_TO_10);
constexpr int VELOCIDAD_MINIMA_RECTO = static_cast<int>(175 * PWM_SCALE_8_TO_10);
constexpr int VELOCIDAD_MINIMA_GIRO_INICIAL = static_cast<int>(140 * PWM_SCALE_8_TO_10);
constexpr int PWM_CALIBRATION_MARGIN = static_cast<int>(10 * PWM_SCALE_8_TO_10);
constexpr uint32_t DRIVE_ACCELERATION_MS = 500;
constexpr uint32_t ENCODER_CONTROL_GRACE_MS = 200;
constexpr uint32_t ENCODER_OUTLIER_PERSISTENCE_MS = 500;
constexpr int64_t ENCODER_MIN_AUDIT_TICKS = 4;
constexpr float ENCODER_MAX_RELATIVE_DISAGREEMENT = 0.40f;
constexpr int PWM_CORRECCION_RUMBO_MAX = static_cast<int>(80 * PWM_SCALE_8_TO_10);
constexpr int PWM_CORRECCION_ENCODER_MAX = static_cast<int>(15 * PWM_SCALE_8_TO_10);
constexpr float KP_RUMBO_PWM_POR_GRADO = 4.0f * PWM_SCALE_8_TO_10;
constexpr float KD_RUMBO_PWM_POR_RAD_S = 12.0f * PWM_SCALE_8_TO_10;
constexpr float KP_ENCODER_PWM_POR_TICK = 1.5f * PWM_SCALE_8_TO_10;
// Todos los giros, incluidas las alineaciones de ruta, usan una sola meta real.
// No existe una parada intermedia de alineacion gruesa.
constexpr float TOLERANCIA_GIRO_DEG = 2.0f;
// Cada waypoint autonomo se declara completado solamente después de regresar
// al rumbo global de reposo: frente del robot hacia +Y. 0, +360 y -360 son
// el mismo rumbo físico.
constexpr float RUMBO_REPOSO_RUTA_DEG = 0.0f;
constexpr float HEADING_RECOVERY_TRIGGER_DEG = 12.0f;
constexpr uint32_t HEADING_RECOVERY_HOLD_MS = 300;
constexpr float HEADING_RECOVERY_MAX_STEP_DEG = 25.0f;
constexpr uint8_t HEADING_RECOVERY_MAX_ATTEMPTS = 7;
// El lazo de giro corrige continuamente el signo del error. Un sobrepaso baja
// PWM hasta cero, respeta el interlock y continúa en sentido contrario.
constexpr uint32_t TURN_TIMEOUT_MS = 240000;
constexpr uint32_t TURN_CONTROL_PERIOD_MS = 20;
// Control simple inspirado en el ensayo físico aprobado: si el giro cruza el
// objetivo, se detiene por completo antes de corregir en sentido contrario.
constexpr uint32_t TURN_OVERSHOOT_PAUSE_MS = 300;
// Durante un giro el MPU es la única autoridad angular. Los encoders sólo
// prueban que los lados se mueven: si hay desplazamiento físico sin respuesta
// del yaw, o el yaw avanza en sentido opuesto, la maniobra se corta pronto.
constexpr float TURN_IMU_MIN_RESPONSE_DEG = 1.0f;
constexpr uint32_t TURN_IMU_RESPONSE_TIMEOUT_MS = 500;
constexpr float TURN_DIRECTION_MISMATCH_DEG = 3.0f;
constexpr uint32_t TURN_DIRECTION_MISMATCH_HOLD_MS = 250;
constexpr float TURN_ANGULAR_PROGRESS_DEG = 0.5f;
constexpr uint32_t TURN_NO_ANGULAR_PROGRESS_TIMEOUT_MS = 5000;
// Una falta de movimiento no termina la mision en el primer intento. El
// controlador frena, deja reposar el puente y vuelve a intentar el residuo.
// FAULT queda reservado para el agotamiento completo de esta escalera.
constexpr uint8_t AUTONOMOUS_MOTION_MAX_ATTEMPTS = 7;
constexpr uint32_t AUTONOMOUS_RETRY_PAUSE_MS = 750;
constexpr int PWM_TURN_FAR_MARGIN = static_cast<int>(25 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_NEAR_MARGIN = static_cast<int>(8 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_SLEW_STEP = static_cast<int>(2 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_BRAKE_SLEW_STEP = static_cast<int>(2 * PWM_SCALE_8_TO_10);
// Techo usado por la búsqueda de torque. El giro normal parte de cero y usa el
// mínimo calibrado más los márgenes cercano/lejano.
constexpr int PWM_TURN_START = static_cast<int>(230 * PWM_SCALE_8_TO_10);
constexpr int PWM_TURN_START_SLEW_STEP = static_cast<int>(2 * PWM_SCALE_8_TO_10);
constexpr float TURN_BRAKING_ZONE_DEG = 25.0f;
constexpr float TURN_NEAR_ZONE_DEG = 8.0f;
constexpr uint32_t TURN_PREDICTION_LOOKAHEAD_MS = 150;
constexpr uint32_t TURN_STABLE_MS = 250;
// El ensayo físico esperaba 2.5 s de actividad por lado durante un giro. El
// watchdog de avance sigue siendo más corto porque allí ya existe rodamiento.
constexpr uint32_t TURN_STALL_TIMEOUT_MS = 2500;
constexpr uint32_t NORMAL_STALL_TIMEOUT_MS = 450;
constexpr uint32_t AUTONOMOUS_SIDE_STALL_TIMEOUT_MS = 2500;
constexpr float CALIBRATION_TURN_DEG = 25.0f;
constexpr int      CALIBRATION_PWM_START          = static_cast<int>(140 * PWM_SCALE_8_TO_10);
constexpr int      CALIBRATION_PWM_END            = PWM_SAFE_HARD_LIMIT;
constexpr int      CALIBRATION_PWM_STEP           = static_cast<int>(5 * PWM_SCALE_8_TO_10);
constexpr uint32_t CALIBRATION_PWM_STEP_INTERVAL_MS = 250;
constexpr uint32_t CALIBRATION_MOVEMENT_STABLE_MS = 100;
constexpr int64_t  CALIBRATION_TICKS_PER_SIDE = 2;
constexpr uint8_t  CALIBRATION_TORQUE_MAX_ATTEMPTS = 7;
constexpr uint32_t CALIBRATION_TORQUE_RETRY_PAUSE_MS = 750;
constexpr uint32_t CALIBRATION_DIRECTION_PAUSE_MS = 750;
constexpr uint32_t CALIBRATION_VALIDATION_PAUSE_MS = 2500;
constexpr float YAW_RECENTER_THRESHOLD_DEG = 360.0f;
constexpr float ORTHOGONAL_TOLERANCE_CM = 0.1f;
constexpr float TOLERANCIA_DISTANCIA_CM = 2.0f;
// La planeacion sigue exigiendo ortogonalidad de 1 mm. Al ejecutar un nuevo
// waypoint se admite el residuo fisico normal del tramo anterior para decidir
// el eje, sin convertirlo en una diagonal.
constexpr float ROUTE_CROSS_TRACK_RESIDUAL_CM = 2.5f;
constexpr float ROUTE_AXIS_DOMINANCE_RATIO = 1.5f;
constexpr float TOLERANCIA_DISTANCIA_MIN_CM = 0.25f;
constexpr float TOLERANCIA_DISTANCIA_FRACCION = 0.10f;
constexpr float GYRO_MOVEMENT_RAD_S = 0.12f;
constexpr float MAX_AUTONOMOUS_SEGMENT_MM = 2000.0f;
constexpr uint32_t CALIBRATION_STALL_ACCUMULATED_MS = 800;
constexpr int      PWM_DIRECTION_PAUSE_MS = 250;
constexpr uint32_t TASK_WEB_STACK_BYTES = 8192;
constexpr uint32_t CONTROL_LOOP_PERIOD_US = SENSOR_PERIOD_MS * 1000UL;
constexpr UBaseType_t CONTROL_LOOP_PRIORITY = 10;
constexpr uint32_t RTOS_STACK_MIN_ACCEPTABLE_BYTES = 1024;
