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

inline bool desviacionFueraDeRango(float errorLateralCm, float maximoCm) {
  return fabsf(errorLateralCm) > maximoCm;
}

inline bool debeRecentrar(float errorLateralCm, uint32_t persistenciaMs,
                          float umbralCm, uint32_t esperaMs,
                          float crecimientoCm, uint32_t crecimientoMs,
                          float umbralCrecimientoCm, uint32_t esperaCrecimientoMs) {
  return (fabsf(errorLateralCm) >= umbralCm && persistenciaMs >= esperaMs) ||
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

}  // namespace ControlRuta
