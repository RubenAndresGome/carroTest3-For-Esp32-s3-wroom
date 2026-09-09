#pragma once

#include <cstdint>

namespace ControlCalibracion {

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
