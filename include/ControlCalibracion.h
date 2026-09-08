#pragma once

#include <cstdint>
#include "ControlSeguridad.h"

namespace ControlCalibracion {

// Magnitud por lado: una fuente silenciosa no divide por dos la otra.
// No demuestra dirección física ni ausencia de deslizamiento.
struct EvidenciaPivot {
  int64_t izquierda = 0, derecha = 0;
  bool bilateral = false, equilibrado = false, torque = false;
};
inline EvidenciaPivot evaluarPivot(float yawDelta, const int64_t d[4]) {
  EvidenciaPivot e;
  e.izquierda = d[0] > d[2] ? d[0] : d[2];
  e.derecha = d[1] > d[3] ? d[1] : d[3];
  e.bilateral = e.izquierda >= 2 && e.derecha >= 2;
  e.equilibrado = e.bilateral && e.izquierda <= 2 * e.derecha && e.derecha <= 2 * e.izquierda;
  e.torque = (yawDelta >= 1.0f || yawDelta <= -1.0f) && (e.izquierda >= 2 || e.derecha >= 2);
  return e;
}

struct VigilanciaPivot {
  bool movimiento = false;
  uint32_t inicioMs = 0, ventanaMs = 0;
  uint8_t ventanasAsimetricas = 0;
  int64_t anterior[4] = {};
};
inline const char* vigilarPivot(VigilanciaPivot& v, const EvidenciaPivot& e,
                               float yawDelta, const int64_t d[4], uint32_t ahora) {
  const float angulo = yawDelta < 0 ? -yawDelta : yawDelta;
  // La ventana de asimetría empieza con movimiento útil confirmado, no con
  // un tick aislado o deriva angular. Armarla antes reproducía los falsos
  // watchdog durante la rampa que motivaron esta corrección.
  if (!v.movimiento && e.torque) {
    v.movimiento = true; v.inicioMs = v.ventanaMs = ahora;
    for (int i=0;i<4;++i) v.anterior[i] = d[i];
  }
  if (!v.movimiento) return nullptr;
  if (ahora - v.ventanaMs >= 250) {
    int64_t ventana[4];
    for (int i=0;i<4;++i) { ventana[i] = d[i] - v.anterior[i]; v.anterior[i] = d[i]; }
    const EvidenciaPivot w = evaluarPivot(0, ventana);
    const bool desigual = (w.izquierda >= 2 || w.derecha >= 2) && !w.equilibrado;
    v.ventanasAsimetricas = desigual ? v.ventanasAsimetricas + 1 : 0;
    v.ventanaMs = ahora;
  }
  if (v.ventanasAsimetricas >= 2 || angulo >= 3.0f || ahora-v.inicioMs >= 1500) {
    if (!e.bilateral) return "cal_pivot_one_side_only";
    if (!e.equilibrado || !e.torque) return "cal_pivot_asymmetric";
  }
  return nullptr;
}

struct EvaluacionEncoders {
  int64_t promedioIzquierdo = 0;
  int64_t promedioDerecho = 0;
  bool ladoIzquierdoValido = false;
  bool ladoDerechoValido = false;
  bool responde[4] = {false, false, false, false};
  bool sinRespuestaAislada[4] = {false, false, false, false};
};

struct VigilanciaSilencioEncoders {
  bool activa = false;
  uint32_t inicioMs = 0;
  uint32_t transcurridoMs = 0;
};

struct EvidenciaMovimiento {
  bool gyroConfirmado = false;
  bool pcntCorroborado = false;
  uint8_t encodersQueResponden = 0;

