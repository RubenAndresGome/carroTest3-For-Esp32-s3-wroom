#pragma once

#include <cstdint>

namespace ControlTorque {

constexpr uint8_t VERSION_FORMATO = 2;
constexpr uint8_t MAX_REGISTROS = 10;
constexpr int PWM_MIN_8BIT = 140;
constexpr int PWM_MAX_8BIT = 247;
constexpr int MARGEN_INICIO_8BIT = 5;

struct Registro {
  uint32_t secuencia;
  int pwmPositivo8;
  int pwmNegativo8;
  int polaridadPositiva;
  int polaridadNegativa;
  int pwmPositivoDerecho8;
  int pwmNegativoDerecho8;

  constexpr Registro(uint32_t secuenciaValor = 0, int pwmPositivoValor = 0,
                     int pwmNegativoValor = 0, int polaridadPositivaValor = 0,
                     int polaridadNegativaValor = 0)
      : secuencia(secuenciaValor), pwmPositivo8(pwmPositivoValor),
        pwmNegativo8(pwmNegativoValor), polaridadPositiva(polaridadPositivaValor),
        polaridadNegativa(polaridadNegativaValor),
        pwmPositivoDerecho8(pwmPositivoValor), pwmNegativoDerecho8(pwmNegativoValor) {}
};

struct Historial {
  Registro registros[MAX_REGISTROS] = {};
  uint8_t cantidad = 0;
  uint32_t siguienteSecuencia = 1;
};

inline bool registroValido(const Registro& registro) {
  return registro.secuencia > 0 &&
      registro.pwmPositivoDerecho8 >= PWM_MIN_8BIT && registro.pwmPositivoDerecho8 <= PWM_MAX_8BIT &&
      registro.pwmNegativoDerecho8 >= PWM_MIN_8BIT && registro.pwmNegativoDerecho8 <= PWM_MAX_8BIT &&
      registro.pwmPositivo8 >= PWM_MIN_8BIT && registro.pwmPositivo8 <= PWM_MAX_8BIT &&
      registro.pwmNegativo8 >= PWM_MIN_8BIT && registro.pwmNegativo8 <= PWM_MAX_8BIT &&
      (registro.polaridadPositiva == -1 || registro.polaridadPositiva == 1) &&
      (registro.polaridadNegativa == -1 || registro.polaridadNegativa == 1) &&
      registro.polaridadPositiva != registro.polaridadNegativa;
}

inline void agregar(Historial& historial, Registro registro) {
  if (!registroValido(registro)) return;
  if (registro.secuencia >= historial.siguienteSecuencia)
    historial.siguienteSecuencia = registro.secuencia + 1;
  if (historial.cantidad < MAX_REGISTROS) {
    historial.registros[historial.cantidad++] = registro;
    return;
  }
  for (uint8_t i = 1; i < MAX_REGISTROS; ++i)
    historial.registros[i - 1] = historial.registros[i];
  historial.registros[MAX_REGISTROS - 1] = registro;
}

inline int promedioDireccion(const Historial& historial, bool positiva, bool izquierda = true) {
  if (historial.cantidad == 0) return PWM_MIN_8BIT;
  int suma = 0;
  for (uint8_t i = 0; i < historial.cantidad; ++i) {
    const Registro& registro = historial.registros[i];
    suma += positiva
        ? (izquierda ? registro.pwmPositivo8 : registro.pwmPositivoDerecho8)
        : (izquierda ? registro.pwmNegativo8 : registro.pwmNegativoDerecho8);
  }
  return (suma + historial.cantidad / 2) / historial.cantidad;
}

inline int baseDesdePromedio(int promedio) {
  int base = promedio - MARGEN_INICIO_8BIT;
  if (base < PWM_MIN_8BIT) return PWM_MIN_8BIT;
  if (base > PWM_MAX_8BIT) return PWM_MAX_8BIT;
  return base;
}

inline int baseParaPolaridad(const Historial& historial, int polaridad, bool izquierda = true) {
  int suma = 0;
  int cantidad = 0;
  for (uint8_t i = 0; i < historial.cantidad; ++i) {
    const Registro& registro = historial.registros[i];
    if (registro.polaridadPositiva == polaridad) {
      suma += izquierda ? registro.pwmPositivo8 : registro.pwmPositivoDerecho8;
      ++cantidad;
    } else if (registro.polaridadNegativa == polaridad) {
      suma += izquierda ? registro.pwmNegativo8 : registro.pwmNegativoDerecho8;
      ++cantidad;
    }
  }
  return baseDesdePromedio(cantidad ? (suma + cantidad / 2) / cantidad : PWM_MIN_8BIT);
}

inline bool iguales(const Historial& primero, const Historial& segundo) {
  if (primero.cantidad != segundo.cantidad) return false;
  for (uint8_t i = 0; i < primero.cantidad; ++i) {
    const Registro& a = primero.registros[i];
    const Registro& b = segundo.registros[i];
    if (a.secuencia != b.secuencia || a.pwmPositivo8 != b.pwmPositivo8 ||
        a.pwmPositivoDerecho8 != b.pwmPositivoDerecho8 || a.pwmNegativoDerecho8 != b.pwmNegativoDerecho8 ||
        a.pwmNegativo8 != b.pwmNegativo8 ||
        a.polaridadPositiva != b.polaridadPositiva ||
        a.polaridadNegativa != b.polaridadNegativa) return false;
  }
  return true;
}

}  // namespace ControlTorque
