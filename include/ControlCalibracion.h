#pragma once

#include <cstdint>

namespace ControlCalibracion {

struct EvaluacionEncoders {
  int64_t promedioIzquierdo = 0;
  int64_t promedioDerecho = 0;
  bool ladoIzquierdoValido = false;
  bool ladoDerechoValido = false;
  bool responde[4] = {false, false, false, false};
  bool sinRespuestaAislada[4] = {false, false, false, false};
};

inline EvaluacionEncoders evaluarEncoders(const int64_t deltas[4],
                                          int64_t ticksMinimos) {
  EvaluacionEncoders resultado;
  resultado.promedioIzquierdo = (deltas[0] + deltas[2]) / 2;
  resultado.promedioDerecho = (deltas[1] + deltas[3]) / 2;
  resultado.ladoIzquierdoValido = resultado.promedioIzquierdo >= ticksMinimos;
  resultado.ladoDerechoValido = resultado.promedioDerecho >= ticksMinimos;

  for (int i = 0; i < 4; ++i) {
    resultado.responde[i] = deltas[i] >= ticksMinimos;
  }

  // La pareja debe aportar el doble del umbral para demostrar que el lado se
  // movio aun cuando una fuente individual permanezca en cero. La aceptacion
  // de la calibracion conserva el promedio de ambos encoders por lado.
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
