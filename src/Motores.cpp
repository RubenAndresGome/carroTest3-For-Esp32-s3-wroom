#include "Motores.h"
#include "Config.h"
#include "Debug.h"

int pwm_aplicado_L = 0;
int pwm_aplicado_R = 0;
int pwm_solicitado_L = 0;
int pwm_solicitado_R = 0;

static constexpr uint32_t PWM_FREQUENCY = 5000;
static const int MOTOR_PINS[8] = {
  PIN_BL_FWD, PIN_BL_REV, PIN_BR_FWD, PIN_BR_REV,
  PIN_FL_FWD, PIN_FL_REV, PIN_FR_FWD, PIN_FR_REV
};

enum class EstadoMotores : uint8_t {
  NO_CONFIGURADOS,
  MAPA_INVALIDO,
  LISTOS,
  ERROR_SALIDA
};

static EstadoMotores estadoSalidaMotores = EstadoMotores::NO_CONFIGURADOS;
static uint8_t mascaraCanalesPWM = 0;
static DiagnosticoSalidaMotor diagnosticoMotores[4] = {};

enum class EstadoInterlock : uint8_t { APAGADO, ACTIVO, ESPERANDO_INVERSION };

struct InterlockLado {
  EstadoInterlock estado = EstadoInterlock::APAGADO;
  int signoActivo = 0;
  int ultimoSignoEnergizado = 0;
  int signoPendiente = 0;
  uint32_t apagadoDesdeMs = 0;
  bool apagadoRegistrado = false;

  int actualizar(int velocidadSolicitada, uint32_t ahora) {
    velocidadSolicitada = constrain(velocidadSolicitada, -PWM_TURN_MAX_LIMIT, PWM_TURN_MAX_LIMIT);
    const int signoDeseado = velocidadSolicitada > 0 ? 1 : (velocidadSolicitada < 0 ? -1 : 0);
    const int magnitudDeseada = abs(velocidadSolicitada);

    if (signoDeseado == 0) {
      detener(ahora);
      return 0;
    }

    if (estado == EstadoInterlock::ACTIVO) {
      if (signoDeseado == signoActivo) return signoActivo * magnitudDeseada;
      ultimoSignoEnergizado = signoActivo;
      apagadoDesdeMs = ahora;
      apagadoRegistrado = true;
      signoActivo = 0;
      signoPendiente = signoDeseado;
      estado = EstadoInterlock::ESPERANDO_INVERSION;
      return 0;
    }

    if (estado == EstadoInterlock::ESPERANDO_INVERSION) {
      signoPendiente = signoDeseado;
      if (static_cast<uint32_t>(ahora - apagadoDesdeMs) < PWM_DIRECTION_PAUSE_MS) return 0;
      signoActivo = signoPendiente;
      signoPendiente = 0;
      estado = EstadoInterlock::ACTIVO;
      return signoActivo * magnitudDeseada;
    }

    if (apagadoRegistrado && ultimoSignoEnergizado != 0 && signoDeseado != ultimoSignoEnergizado &&
        static_cast<uint32_t>(ahora - apagadoDesdeMs) < PWM_DIRECTION_PAUSE_MS) {
      signoPendiente = signoDeseado;
      estado = EstadoInterlock::ESPERANDO_INVERSION;
      return 0;
    }

    signoActivo = signoDeseado;
    signoPendiente = 0;
    estado = EstadoInterlock::ACTIVO;
    return signoActivo * magnitudDeseada;
  }

  void detener(uint32_t ahora) {
    if (estado == EstadoInterlock::ACTIVO && signoActivo != 0) {
      ultimoSignoEnergizado = signoActivo;
      apagadoDesdeMs = ahora;
      apagadoRegistrado = true;
    }
    signoActivo = 0;
    signoPendiente = 0;
    estado = EstadoInterlock::APAGADO;
  }
};

static InterlockLado interlockL;
static InterlockLado interlockR;

static int canalParaPin(int pin) {
  for (int i = 0; i < 8; ++i) if (MOTOR_PINS[i] == pin) return i;
  // Nunca asumir canal 0: un pin desconocido no debe energizar otra salida.
  return -1;
}

static bool parMotorValido(int pinFwd, int pinRev) {
  const int canalFwd = canalParaPin(pinFwd);
  const int canalRev = canalParaPin(pinRev);
  return pinFwd >= 0 && pinRev >= 0 && canalFwd >= 0 && canalRev >= 0 &&
         canalFwd != canalRev;
}

bool validarMapaMotores() {
  for (int i = 0; i < 8; ++i) {
    if (MOTOR_PINS[i] < 0 || MOTOR_PINS[i] > 48) return false;
    for (int j = i + 1; j < 8; ++j) {
      if (MOTOR_PINS[i] == MOTOR_PINS[j]) return false;
    }
  }
  return parMotorValido(PIN_FL_FWD, PIN_FL_REV) &&
         parMotorValido(PIN_BL_FWD, PIN_BL_REV) &&
         parMotorValido(PIN_FR_FWD, PIN_FR_REV) &&
         parMotorValido(PIN_BR_FWD, PIN_BR_REV);
}

