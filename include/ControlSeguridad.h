#pragma once

#include <cmath>
#include <cstdint>

// Predicados sin hardware compartidos por el súper-ciclo y las pruebas
// nativas. Mantenerlos puros evita que las pruebas dependan de PWM, PCNT o
// una placa conectada, sin cambiar la acción segura que toma el firmware.
namespace ControlSeguridad {

inline bool imuApta(bool presente, bool obsoleta) {
  return presente && !obsoleta;
}

inline bool estopSolicitado(bool solicitado) {
  return solicitado;
}

inline bool encoderEsOutlier(int64_t delta, float mediana, float desacuerdoMaximo) {
  return fabsf(static_cast<float>(delta) - mediana) /
             fmaxf(1.0f, fabsf(mediana)) > desacuerdoMaximo;
}

struct ClasificacionEncoders {
  bool confiable[4] = {true, true, true, true};
  float mediana = 0.0f;
  bool modoDegradado = false;
  bool ladoIzquierdoValido = true;
  bool ladoDerechoValido = true;
};

// Estimador robusto para FL/FR/BL/BR. La mediana de cuatro ignora un canal
// aislado en cero o un pico; después se conserva sólo el canal coherente de
// cada lado. Si un lado completo queda sin fuente, el llamador debe parar.
inline float medianaCuatro(const int64_t valores[4]) {
  int64_t ordenados[4] = {valores[0], valores[1], valores[2], valores[3]};
  for (int i = 1; i < 4; ++i) {
    const int64_t actual = ordenados[i];
    int j = i - 1;
    while (j >= 0 && ordenados[j] > actual) {
      ordenados[j + 1] = ordenados[j];
      --j;
    }
    ordenados[j + 1] = actual;
  }
  return 0.5f * static_cast<float>(ordenados[1] + ordenados[2]);
}

inline ClasificacionEncoders clasificarEncoders(const int64_t valores[4],
                                                 float desacuerdoMaximo) {
  ClasificacionEncoders resultado;
  resultado.mediana = medianaCuatro(valores);
  int coincidentes = 0;
  for (int i = 0; i < 4; ++i) {
    if (!encoderEsOutlier(valores[i], resultado.mediana, desacuerdoMaximo)) {
      ++coincidentes;
    }
  }

  // Dos ceros y dos lecturas sanas dejan la mediana simple entre ambos grupos.
  // Elegir entonces el grupo más numeroso; ante empate se prefiere la señal de
  // mayor magnitud, porque un cero bajo PWM representa ausencia de pulsos.
  if (coincidentes < 2) {
    float mejorReferencia = 0.0f;
    int mejorCantidad = -1;
    for (int candidato = 0; candidato < 4; ++candidato) {
      int cantidad = 0;
      float suma = 0.0f;
      for (int i = 0; i < 4; ++i) {
        if (!encoderEsOutlier(valores[i], static_cast<float>(valores[candidato]),
                              desacuerdoMaximo)) {
          ++cantidad;
          suma += valores[i];
        }
      }
      const float referencia = cantidad ? suma / cantidad : 0.0f;
      if (cantidad > mejorCantidad ||
          (cantidad == mejorCantidad && fabsf(referencia) > fabsf(mejorReferencia))) {
        mejorCantidad = cantidad;
        mejorReferencia = referencia;
      }
    }
    resultado.mediana = mejorReferencia;
  }
  for (int i = 0; i < 4; ++i) {
    resultado.confiable[i] = !encoderEsOutlier(valores[i], resultado.mediana,
                                                desacuerdoMaximo);
    resultado.modoDegradado |= !resultado.confiable[i];
  }
  resultado.ladoIzquierdoValido = resultado.confiable[0] || resultado.confiable[2];
  resultado.ladoDerechoValido = resultado.confiable[1] || resultado.confiable[3];
  return resultado;
}

inline float promedioConfiableLado(const int64_t valores[4],
                                   const bool confiable[4], bool izquierdo) {
  const int primero = izquierdo ? 0 : 1;
  const int segundo = izquierdo ? 2 : 3;
  float suma = 0.0f;
  int cantidad = 0;
  if (confiable[primero]) { suma += valores[primero]; ++cantidad; }
  if (confiable[segundo]) { suma += valores[segundo]; ++cantidad; }
  return cantidad ? suma / cantidad : 0.0f;
}

inline bool ladoEnStall(bool ladoExigido, bool pulsoFrontalCero,
                        bool pulsoPosteriorCero) {
  return ladoExigido && pulsoFrontalCero && pulsoPosteriorCero;
}

}  // namespace ControlSeguridad
