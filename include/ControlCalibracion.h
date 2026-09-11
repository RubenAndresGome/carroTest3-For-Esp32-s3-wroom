#pragma once

#include <cstdint>
#include <cmath>

namespace ControlCalibracion {

struct EvidenciaPivot {
  float izquierda = 0.0f;
  float derecha = 0.0f;
  float desbalanceRelativo = 0.0f;
  bool ladoIzquierdoValido = false;
  bool ladoDerechoValido = false;
  bool bilateral = false;
  bool equilibrado = false;
  bool torque = false;
};

inline float promedioLadoConfiable(const int64_t deltas[4],
                                   const bool confiable[4], bool izquierdo) {
  const int primero = izquierdo ? 0 : 1;
  const int segundo = izquierdo ? 2 : 3;
  float suma = 0.0f;
  uint8_t fuentes = 0;
  if (confiable == nullptr || confiable[primero]) {
    suma += static_cast<float>(deltas[primero]);
    ++fuentes;
  }
  if (confiable == nullptr || confiable[segundo]) {
    suma += static_cast<float>(deltas[segundo]);
    ++fuentes;
  }
  return fuentes ? suma / fuentes : 0.0f;
}

inline EvidenciaPivot evaluarPivot(float yawDeltaDeg, const int64_t deltas[4],
                                   const bool confiable[4], int64_t ticksMinimos,
                                   float desbalanceMaximoRelativo) {
  EvidenciaPivot e;
  e.izquierda = promedioLadoConfiable(deltas, confiable, true);
  e.derecha = promedioLadoConfiable(deltas, confiable, false);
  e.ladoIzquierdoValido = e.izquierda >= static_cast<float>(ticksMinimos);
  e.ladoDerechoValido = e.derecha >= static_cast<float>(ticksMinimos);
  e.bilateral = e.ladoIzquierdoValido && e.ladoDerechoValido;
  const float mayor = fmaxf(e.izquierda, e.derecha);
  e.desbalanceRelativo = mayor > 0.0f
      ? fabsf(e.izquierda - e.derecha) / mayor : 0.0f;
  e.equilibrado = e.bilateral &&
      e.desbalanceRelativo <= desbalanceMaximoRelativo;
  e.torque = fabsf(yawDeltaDeg) >= 1.0f &&
      (e.ladoIzquierdoValido || e.ladoDerechoValido);
  return e;
}

struct ComandoPivot {
  int izquierda = 0;
  int derecha = 0;
  int correccion = 0;
};

inline int limitarEntero(int valor, int minimo, int maximo) {
  return valor < minimo ? minimo : (valor > maximo ? maximo : valor);
}

inline ComandoPivot comandoPivotCentrado(int signoYaw, int pwmBase,
                                         float ticksIzquierda,
                                         float ticksDerecha,
                                         float kpPwmPorTick,
                                         int correccionMaxima,
                                         int pwmMaximo) {
  ComandoPivot comando;
  comando.correccion = limitarEntero(
      static_cast<int>(lroundf((ticksIzquierda - ticksDerecha) * kpPwmPorTick)),
      -correccionMaxima, correccionMaxima);
  const int magnitudIzquierda = limitarEntero(
      pwmBase - comando.correccion, 0, pwmMaximo);
  const int magnitudDerecha = limitarEntero(
      pwmBase + comando.correccion, 0, pwmMaximo);
  // Marco canónico: +yaw horario/derecha = L+ / R-. La corrección de balance
  // sólo cambia magnitudes; nunca puede cambiar estos signos.
  const int signo = signoYaw >= 0 ? 1 : -1;
  comando.izquierda = signo * magnitudIzquierda;
  comando.derecha = -signo * magnitudDerecha;
  return comando;
}

enum class ResultadoGuardPivot : uint8_t {
  ESPERANDO_TICKS,
  CONFIRMANDO_YAW,
  CONFIRMADO,
  SIGNO_INCORRECTO,
  ROTACION_NO_CONFIRMADA,
  DESBALANCEADO
};

inline bool signoGyroCorrecto(int signoYawEsperado, float gyroZRadS,
                              float gyroMinimoRadS) {
  const int signo = signoYawEsperado >= 0 ? 1 : -1;
  return gyroZRadS * signo >= gyroMinimoRadS;
}

inline bool signoGyroContrario(int signoYawEsperado, float gyroZRadS,
                               float gyroMinimoRadS) {
  const int signo = signoYawEsperado >= 0 ? 1 : -1;
  return gyroZRadS * signo <= -gyroMinimoRadS;
}

inline bool congelarRampaTrasTicks(const EvidenciaPivot& evidencia) {
  return evidencia.bilateral;
}

inline ResultadoGuardPivot evaluarGuardPivot(
    int signoYawEsperado, float gyroZRadS, float deltaYawDeg,
    const EvidenciaPivot& evidencia, uint32_t verificacionMs,
    uint32_t yawCorrectoSostenidoMs, uint32_t desbalanceSostenidoMs,
    float gyroMinimoRadS, uint32_t yawSostenidoRequeridoMs,
    uint32_t ventanaMaximaMs, int64_t ticksMaximosPorLado,
    uint32_t desbalanceMaximoMs) {
  if (!evidencia.bilateral) return ResultadoGuardPivot::ESPERANDO_TICKS;
  if (signoGyroContrario(signoYawEsperado, gyroZRadS, gyroMinimoRadS) ||
      deltaYawDeg * (signoYawEsperado >= 0 ? 1.0f : -1.0f) <= -1.0f) {
    return ResultadoGuardPivot::SIGNO_INCORRECTO;
  }
  if (!evidencia.equilibrado &&
      desbalanceSostenidoMs >= desbalanceMaximoMs) {
    return ResultadoGuardPivot::DESBALANCEADO;
  }
  if (evidencia.equilibrado &&
      signoGyroCorrecto(signoYawEsperado, gyroZRadS, gyroMinimoRadS) &&
      yawCorrectoSostenidoMs >= yawSostenidoRequeridoMs) {
    return ResultadoGuardPivot::CONFIRMADO;
  }
  if (verificacionMs >= ventanaMaximaMs ||
      (evidencia.izquierda >= static_cast<float>(ticksMaximosPorLado) &&
       evidencia.derecha >= static_cast<float>(ticksMaximosPorLado))) {
    return ResultadoGuardPivot::ROTACION_NO_CONFIRMADA;
  }
  return ResultadoGuardPivot::CONFIRMANDO_YAW;
}

inline bool retornoAlOrigenAceptable(float derivaXCm, float derivaYCm,
                                     float errorYawDeg,
                                     float derivaCentroMaxCm,
                                     float errorYawMaxDeg) {
  return hypotf(derivaXCm, derivaYCm) <= derivaCentroMaxCm &&
         fabsf(errorYawDeg) <= errorYawMaxDeg;
}

struct EvaluacionEncoders {
  int64_t promedioIzquierdo = 0;
  int64_t promedioDerecho = 0;
  uint8_t fuentesIzquierdas = 0;
  uint8_t fuentesDerechas = 0;
  bool ladoIzquierdoValido = false;
  bool ladoDerechoValido = false;
  bool responde[4] = {false, false, false, false};
  bool sinRespuestaAislada[4] = {false, false, false, false};
};

inline EvaluacionEncoders evaluarEncoders(const int64_t deltas[4],
                                          int64_t ticksMinimos,
                                          const bool habilitado[4] = nullptr) {
  EvaluacionEncoders resultado;
  for (int i = 0; i < 4; ++i) {
    resultado.responde[i] = (habilitado == nullptr || habilitado[i]) &&
                            deltas[i] >= ticksMinimos;
    if (!resultado.responde[i]) continue;
    if (i == 0 || i == 2) {
      resultado.promedioIzquierdo += deltas[i];
      ++resultado.fuentesIzquierdas;
    } else {
      resultado.promedioDerecho += deltas[i];
      ++resultado.fuentesDerechas;
    }
  }
  if (resultado.fuentesIzquierdas > 0)
    resultado.promedioIzquierdo /= resultado.fuentesIzquierdas;
  if (resultado.fuentesDerechas > 0)
    resultado.promedioDerecho /= resultado.fuentesDerechas;
  resultado.ladoIzquierdoValido = resultado.fuentesIzquierdas > 0;
  resultado.ladoDerechoValido = resultado.fuentesDerechas > 0;

  // Una fuente sana por lado basta. La fuente silenciosa se identifica sin
  // dividir por dos la magnitud de su pareja; así no se sesga la distancia.
  const int parejas[4] = {2, 3, 0, 1};
  for (int i = 0; i < 4; ++i) {
    resultado.sinRespuestaAislada[i] =
        (habilitado == nullptr || habilitado[i]) && !resultado.responde[i] &&
        resultado.responde[parejas[i]];
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