static void apagarCanalesPWM() {
  // Apagado directo e idempotente: no depende del mapa ni llama a
  // frenarMotores(), evitando recursión durante una ruta de error. Antes de
  // ledcSetup los GPIO ya están en LOW por setup_MotorPinsLow(); no se escribe
  // sobre canales que aún no existen.
  for (int canal = 0; canal < 8; ++canal) {
    if ((mascaraCanalesPWM & (1U << canal)) != 0U) ledcWrite(canal, 0);
  }
  for (auto& diagnostico : diagnosticoMotores) {
    diagnostico.pwmLogico = 0;
    diagnostico.gpioActivo = -1;
    diagnostico.duty = 0;
    diagnostico.dutyAvance = 0;
    diagnostico.dutyReversa = 0;
  }
}

static bool setMotorPWM(MotorId motor, int pinFwd, int pinRev, int vel) {
  if (!parMotorValido(pinFwd, pinRev)) return false;
  const int canalFwd = canalParaPin(pinFwd);
  const int canalRev = canalParaPin(pinRev);
  const ControlMotores::SalidaElectrica salida =
      ControlMotores::resolverSalida(pinFwd, pinRev, vel);
  ledcWrite(canalFwd, salida.dutyAvance);
  ledcWrite(canalRev, salida.dutyReversa);
  DiagnosticoSalidaMotor& diagnostico = diagnosticoMotores[static_cast<uint8_t>(motor)];
  diagnostico.pwmLogico = salida.pwmLogico;
  diagnostico.gpioActivo = salida.gpioActivo;
  diagnostico.duty = salida.duty;
  diagnostico.dutyAvance = salida.dutyAvance;
  diagnostico.dutyReversa = salida.dutyReversa;
  return true;
}

static bool aplicarLadoUnico(MotorId motor, int pinFwd, int pinRev, int vel) {
  vel = constrain(vel, -PWM_TURN_MAX_LIMIT, PWM_TURN_MAX_LIMIT);
  return setMotorPWM(motor, pinFwd, pinRev, vel);
}

bool aplicarVelocidades(int velIzq, int velDer) {
  // Preflight completo: si el mapa o la inicialización son inválidos, no se
  // permite ninguna escritura parcial ni se elige un canal por defecto.
  const bool mapaValido = validarMapaMotores();
  const bool paresValidos = mapaValido &&
      parMotorValido(PIN_FL_FWD, PIN_FL_REV) &&
      parMotorValido(PIN_BL_FWD, PIN_BL_REV) &&
      parMotorValido(PIN_FR_FWD, PIN_FR_REV) &&
      parMotorValido(PIN_BR_FWD, PIN_BR_REV);
  if (!motoresListos() || !paresValidos) {
    if (!mapaValido || !paresValidos) {
      estadoSalidaMotores = EstadoMotores::MAPA_INVALIDO;
    } else if (estadoSalidaMotores != EstadoMotores::ERROR_SALIDA) {
      // ERROR_SALIDA queda enclavado hasta una nueva inicialización explícita.
      estadoSalidaMotores = EstadoMotores::NO_CONFIGURADOS;
    }
    pwm_solicitado_L = 0;
    pwm_solicitado_R = 0;
    pwm_aplicado_L = 0;
    pwm_aplicado_R = 0;
    apagarCanalesPWM();
    return false;
  }
  const uint32_t ahora = millis();
  pwm_solicitado_L = constrain(velIzq, -PWM_TURN_MAX_LIMIT, PWM_TURN_MAX_LIMIT);
  pwm_solicitado_R = constrain(velDer, -PWM_TURN_MAX_LIMIT, PWM_TURN_MAX_LIMIT);
  pwm_aplicado_L = interlockL.actualizar(pwm_solicitado_L, ahora);
  pwm_aplicado_R = interlockR.actualizar(pwm_solicitado_R, ahora);

  const bool escrito = aplicarLadoUnico(MotorId::FL, PIN_FL_FWD, PIN_FL_REV, pwm_aplicado_L) &&
                       aplicarLadoUnico(MotorId::BL, PIN_BL_FWD, PIN_BL_REV, pwm_aplicado_L) &&
                       aplicarLadoUnico(MotorId::FR, PIN_FR_FWD, PIN_FR_REV, pwm_aplicado_R) &&
                       aplicarLadoUnico(MotorId::BR_WHEEL, PIN_BR_FWD, PIN_BR_REV, pwm_aplicado_R);
  if (!escrito) {
    estadoSalidaMotores = EstadoMotores::ERROR_SALIDA;
    pwm_solicitado_L = 0;
    pwm_solicitado_R = 0;
    pwm_aplicado_L = 0;
    pwm_aplicado_R = 0;
    apagarCanalesPWM();
    return false;
  }
  return true;
}

void frenarMotores() {
  const uint32_t ahora = millis();
  interlockL.detener(ahora);
  interlockR.detener(ahora);
  apagarCanalesPWM();
  pwm_aplicado_L = 0;
  pwm_aplicado_R = 0;
  pwm_solicitado_L = 0;
  pwm_solicitado_R = 0;
}