  bool confirmada() const { return gyroConfirmado && pcntCorroborado; }
};

struct ComandoPivot {
  int izquierda = 0;
  int derecha = 0;
};

inline ComandoPivot comandoPivot(int candidato, int pwm) {
  ComandoPivot comando;
  comando.izquierda = -candidato * pwm;
  comando.derecha = candidato * pwm;
  return comando;
}

inline EvidenciaMovimiento evaluarMovimiento(float gyroAbsRadS,
                                             float gyroMinimoRadS,
                                             const int64_t deltas[4],
                                             int64_t ticksMinimos) {
  EvidenciaMovimiento evidencia;
  evidencia.gyroConfirmado = gyroAbsRadS >= gyroMinimoRadS;
  for (int i = 0; i < 4; ++i) {
    if (deltas[i] >= ticksMinimos) ++evidencia.encodersQueResponden;
  }
  evidencia.pcntCorroborado = evidencia.encodersQueResponden > 0;
  return evidencia;
}

inline bool actualizarSilencioConGiro(VigilanciaSilencioEncoders& vigilancia,
                                      bool giroConfirmado,
                                      const int64_t deltas[4],
                                      uint32_t ahoraMs,
                                      uint32_t limiteMs) {
  bool algunPulso = false;
  for (int i = 0; i < 4; ++i) algunPulso |= deltas[i] != 0;
  if (!giroConfirmado || algunPulso) {
    vigilancia = {};
    return false;
  }
  if (!vigilancia.activa) {
    vigilancia.activa = true;
    vigilancia.inicioMs = ahoraMs;
    vigilancia.transcurridoMs = 0;
    return false;
  }
  vigilancia.transcurridoMs = ahoraMs - vigilancia.inicioMs;
  return vigilancia.transcurridoMs >= limiteMs;
}

inline void reiniciarVigilanciaRetorno(float errorAbsDeg,
                                       uint32_t ahoraMs,
                                       float& referenciaErrorDeg,
                                       uint32_t& ultimoProgresoMs) {
  referenciaErrorDeg = errorAbsDeg;
  ultimoProgresoMs = ahoraMs;
}

inline bool retornoSinProgreso(float errorAbsDeg,
                               uint32_t ahoraMs,
                               float progresoMinimoDeg,
                               uint32_t limiteMs,
                               float& referenciaErrorDeg,
                               uint32_t& ultimoProgresoMs) {
  if (referenciaErrorDeg - errorAbsDeg >= progresoMinimoDeg) {
    referenciaErrorDeg = errorAbsDeg;
    ultimoProgresoMs = ahoraMs;
    return false;
  }
  return ahoraMs - ultimoProgresoMs >= limiteMs;
}

inline EvaluacionEncoders evaluarEncoders(const int64_t deltas[4],
                                          int64_t ticksMinimos,
                                          const bool confiable[4] = nullptr) {
  EvaluacionEncoders resultado;
  if (confiable != nullptr) {
    resultado.promedioIzquierdo = static_cast<int64_t>(
        ControlSeguridad::promedioConfiableLado(deltas, confiable, true));
    resultado.promedioDerecho = static_cast<int64_t>(
        ControlSeguridad::promedioConfiableLado(deltas, confiable, false));
  } else {
    resultado.promedioIzquierdo = (deltas[0] + deltas[2]) / 2;
    resultado.promedioDerecho = (deltas[1] + deltas[3]) / 2;
  }
  resultado.ladoIzquierdoValido = resultado.promedioIzquierdo >= ticksMinimos;
  resultado.ladoDerechoValido = resultado.promedioDerecho >= ticksMinimos;

  for (int i = 0; i < 4; ++i) {
    resultado.responde[i] = deltas[i] >= ticksMinimos;
  }

  // La pareja debe aportar el doble del umbral para demostrar que el lado se
  // movio aun cuando una fuente individual permanezca en cero. La aceptacion
  // de la calibracion conserva el promedio de ambos encoders por lado cuando
  // ambos son confiables, o la fuente aislada restante si una fallo.
  const int parejas[4] = {2, 3, 0, 1};
  for (int i = 0; i < 4; ++i) {
    resultado.sinRespuestaAislada[i] =
        !resultado.responde[i] && deltas[parejas[i]] >= ticksMinimos * 2;
  }
  return resultado;
}

inline uint8_t totalPasosRampa(int inicio, int fin, int paso) {
  if (paso <= 0 || fin <= inicio) return 1;
  return static_cast<uint8_t>(((fin - inicio) + paso - 1) / paso + 1);
}

inline uint8_t pasoRampaActual(int pwm, int inicio, int fin, int paso) {
  const uint8_t total = totalPasosRampa(inicio, fin, paso);
  if (pwm <= inicio || paso <= 0) return 1;
  if (pwm >= fin) return total;
  const int avance = pwm - inicio;
  const int indice = (avance + paso - 1) / paso + 1;
  return static_cast<uint8_t>(indice > total ? total : indice);
}

}  // namespace ControlCalibracion
