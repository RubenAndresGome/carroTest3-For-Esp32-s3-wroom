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

inline bool deltaEncoderPlausible(int64_t delta, int64_t maximoAbsoluto) {
  return maximoAbsoluto > 0 && delta >= -maximoAbsoluto && delta <= maximoAbsoluto;
}

inline int64_t saturarDeltaEncoder(int64_t delta, int64_t maximoAbsoluto) {
  if (maximoAbsoluto <= 0) return 0;
  if (delta > maximoAbsoluto) return maximoAbsoluto;
  if (delta < -maximoAbsoluto) return -maximoAbsoluto;
  return delta;
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
  if (confiable[primero] && confiable[segundo]) {
    const float v1 = static_cast<float>(valores[primero]);
    const float v2 = static_cast<float>(valores[segundo]);
    // Si ambos avanzan pero difieren apreciablemente (>20%) con magnitudes > 8 ticks,
    // el valor menor representa el rodamiento real sobre el suelo sin patinaje ni wheel spin.
    if (v1 > 8.0f && v2 > 8.0f && fabsf(v1 - v2) / fminf(v1, v2) > 0.20f) {
      return fminf(v1, v2);
    }
    return (v1 + v2) * 0.5f;
  }
  if (confiable[primero]) return static_cast<float>(valores[primero]);
  if (confiable[segundo]) return static_cast<float>(valores[segundo]);
  return 0.0f;
}

// Proteccion de modo degradado: cuando un lado depende de un unico encoder
// (su pareja fue excluida) su lectura no tiene contrapeso y el ruido o el
// patinaje durante los micro-pulsos pueden inflar la distancia. Se acota a la
// referencia del lado opuesto corroborado solo si esa referencia es positiva:
// nunca se oculta un lado realmente detenido.
inline float acotarLadoFuenteUnica(float valorLado, float referenciaLadoOpuesto,
                                    float desacuerdoMaximo) {
  if (referenciaLadoOpuesto <= 0.0f || valorLado <= 0.0f) return valorLado;
  const float limite = referenciaLadoOpuesto * (1.0f + desacuerdoMaximo);
  return valorLado > limite ? referenciaLadoOpuesto : valorLado;
}

struct PromediosLado {
  float izquierdo = 0.0f;
  float derecho = 0.0f;
};

// Promedio confiable por lado con la proteccion de fuente unica aplicada de
// forma simetrica. Es el estimador canonico para distancia y odometria.
inline PromediosLado promediosConfiableAcotados(const int64_t valores[4],
                                                const bool confiable[4],
                                                float desacuerdoMaximo) {
  PromediosLado resultado;
  resultado.izquierdo = promedioConfiableLado(valores, confiable, true);
  resultado.derecho = promedioConfiableLado(valores, confiable, false);
  const bool izquierdoFuenteUnica = confiable[0] != confiable[2];
  const bool derechoFuenteUnica = confiable[1] != confiable[3];
  if (izquierdoFuenteUnica && !derechoFuenteUnica) {
    resultado.izquierdo = acotarLadoFuenteUnica(resultado.izquierdo,
                                                 resultado.derecho, desacuerdoMaximo);
  } else if (derechoFuenteUnica && !izquierdoFuenteUnica) {
    resultado.derecho = acotarLadoFuenteUnica(resultado.derecho,
                                               resultado.izquierdo, desacuerdoMaximo);
  }
  return resultado;
}

// Estimador de avance balanceado bilateralmente: pondera equitativamente ambos
// lados del chasis (50% izquierdo, 50% derecho) incluso si un lado tiene un encoder
// excluido por hardware o fallo de canal, integrando el rechazo de patinaje.
inline float estimarAvanceBilateralConfiable(const int64_t valores[4],
                                            const bool confiable[4],
                                            float desacuerdoMaximo) {
  const PromediosLado promedios = promediosConfiableAcotados(valores, confiable, desacuerdoMaximo);
  const bool ladoIzqValido = confiable[0] || confiable[2];
  const bool ladoDerValido = confiable[1] || confiable[3];
  if (ladoIzqValido && ladoDerValido) {
    return (promedios.izquierdo + promedios.derecho) * 0.5f;
  }
  if (ladoIzqValido) return promedios.izquierdo;
  if (ladoDerValido) return promedios.derecho;
  return -1.0f;
}

