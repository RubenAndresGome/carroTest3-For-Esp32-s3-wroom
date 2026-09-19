#pragma once

#include <cmath>
#include <cstdint>

// Lazo angular graduado durante el avance lineal. Unidad pura (sin Arduino ni
// hardware) para validar umbrales, signos y transiciones en pruebas nativas.
//
// Convenio del robot: 0 grados = +Y, 90 grados = +X, yaw horario positivo.
// err = objetivo - actual. err > 0 exige girar a la derecha (horario): se
// reduce el lado derecho y se empuja el izquierdo.
namespace ControlAngular {

enum class ModoRecuperacion : uint8_t {
  CONTINUO_SUAVE,       // Modo A: diferencial suave sin detener el avance
  FRENADO_TRANSITORIO,  // Modo B: diferencial fuerte con retencion breve
  PAUSA_PIVOTE,         // Modo C: pausa, pivote y reanudar el conteo
  CIERRE_FINAL,         // Modo D: aproximacion estricta, sin pivote
};

struct DecisionAngular {
  ModoRecuperacion modo = ModoRecuperacion::CONTINUO_SUAVE;
  float modLadoIzq = 1.0f;   // factor multiplicativo [0.50 .. 1.10]
  float modLadoDer = 1.0f;
  bool solicitarPivote = false;
};

inline float limitarFactor(float valor, float minimo, float maximo) {
  return valor < minimo ? minimo : (valor > maximo ? maximo : valor);
}

// Entradas puras: error angular (grados con signo), distancia restante, tiempo
// sostenido sobre el umbral transitorio, y los cortes de fase.
inline DecisionAngular evaluarLazoAngular(float errorAngDeg, float restanteDistCm,
                                          uint32_t msEnError,
                                          float umbralContinuoDeg,
                                          float umbralTransitorioDeg,
                                          float distanciaCierreCm,
                                          uint32_t msHisteresis) {
  DecisionAngular decision;
  const float errAbs = fabsf(errorAngDeg);

  if (restanteDistCm <= distanciaCierreCm) {
    decision.modo = ModoRecuperacion::CIERRE_FINAL;
    return decision;
  }

  if (errAbs <= umbralContinuoDeg) {
    decision.modo = ModoRecuperacion::CONTINUO_SUAVE;
    // Proporcional suave: el error relativo modula hasta +-15% de par
    // diferencial. err > 0 frena el lado derecho; err < 0 frena el izquierdo.
    const float correccion = (umbralContinuoDeg > 0.0f)
        ? (errorAngDeg / umbralContinuoDeg) * 0.15f
        : 0.0f;
    decision.modLadoIzq = limitarFactor(1.0f + correccion, 0.85f, 1.15f);
    decision.modLadoDer = limitarFactor(1.0f - correccion, 0.85f, 1.15f);
    return decision;
  }

  if (errAbs <= umbralTransitorioDeg) {
    decision.modo = ModoRecuperacion::FRENADO_TRANSITORIO;
    // Retencion fuerte del lado interno; el externo empuja para reorientar
    // sin detener el avance general.
    if (errorAngDeg > 0.0f) {
      decision.modLadoIzq = 1.10f;
      decision.modLadoDer = 0.55f;
    } else {
      decision.modLadoIzq = 0.55f;
      decision.modLadoDer = 1.10f;
    }
    return decision;
  }

  // |err| > umbral transitorio: solo se autoriza la pausa con pivote tras la
  // histeresis temporal; antes se mantiene la retencion diferencial fuerte.
  decision.solicitarPivote = msEnError >= msHisteresis;
  decision.modo = decision.solicitarPivote ? ModoRecuperacion::PAUSA_PIVOTE
                                           : ModoRecuperacion::FRENADO_TRANSITORIO;
  if (errorAngDeg > 0.0f) {
    decision.modLadoIzq = 1.10f;
    decision.modLadoDer = decision.solicitarPivote ? 0.0f : 0.50f;
  } else {
    decision.modLadoIzq = decision.solicitarPivote ? 0.0f : 0.50f;
    decision.modLadoDer = 1.10f;
  }
  return decision;
}

}  // namespace ControlAngular
