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
  return 0;
}

static void setMotorPWM(int pinFwd, int pinRev, int vel) {
  const int canalFwd = canalParaPin(pinFwd);
  const int canalRev = canalParaPin(pinRev);
  if (vel > 0) {
    ledcWrite(canalFwd, vel);
    ledcWrite(canalRev, 0);
  } else if (vel < 0) {
    ledcWrite(canalFwd, 0);
    ledcWrite(canalRev, -vel);
  } else {
    ledcWrite(canalFwd, 0);
    ledcWrite(canalRev, 0);
  }
}

static void aplicarLadoUnico(int pinFwd, int pinRev, int vel) {
  vel = constrain(vel, -PWM_TURN_MAX_LIMIT, PWM_TURN_MAX_LIMIT);
  setMotorPWM(pinFwd, pinRev, vel * PWM_FORWARD_POLARITY);
}

void aplicarVelocidades(int velIzq, int velDer) {
  const uint32_t ahora = millis();
  pwm_solicitado_L = constrain(velIzq, -PWM_TURN_MAX_LIMIT, PWM_TURN_MAX_LIMIT);
  pwm_solicitado_R = constrain(velDer, -PWM_TURN_MAX_LIMIT, PWM_TURN_MAX_LIMIT);
  pwm_aplicado_L = interlockL.actualizar(pwm_solicitado_L, ahora);
  pwm_aplicado_R = interlockR.actualizar(pwm_solicitado_R, ahora);

  aplicarLadoUnico(PIN_FL_FWD, PIN_FL_REV, pwm_aplicado_L);
  aplicarLadoUnico(PIN_BL_FWD, PIN_BL_REV, pwm_aplicado_L);
  aplicarLadoUnico(PIN_FR_FWD, PIN_FR_REV, pwm_aplicado_R);
  aplicarLadoUnico(PIN_BR_FWD, PIN_BR_REV, pwm_aplicado_R);
}

void frenarMotores() {
  const uint32_t ahora = millis();
  interlockL.detener(ahora);
  interlockR.detener(ahora);
  for (int canal = 0; canal < 8; ++canal) ledcWrite(canal, 0);
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
  if (prueba.actualizar(PWM_SAFE_HARD_LIMIT + 100, 5000) != PWM_SAFE_HARD_LIMIT) return false;

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

void setup_MotorPinsLow() {
  for (int i = 0; i < 8; ++i) {
    pinMode(MOTOR_PINS[i], OUTPUT);
    digitalWrite(MOTOR_PINS[i], LOW);
  }
}

void setup_Motores() {
  LOG_CORE("Inicializando Motores DRV8833...");
  for (int canal = 0; canal < 8; ++canal) {
    ledcSetup(canal, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcAttachPin(MOTOR_PINS[canal], canal);
  }
  frenarMotores();
}