bool validarInterlockMotores() {
  InterlockLado prueba;
  if (prueba.actualizar(100, 1000) != 100) return false;
  if (prueba.actualizar(-100, 1010) != 0) return false;
  if (prueba.actualizar(-100, 1259) != 0) return false;
  if (prueba.actualizar(-100, 1260) != -100) return false;

  prueba = InterlockLado{};
  if (prueba.actualizar(90, 2000) != 90) return false;
  prueba.detener(2020);
  if (prueba.actualizar(-90, 2050) != 0) return false;
  if (prueba.actualizar(-90, 2269) != 0) return false;
  if (prueba.actualizar(-90, 2270) != -90) return false;

  prueba = InterlockLado{};
  if (prueba.actualizar(80, 3000) != 80) return false;
  if (prueba.actualizar(-80, 3010) != 0) return false;
  if (prueba.actualizar(70, 3050) != 0) return false;
  if (prueba.actualizar(70, 3259) != 0) return false;
  if (prueba.actualizar(70, 3260) != 70) return false;

  prueba = InterlockLado{};
  if (prueba.actualizar(80, 4000) != 80) return false;
  if (prueba.actualizar(-80, 4010) != 0) return false;
  prueba.detener(4020);
  if (prueba.estado != EstadoInterlock::APAGADO || prueba.signoPendiente != 0 || prueba.signoActivo != 0) return false;

  prueba = InterlockLado{};
  if (prueba.actualizar(PWM_TURN_MAX_LIMIT + 100, 5000) != PWM_TURN_MAX_LIMIT) return false;

  prueba = InterlockLado{};
  if (prueba.actualizar(60, UINT32_MAX - 50U) != 60) return false;
  if (prueba.actualizar(-60, UINT32_MAX - 40U) != 0) return false;
  if (prueba.actualizar(-60, 208U) != 0) return false;
  if (prueba.actualizar(-60, 209U) != -60) return false;
  return true;
}

static const char* textoEstado(EstadoInterlock estado) {
  switch (estado) {
    case EstadoInterlock::ACTIVO: return "ACTIVE";
    case EstadoInterlock::ESPERANDO_INVERSION: return "WAITING_REVERSAL";
    default: return "OFF";
  }
}

const char* estadoInterlockL() { return textoEstado(interlockL.estado); }
const char* estadoInterlockR() { return textoEstado(interlockR.estado); }
int signoEnergizadoL() { return interlockL.signoActivo; }
int signoEnergizadoR() { return interlockR.signoActivo; }
int signoPendienteL() { return interlockL.signoPendiente; }
int signoPendienteR() { return interlockR.signoPendiente; }

DiagnosticoSalidaMotor diagnosticoSalidaMotor(MotorId motor) {
  return diagnosticoMotores[static_cast<uint8_t>(motor)];
}

bool motoresListos() { return estadoSalidaMotores == EstadoMotores::LISTOS; }

const char* estadoMotores() {
  switch (estadoSalidaMotores) {
    case EstadoMotores::MAPA_INVALIDO: return "MAPA_INVALIDO";
    case EstadoMotores::LISTOS: return "LISTOS";
    case EstadoMotores::ERROR_SALIDA: return "ERROR_SALIDA";
    default: return "NO_CONFIGURADOS";
  }
}

void setup_MotorPinsLow() {
  // Bajar todos los GPIO físicamente representables incluso si el mapa tiene
  // duplicados. Un error de configuración no debe impedir desenergizar el
  // resto de entradas del driver durante el arranque.
  for (int i = 0; i < 8; ++i) {
    if (MOTOR_PINS[i] < 0 || MOTOR_PINS[i] > 48) continue;
    pinMode(MOTOR_PINS[i], OUTPUT);
    digitalWrite(MOTOR_PINS[i], LOW);
  }
  if (!validarMapaMotores()) estadoSalidaMotores = EstadoMotores::MAPA_INVALIDO;
}

bool setup_Motores() {
  LOG_CORE("Inicializando Motores DRV8833...");
  // Una reinicialización debe desenergizar primero cualquier canal válido de
  // la configuración anterior. La máscara también permite apagar un setup
  // parcial sin escribir sobre canales que nunca se configuraron.
  apagarCanalesPWM();
  mascaraCanalesPWM = 0;
  if (!validarMapaMotores()) {
    estadoSalidaMotores = EstadoMotores::MAPA_INVALIDO;
    return false;
  }
  for (int canal = 0; canal < 8; ++canal) {
    const double frecuenciaReal = ledcSetup(canal, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    if (frecuenciaReal <= 0.0) {
      apagarCanalesPWM();
      estadoSalidaMotores = EstadoMotores::ERROR_SALIDA;
      return false;
    }
    mascaraCanalesPWM |= static_cast<uint8_t>(1U << canal);
    ledcAttachPin(MOTOR_PINS[canal], canal);
    // No depender del duty inicial que deje la implementación de LEDC.
    ledcWrite(canal, 0);
  }
  estadoSalidaMotores = EstadoMotores::LISTOS;
  frenarMotores();
  return true;
}
