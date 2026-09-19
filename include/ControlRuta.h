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

inline float correccionLateralParaDireccion(float correccionRumboDeg, int /*direccion*/) {
  // El ángulo de corrección del cuerpo respecto a la línea de trayecto conserva
  // el mismo sentido físico en avance y reversa. La inversión motriz correspondiente
  // al frenado diferencial se aplica en frenarLadoIzquierdoParaRumbo.
  return correccionRumboDeg;
}

// Invertir ambas ruedas también invierte el giro físico al frenar sólo un
// lado. Esta función conserva el signo de corrección calibrado del yaw.
inline bool frenarLadoIzquierdoParaRumbo(int candidatoGiro, int direccion) {
  return (candidatoGiro < 0) != (direccion < 0);
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

// Traslacion parasita en giro de radio cero. En un chasis 4WD sin suspension,
// los 4 apoyos sobrerrestringen el plano y el peso se concentra en una diagonal;
// ademas los motores TT no entregan la misma fuerza. El centro instantaneo de
// rotacion (ICR) queda fuera del centro geometrico y, al girar dTheta, el centro
// del chasis se desplaza:
//   dx = -y_icr * dTheta
//   dy =  x_icr * dTheta
// La correccion se aplica solo en giro puro (pivote); en arcos y avance recto
// el ICR efectivo es otro y no se compensa.
struct CorreccionICR {
  float dxCm;
  float dyCm;
};

inline CorreccionICR corregirTraslacionParasita(float xIcrCm, float yIcrCm,
                                                float deltaThetaRad) {
  return {-yIcrCm * deltaThetaRad, xIcrCm * deltaThetaRad};
}

// Invarianza de marco local: la recta directriz del tramo se ancla en la pose
// real del robot al iniciar el avance, no en un origen deducido del waypoint.
// Asi el desvio lateral al instante t=0 es exactamente cero para cualquier
// rumbo theta y el tramo no hereda el error residual de pasos anteriores
// (elimina el "efecto cangrejo"). El objetivo euclidiano se conserva respecto
// al waypoint real para no relajar la aceptacion del endpoint.
inline ErroresTrayectoria calcularErroresTrayectoriaAnclado(
    float posicionXCm, float posicionYCm, float origenXCm, float origenYCm,
    float objetivoXCm, float objetivoYCm, float rumboPlanificadoDeg,
    float distanciaPlanificadaCm) {
  constexpr float kPi = 3.14159265358979323846f;
  const float rumboRad = rumboPlanificadoDeg * kPi / 180.0f;
  const float ux = sinf(rumboRad);
  const float uy = cosf(rumboRad);
  const float dx = posicionXCm - origenXCm;
  const float dy = posicionYCm - origenYCm;
  const float recorridoLongitudinal = dx * ux + dy * uy;
  return {
      distanciaPlanificadaCm - recorridoLongitudinal,
      dx * uy - dy * ux,
      hypotf(objetivoXCm - posicionXCm, objetivoYCm - posicionYCm),
  };
}

// Variante historica: deduce el origen de la recta a partir del waypoint y la
// distancia planificada. Conserva el comportamiento previo para las pruebas y
// los llamadores que no anclan el tramo.
inline ErroresTrayectoria calcularErroresTrayectoria(
    float posicionXCm, float posicionYCm, float objetivoXCm, float objetivoYCm,
    float rumboPlanificadoDeg, float distanciaPlanificadaCm) {
  constexpr float kPi = 3.14159265358979323846f;
  const float rumboRad = rumboPlanificadoDeg * kPi / 180.0f;
  const float ux = sinf(rumboRad);
  const float uy = cosf(rumboRad);
  return calcularErroresTrayectoriaAnclado(
      posicionXCm, posicionYCm, objetivoXCm - distanciaPlanificadaCm * ux,
      objetivoYCm - distanciaPlanificadaCm * uy, objetivoXCm, objetivoYCm,
      rumboPlanificadoDeg, distanciaPlanificadaCm);
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

// Una corrección menor que la tolerancia más el arrastre máximo no es una
// maniobra repetible: el robot podría volver a pasarse y acumular giros. Se
// detiene, conserva la evidencia y solicita calibración en vez de inventar un
// pivote de 180° para corregir unos pocos centímetros.
inline DecisionEndpoint decidirEndpointSeguro(bool objetivoAbsoluto, bool endpointAceptado,
                                              uint8_t intentosRealizados, uint8_t maximoIntentos,
                                              float distanciaErrorCm,
                                              float distanciaMinimaRecuperableCm) {
  const DecisionEndpoint decision = decidirEndpoint(
      objetivoAbsoluto, endpointAceptado, intentosRealizados, maximoIntentos);
  return decision == DecisionEndpoint::RECUPERAR &&
             distanciaErrorCm < distanciaMinimaRecuperableCm
      ? DecisionEndpoint::CALIBRAR
      : decision;
}

struct SalidaGiroBilateral {
  int pwmL;
  int pwmR;
  float compensacionPwm;
  float desplazamientoCentroCm;
};

// Control cinemático diferencial en giro sobre el propio eje (P_C).
// Garantiza v_C = (v_R + v_L)/2 = 0 regulando en lazo cerrado el error
// traslacional del centro del chasis para evitar giros tangenciales.
inline SalidaGiroBilateral balancearGiroDiferencial(
    int pwmBase, int cand, float ticksIzq, float ticksDer,
    float cmPorTick, float kp, float limitePwm, int maxPwm) {
  if (cand == 0 || pwmBase <= 0) {
    return {0, 0, 0.0f, 0.0f};
  }
  const float signoL = cand > 0 ? 1.0f : -1.0f;
  const float signoR = cand > 0 ? -1.0f : 1.0f;
  const float dispL = signoL * ticksIzq * cmPorTick;
  const float dispR = signoR * ticksDer * cmPorTick;
  const float despCentroCm = (dispL + dispR) * 0.5f;
  const float compensacionPwm = limitar(despCentroCm * kp, -limitePwm, limitePwm);
  const int comp = static_cast<int>(roundf(compensacionPwm));

  SalidaGiroBilateral salida;
  salida.desplazamientoCentroCm = despCentroCm;
  salida.compensacionPwm = compensacionPwm;
  if (cand > 0) {
    salida.pwmL = static_cast<int>(limitar(static_cast<float>(pwmBase - comp), 0.0f, static_cast<float>(maxPwm)));
    salida.pwmR = static_cast<int>(limitar(static_cast<float>(-pwmBase - comp), static_cast<float>(-maxPwm), 0.0f));
  } else {
    salida.pwmL = static_cast<int>(limitar(static_cast<float>(-pwmBase - comp), static_cast<float>(-maxPwm), 0.0f));
    salida.pwmR = static_cast<int>(limitar(static_cast<float>(pwmBase - comp), 0.0f, static_cast<float>(maxPwm)));
  }
  return salida;
}

}  // namespace ControlRuta