// Media aritmetica total de los canales marcados como confiables, sin
// distincion de lado ni regla de minimos. Es el estimador de distancia pedido
// por el usuario: un encoder que sub-lee no sesga el resultado a la baja.
// Retorna -1 cuando no queda ninguna fuente confiable.
inline float mediaEncodersSaludables(const int64_t valores[4],
                                     const bool confiable[4]) {
  float suma = 0.0f;
  int n = 0;
  for (int i = 0; i < 4; ++i) {
    if (confiable[i]) {
      suma += static_cast<float>(valores[i]);
      ++n;
    }
  }
  return n > 0 ? suma / static_cast<float>(n) : -1.0f;
}

inline bool ladoEnStall(bool ladoExigido, bool pulsoFrontalCero,
                        bool pulsoPosteriorCero) {
  return ladoExigido && pulsoFrontalCero && pulsoPosteriorCero;
}

inline bool fuentesPorLadoValidas(const bool confiable[4]) {
  return (confiable[0] || confiable[2]) && (confiable[1] || confiable[3]);
}

// Un desacuerdo de magnitud entre dos ruedas que sí producen pulsos puede ser
// deslizamiento, carga o distinta resolución mecánica. No permite excluir las
// dos fuentes del mismo lado. La evidencia inequívoca de cable/encoder ausente
// es el cero aislado mientras su pareja sí avanza bajo PWM.
inline bool encoderSinRespuestaAislada(int64_t delta, int64_t deltaPareja,
                                       bool ladoExigido) {
  return ladoExigido && delta == 0 && deltaPareja > 0;
}

inline uint8_t nivelAntiFriccion8Bit(uint8_t indice) {
  static const uint8_t niveles[7] = {161, 168, 176, 184, 191, 216, 242};
  if (indice < 1) indice = 1;
  if (indice > 7) indice = 7;
  return niveles[indice - 1];
}

inline bool movimientoAntiFriccionConfirmado(float deltaIzq, float deltaDer,
                                               float minimoTicks) {
  return deltaIzq >= minimoTicks && deltaDer >= minimoTicks;
}

inline bool stopDebePreservarFallo(bool fallo, bool estop) {
  return fallo || estop;
}

struct EstadoVigilanciaDivergenciaGiro {
  float menorErrorAbs = 999.0f;
  uint32_t inicioDivergenciaMs = 0;

  void reiniciar(float errorAbsInicial) {
    menorErrorAbs = errorAbsInicial;
    inicioDivergenciaMs = 0;
  }
};

inline bool evaluarDivergenciaGiro(
    EstadoVigilanciaDivergenciaGiro& estado,
    float errorAbsActual,
    uint32_t ahoraMs,
    float umbralDivergenciaDeg,
    uint32_t tiempoLimiteMs,
    bool movimientoPresente) {
  if (errorAbsActual < estado.menorErrorAbs) {
    estado.menorErrorAbs = errorAbsActual;
    estado.inicioDivergenciaMs = 0;
    return false;
  }

  if (movimientoPresente && errorAbsActual > (estado.menorErrorAbs + umbralDivergenciaDeg)) {
    if (estado.inicioDivergenciaMs == 0) {
      estado.inicioDivergenciaMs = ahoraMs;
    } else if (ahoraMs - estado.inicioDivergenciaMs >= tiempoLimiteMs) {
      return true;
    }
  } else {
    estado.inicioDivergenciaMs = 0;
  }
  return false;
}

}  // namespace ControlSeguridad
