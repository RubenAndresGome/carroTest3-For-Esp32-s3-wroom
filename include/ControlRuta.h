#pragma once

#include <cstdint>
#include <cmath>

// Cálculos deterministas y sin hardware del control de convergencia. Esta
// unidad se comparte entre Cinematica.cpp y las pruebas Unity para que los
// signos, límites y criterios de aceptación se validen sobre la misma lógica.
namespace ControlRuta {

struct ErroresTrayectoria {
  float longitudinalCm;
  float lateralCm;
  float euclidianoCm;
};

struct VectorPlano {
  float x;
  float y;
};

struct PuntoReingreso {
  float xCm;
  float yCm;
  float avanceDesdeProyeccionCm;
};

struct CandidatoReingreso {
  bool valido;
  int direccion;
  float rumboTrayectoDeg;
  float rumboCuerpoDeg;
  float giroHaciaLineaDeg;
  float giroRetornoDeg;
  float distanciaCm;
  float coste;
};

// Seguimiento de una maniobra de traslación concreta. La magnitud observada
// siempre es la distancia restante a su objetivo activo (segmento, punto de
// reingreso o endpoint), por lo que una reversa convergente también disminuye.
struct SeguimientoProgreso {
  bool inicializado = false;
  float mejorRestanteCm = 0.0f;
  bool divergenciaActiva = false;
  uint32_t inicioDivergenciaMs = 0;
};

struct EpisodioRecuperacion {
  bool activo = false;
  uint32_t inicioMs = 0;
};

enum class EstadoEpisodioRecuperacion : uint8_t {
  INACTIVO,
  ACTIVO,
  VENCIDO,
  CERRADO,
};

struct EstadoPI {
  float integralGradoS = 0.0f;
};

struct SalidaPI {
  float p = 0.0f;
  float i = 0.0f;
  float d = 0.0f;
  float total = 0.0f;
  float integralGradoS = 0.0f;
};

enum class DecisionEndpoint : uint8_t {
  COMPLETAR,
  RECUPERAR,
  CALIBRAR,
  FALLAR,
};

inline float limitar(float valor, float minimo, float maximo) {
  return valor < minimo ? minimo : (valor > maximo ? maximo : valor);
}

inline float normalizar360(float grados) {
  grados = fmodf(grados, 360.0f);
  return grados < 0.0f ? grados + 360.0f : grados;
}

inline float errorAngularDeg(float objetivoDeg, float actualDeg) {
  float error = normalizar360(objetivoDeg) - normalizar360(actualDeg);
  if (error > 180.0f) error -= 360.0f;
  if (error <= -180.0f) error += 360.0f;
  return error;
}

inline bool reversaAutomatica(float rumboTrayectoDeg, float rumboActualDeg,
                              float umbralDeg) {
  return fabsf(errorAngularDeg(rumboTrayectoDeg, rumboActualDeg)) >= umbralDeg;
}

inline float rumboCuerpoParaTrayecto(float rumboTrayectoDeg, int direccion) {
  return normalizar360(rumboTrayectoDeg + (direccion < 0 ? 180.0f : 0.0f));
}

// En reversa el chasis debe conservar el rumbo de su cuerpo al finalizar la traslación,
// evitando pivotes parásitos (giro_fin) que degraden la posición por arrastre lateral (skid-steer scrubbing).
inline float rumboFinalParaPaso(float rumboTrayectoDeg, int direccion, float rumboCuerpoDeg) {
  return direccion < 0 ? normalizar360(rumboCuerpoDeg) : normalizar360(rumboTrayectoDeg);
}

inline float correccionLateralParaDireccion(float correccionRumboDeg, int direccion) {
  return direccion < 0 ? -correccionRumboDeg : correccionRumboDeg;
}

// Decide qué lado reducir a partir del marco cartesiano, sin depender del
// signo eléctrico que haya aprendido la calibración del pivote.
inline bool frenarLadoIzquierdoParaRumbo(float correccionRumbo, int direccion) {
  // +yaw es horario: en avance se frena la derecha; en reversa se intercambia.
  // La decisión geométrica no depende del candidato eléctrico aprendido.
  return (correccionRumbo < 0.0f) != (direccion < 0);
}

inline VectorPlano vectorUnitarioRumbo(float rumboDeg) {
  constexpr float kPi = 3.14159265358979323846f;
  const float rumboRad = normalizar360(rumboDeg) * kPi / 180.0f;
  return {sinf(rumboRad), cosf(rumboRad)};
}

inline float distanciaPorTick(float diametroEfectivoCm, int pulsosPorRevolucion) {
  return pulsosPorRevolucion > 0
      ? 3.14159265358979323846f * diametroEfectivoCm / static_cast<float>(pulsosPorRevolucion)
      : 0.0f;
}

inline float distanciaFrenoPrevista(float pwm, float baseCm, float cmPorPwm,
                                    float maximoCm) {
  return limitar(baseCm + fabsf(pwm) * cmPorPwm, baseCm, maximoCm);
}

inline float distanciaConBaseline(float acumuladaCm, float ticksDesdeBaseline,
                                  float cmPorTick) {
  return fmaxf(0.0f, acumuladaCm) +
         fmaxf(0.0f, ticksDesdeBaseline) * fmaxf(0.0f, cmPorTick);
}

inline float distanciaRestanteObjetivoActivo(
    float errorLongitudinalCm, float distanciaEndpointCm,
    float distanciaReingresoCm, bool recuperacionEndpoint,
    bool avanceReingreso) {
  if (avanceReingreso) return fmaxf(0.0f, distanciaReingresoCm);
  if (recuperacionEndpoint) return fmaxf(0.0f, distanciaEndpointCm);
  return fabsf(errorLongitudinalCm);
}

inline float umbralLateralSegmento(float longitudSegmentoCm, float proporcion,
                                   float pisoCm) {
  const float longitud = fmaxf(0.0f, longitudSegmentoCm);
  return fmaxf(pisoCm, longitud * proporcion);
}

inline float proporcionLateralSegmento(float errorLateralCm,
                                       float longitudSegmentoCm) {
  const float denominador = fmaxf(longitudSegmentoCm, 0.1f);
  return fabsf(errorLateralCm) / denominador;
}

inline bool lateralEnRango(float errorLateralCm, float longitudSegmentoCm,
                           float proporcion, float pisoCm) {
  return fabsf(errorLateralCm) <=
         umbralLateralSegmento(longitudSegmentoCm, proporcion, pisoCm);
}

inline bool lateralRecuperado(float errorLateralCm, float longitudSegmentoCm,
                              float proporcionSalida, float pisoCm) {
  return fabsf(errorLateralCm) <=
         umbralLateralSegmento(longitudSegmentoCm, proporcionSalida, pisoCm);
}

// Cruzar el eje con un error pequeño no requiere consumir un intento de
// recuperación: basta frenar, limpiar el integral y recuperar el rumbo de la
// línea. Se mantiene aquí para que firmware y pruebas compartan exactamente
// el mismo criterio de signo y tolerancia.
inline bool cruzoEjeConErrorBajo(float errorAnteriorCm, float errorActualCm,
                                 float toleranciaCm) {
  return errorAnteriorCm * errorActualCm < 0.0f &&
         fabsf(errorActualCm) <= toleranciaCm;
}

inline ErroresTrayectoria calcularErroresTrayectoria(
    float posicionXCm, float posicionYCm, float objetivoXCm, float objetivoYCm,
    float rumboPlanificadoDeg, float distanciaPlanificadaCm) {
  const VectorPlano eje = vectorUnitarioRumbo(rumboPlanificadoDeg);
  const float ux = eje.x;
  const float uy = eje.y;
  const float inicioXCm = objetivoXCm - distanciaPlanificadaCm * ux;
  const float inicioYCm = objetivoYCm - distanciaPlanificadaCm * uy;
  const float dx = posicionXCm - inicioXCm;
  const float dy = posicionYCm - inicioYCm;
  const float recorridoLongitudinal = dx * ux + dy * uy;
  return {
      distanciaPlanificadaCm - recorridoLongitudinal,
      dx * uy - dy * ux,
      hypotf(objetivoXCm - posicionXCm, objetivoYCm - posicionYCm),
  };
}

inline PuntoReingreso calcularPuntoReingreso(
    float posicionXCm, float posicionYCm, float objetivoXCm, float objetivoYCm,
    float rumboPlanificadoDeg, float distanciaPlanificadaCm,
    float errorLateralCm, float adelantoMinimoCm) {
  const VectorPlano eje = vectorUnitarioRumbo(rumboPlanificadoDeg);
  const float inicioXCm = objetivoXCm - distanciaPlanificadaCm * eje.x;
  const float inicioYCm = objetivoYCm - distanciaPlanificadaCm * eje.y;
  const float dx = posicionXCm - inicioXCm;
  const float dy = posicionYCm - inicioYCm;
  const float proyeccion = limitar(dx * eje.x + dy * eje.y, 0.0f,
                                   distanciaPlanificadaCm);
  const float adelanto = fmaxf(adelantoMinimoCm, 2.0f * fabsf(errorLateralCm));
  const float recorridoObjetivo = limitar(proyeccion + adelanto, 0.0f,
                                          distanciaPlanificadaCm);
  return {
      inicioXCm + recorridoObjetivo * eje.x,
      inicioYCm + recorridoObjetivo * eje.y,
      recorridoObjetivo - proyeccion,
  };
}

inline float rumboEntrePuntosDeg(float origenXCm, float origenYCm,
                                 float destinoXCm, float destinoYCm,
                                 float rumboAlternativoDeg) {
  const float dx = destinoXCm - origenXCm;
  const float dy = destinoYCm - origenYCm;
  return hypotf(dx, dy) > 0.001f
      ? normalizar360(atan2f(dx, dy) * 180.0f / 3.14159265358979323846f)
      : normalizar360(rumboAlternativoDeg);
}

inline float costeReingreso(float distanciaCm, float longitudSegmentoCm,
                            float giroHaciaLineaDeg, float giroRetornoDeg,
                            bool reversa, float penalizacionReversa = 0.10f) {
  const float denominador = fmaxf(longitudSegmentoCm, 10.0f);
  const float costeTraslacion = fmaxf(0.0f, distanciaCm) / denominador;
  const float costeGiro =
      (fabsf(giroHaciaLineaDeg) + fabsf(giroRetornoDeg)) / 360.0f;
  return costeTraslacion + costeGiro + (reversa ? penalizacionReversa : 0.0f);
}

inline CandidatoReingreso evaluarCandidatoReingreso(
    float posicionXCm, float posicionYCm, const PuntoReingreso& punto,
    float rumboSegmentoDeg, float longitudSegmentoCm, float headingActualDeg,
    int direccion, float rumboRetornoCuerpoDeg) {
  CandidatoReingreso candidato{};
  candidato.direccion = direccion < 0 ? -1 : 1;
  candidato.distanciaCm = hypotf(punto.xCm - posicionXCm,
                                 punto.yCm - posicionYCm);
  candidato.rumboTrayectoDeg = rumboEntrePuntosDeg(
      posicionXCm, posicionYCm, punto.xCm, punto.yCm, rumboSegmentoDeg);
  candidato.rumboCuerpoDeg = rumboCuerpoParaTrayecto(
      candidato.rumboTrayectoDeg, candidato.direccion);
  candidato.giroHaciaLineaDeg = errorAngularDeg(
      candidato.rumboCuerpoDeg, headingActualDeg);
  candidato.giroRetornoDeg = errorAngularDeg(
      rumboRetornoCuerpoDeg, candidato.rumboCuerpoDeg);
  candidato.coste = costeReingreso(
      candidato.distanciaCm, longitudSegmentoCm,
      candidato.giroHaciaLineaDeg, candidato.giroRetornoDeg,
      candidato.direccion < 0);
  candidato.valido = isfinite(candidato.distanciaCm) &&
                     candidato.distanciaCm >= 0.5f;
  return candidato;
}

inline CandidatoReingreso evaluarCandidatoReingreso(
    float posicionXCm, float posicionYCm, const PuntoReingreso& punto,
    float rumboSegmentoDeg, float longitudSegmentoCm, float headingActualDeg,
    int direccion, int direccionPlanificada) {
  return evaluarCandidatoReingreso(
      posicionXCm, posicionYCm, punto, rumboSegmentoDeg, longitudSegmentoCm,
      headingActualDeg, direccion,
      rumboCuerpoParaTrayecto(rumboSegmentoDeg, direccionPlanificada));
}

inline CandidatoReingreso elegirCandidatoReingreso(
    const CandidatoReingreso& avance, const CandidatoReingreso& reversa) {
  if (!avance.valido) return reversa;
  if (!reversa.valido) return avance;
  // Un empate favorece avance: mantiene la semántica histórica y evita una
  // inversión innecesaria del chasis.
  return reversa.coste + 1e-4f < avance.coste ? reversa : avance;
}

inline bool desviacionFueraDeRango(float errorLateralCm, float maximoCm) {
  return fabsf(errorLateralCm) > maximoCm;
}

inline bool debeRecentrar(float errorLateralCm, uint32_t persistenciaMs,
                          float umbralCm, uint32_t esperaMs,
                          float crecimientoCm, uint32_t crecimientoMs,
                          float umbralCrecimientoCm, uint32_t esperaCrecimientoMs) {
  return (fabsf(errorLateralCm) > umbralCm && persistenciaMs >= esperaMs) ||
         (crecimientoCm >= umbralCrecimientoCm &&
          crecimientoMs >= esperaCrecimientoMs);
}

inline bool progresoEnSentidoIncorrecto(float errorLongitudinalCm,
                                        float mejorErrorLongitudinalCm,
                                        uint32_t persistenciaMs,
                                        float crecimientoMaxCm,
                                        uint32_t esperaMs) {
  return errorLongitudinalCm >= mejorErrorLongitudinalCm + crecimientoMaxCm &&
         persistenciaMs >= esperaMs;
}

inline void iniciarSeguimientoProgreso(SeguimientoProgreso& seguimiento,
                                       float distanciaRestanteCm) {
  seguimiento.inicializado = true;
  seguimiento.mejorRestanteCm = fmaxf(0.0f, distanciaRestanteCm);
  seguimiento.divergenciaActiva = false;
  seguimiento.inicioDivergenciaMs = 0;
}

inline bool actualizarSeguimientoProgreso(
    SeguimientoProgreso& seguimiento, float distanciaRestanteCm,
    uint32_t ahoraMs, float crecimientoMaxCm, uint32_t esperaMs) {
  const float restante = fmaxf(0.0f, distanciaRestanteCm);
  if (!seguimiento.inicializado) {
    iniciarSeguimientoProgreso(seguimiento, restante);
    return false;
  }
  if (restante < seguimiento.mejorRestanteCm) {
    seguimiento.mejorRestanteCm = restante;
    seguimiento.divergenciaActiva = false;
  } else if (restante >= seguimiento.mejorRestanteCm + crecimientoMaxCm) {
    if (!seguimiento.divergenciaActiva) {
      seguimiento.divergenciaActiva = true;
      seguimiento.inicioDivergenciaMs = ahoraMs;
    }
  } else {
    seguimiento.divergenciaActiva = false;
  }
  return seguimiento.divergenciaActiva &&
         static_cast<uint32_t>(ahoraMs - seguimiento.inicioDivergenciaMs) >=
             esperaMs;
}

inline void abrirEpisodioRecuperacion(EpisodioRecuperacion& episodio,
                                      uint32_t ahoraMs) {
  if (!episodio.activo) {
    episodio.activo = true;
    episodio.inicioMs = ahoraMs;
  }
}

inline EstadoEpisodioRecuperacion procesarEpisodioRecuperacion(
    EpisodioRecuperacion& episodio, uint32_t ahoraMs, uint32_t presupuestoMs,
    bool solicitarCierre = false) {
  if (!episodio.activo) return EstadoEpisodioRecuperacion::INACTIVO;
  // La resta unsigned conserva la semántica correcta al cruzar rollover y el
  // vencimiento tiene prioridad sobre una transición que intentaría cerrar.
  if (static_cast<uint32_t>(ahoraMs - episodio.inicioMs) >= presupuestoMs) {
    return EstadoEpisodioRecuperacion::VENCIDO;
  }
  if (solicitarCierre) {
    episodio.activo = false;
    return EstadoEpisodioRecuperacion::CERRADO;
  }
  return EstadoEpisodioRecuperacion::ACTIVO;
}

inline void cancelarEpisodioRecuperacion(EpisodioRecuperacion& episodio) {
  episodio.activo = false;
}

inline bool reingresoAceptable(float errorLateralCm, float distanciaReingresoCm,
                               float toleranciaLateralCm,
                               float toleranciaPuntoCm) {
  return fabsf(errorLateralCm) <= toleranciaLateralCm &&
         distanciaReingresoCm <= toleranciaPuntoCm;
}

inline bool reingresoDivergente(float errorLateralInicialCm,
                                float errorLateralActualCm,
                                float distanciaRecorridaCm,
                                uint32_t transcurridoMs,
                                float mejoraMinimaCm,
                                float distanciaEvaluacionCm,
                                uint32_t tiempoEvaluacionMs) {
  const bool evaluar = transcurridoMs >= tiempoEvaluacionMs ||
                       distanciaRecorridaCm >= distanciaEvaluacionCm;
  return evaluar &&
         fabsf(errorLateralActualCm) >
             fmaxf(0.0f, fabsf(errorLateralInicialCm) - mejoraMinimaCm);
}

inline float correccionLateralRumboDeg(float errorLateralCm, float gananciaDegPorCm,
                                       float limiteDeg) {
  // Lateral positivo equivale a estar a la derecha de la ruta; el rumbo debe
  // disminuir para volver a la línea (0° = +Y, 90° = +X).
  return limitar(-errorLateralCm * gananciaDegPorCm, -limiteDeg, limiteDeg);
}

// Calcula la reducción dinámica de PWM sobre el lado más veloz/empujador en avance recto.
// Proporcional al error angular y acotada estrictamente a un ratio máximo (ej. 30%) del PWM base.
inline int calcularReduccionAsimetriaPwm(int pwmBase, float errorRumboDeg,
                                         float kpAsimetriaPorGrado,
                                         float maximoRatioReduccion) {
  if (pwmBase <= 0 || kpAsimetriaPorGrado <= 0.0f || maximoRatioReduccion <= 0.0f) {
    return 0;
  }
  const float ratio = fminf(maximoRatioReduccion, fabsf(errorRumboDeg) * kpAsimetriaPorGrado);
  return static_cast<int>(lroundf(static_cast<float>(pwmBase) * ratio));
}

inline SalidaPI actualizarPI(EstadoPI& estado, float errorDeg, float gyroRadS,
                             float dtS, float kp, float ki, float kd,
                             float limitePwm, float limiteIntegralGradoS) {
  const float p = errorDeg * kp;
  const float d = -gyroRadS * kd;
  const float candidata = limitar(estado.integralGradoS + errorDeg * dtS,
                                  -limiteIntegralGradoS, limiteIntegralGradoS);
  const float sinLimite = p + candidata * ki + d;
  const bool reduceSaturacion =
      (sinLimite > limitePwm && errorDeg < 0.0f) ||
      (sinLimite < -limitePwm && errorDeg > 0.0f);
  if (fabsf(sinLimite) <= limitePwm || reduceSaturacion) {
    estado.integralGradoS = candidata;
  }
  const float i = estado.integralGradoS * ki;
  SalidaPI salida;
  salida.p = p;
  salida.i = i;
  salida.d = d;
  salida.total = limitar(p + i + d, -limitePwm, limitePwm);
  salida.integralGradoS = estado.integralGradoS;
  return salida;
}

inline bool endpointAceptable(float errorLateralCm, float errorEuclidianoCm,
                              float errorYawDeg, float toleranciaEndpointCm,
                              float toleranciaYawDeg) {
  return fabsf(errorLateralCm) <= toleranciaEndpointCm &&
         errorEuclidianoCm <= toleranciaEndpointCm &&
         fabsf(errorYawDeg) <= toleranciaYawDeg;
}



inline bool agotoIntentosEndpoint(uint8_t intentosRealizados, uint8_t maximoIntentos) {
  return intentosRealizados >= maximoIntentos;
}

inline DecisionEndpoint decidirEndpoint(bool objetivoAbsoluto, bool endpointAceptado,
                                        uint8_t intentosRealizados, uint8_t maximoIntentos) {
  if (!objetivoAbsoluto || endpointAceptado) return DecisionEndpoint::COMPLETAR;
  return agotoIntentosEndpoint(intentosRealizados, maximoIntentos)
      ? DecisionEndpoint::FALLAR
      : DecisionEndpoint::RECUPERAR;
}

// Los waypoints absolutos son estrictos. Un residual demasiado corto para una
// maniobra repetible se bloquea; nunca se convierte en una finalización suave.
inline DecisionEndpoint decidirEndpointSeguro(bool objetivoAbsoluto, bool endpointAceptado,
                                              uint8_t intentosRealizados, uint8_t maximoIntentos,
                                              float distanciaErrorCm,
                                              float distanciaMinimaRecuperableCm) {
  const DecisionEndpoint decision = decidirEndpoint(
      objetivoAbsoluto, endpointAceptado, intentosRealizados, maximoIntentos);
  return decision == DecisionEndpoint::RECUPERAR &&
             distanciaErrorCm < distanciaMinimaRecuperableCm
      ? DecisionEndpoint::FALLAR : decision;
}

// --- Sistema de micro-pulsos deterministas de alto par con interlocks ---
struct SalidaPulso {
  int pwmIzquierdo = 0;
  int pwmDerecho = 0;
  int correccionDiferencial = 0;
};

inline bool tramoRequiereModoPulsado(float distanciaRestanteCm, float umbralPulsadoCm) {
  return distanciaRestanteCm > 0.0f && distanciaRestanteCm <= umbralPulsadoCm;
}

inline SalidaPulso calcularPulsoTraccion(int pwmBase, float errorRumboDeg,
                                         float kpRumbo, int diffMax, int pwmMaxLimit) {
  int diff = static_cast<int>(lroundf(errorRumboDeg * kpRumbo));
  if (diff > diffMax) diff = diffMax;
  if (diff < -diffMax) diff = -diffMax;

  int pwmL = pwmBase - diff;
  int pwmR = pwmBase + diff;
  if (pwmL < 0) pwmL = 0;
  if (pwmL > pwmMaxLimit) pwmL = pwmMaxLimit;
  if (pwmR < 0) pwmR = 0;
  if (pwmR > pwmMaxLimit) pwmR = pwmMaxLimit;

  SalidaPulso s;
  s.pwmIzquierdo = pwmL;
  s.pwmDerecho = pwmR;
  s.correccionDiferencial = diff;
  return s;
}

inline bool interlockFinPulsado(float distanciaRestanteCm, float toleranciaCm,
                                float distanciaPorPulsoPromedioCm) {
  return distanciaRestanteCm <= toleranciaCm ||
         (distanciaPorPulsoPromedioCm > 0.0f && distanciaRestanteCm <= distanciaPorPulsoPromedioCm * 0.45f);
}

inline float actualizarMemoriaImpulso(float memoriaActualCm, float deltaMedidoCm) {
  if (deltaMedidoCm >= 0.2f && deltaMedidoCm <= 5.0f) {
    return 0.7f * memoriaActualCm + 0.3f * deltaMedidoCm;
  }
  return memoriaActualCm;
}

}  // namespace ControlRuta
