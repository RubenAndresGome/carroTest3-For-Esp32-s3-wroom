#pragma once

#include <cstdint>

// Perfil de compensacion adaptativo por lado. El HMI lo persiste por tipo de
// piso en su base de datos y lo inyecta por comando; el firmware conserva una
// copia en RAM para el control. Es una unidad pura (sin Arduino ni hardware)
// para poder validar sus limites y su promedio en las pruebas nativas.
namespace ControlCompensacion {

constexpr float TRIM_MIN = 0.80f;
constexpr float TRIM_MAX = 1.20f;
constexpr int DEADBAND_MAX_8BIT = 30;

struct Muestra {
  float trimIzq = 1.0f;
  float trimDer = 1.0f;
  int deadbandIzq8 = 0;
  int deadbandDer8 = 0;
};

// Piso dinamico de arranque: solo eleva velocidades por debajo del umbral de
// crucero (vence la esticcion de los TT). Nunca suma sobre el crucero, por lo
// que no puede superar el limite continuo de avance (242/255).
inline int aplicarPisoDeadband(int vel, int deadband, int umbralCrucero) {
  if (vel == 0 || deadband <= 0) return vel;
  const int mag = vel < 0 ? -vel : vel;
  if (umbralCrucero > 0 && mag >= umbralCrucero) return vel;
  const int piso = mag > deadband ? mag : deadband;
  return vel > 0 ? piso : -piso;
}

inline float limitarTrim(float valor) {
  return valor < TRIM_MIN ? TRIM_MIN : (valor > TRIM_MAX ? TRIM_MAX : valor);
}

inline int limitarDeadband8(int valor8) {
  if (valor8 < 0) return 0;
  return valor8 > DEADBAND_MAX_8BIT ? DEADBAND_MAX_8BIT : valor8;
}

class Perfil {
 public:
  void reiniciar() { *this = Perfil(); }

  // Media incremental acotada: cada muestra pesa igual y el perfil converge
  // con mas datos sin saltos bruscos.
  void agregar(const Muestra& muestra) {
    const int n = cantidad_ + 1;
    trimIzq_ = limitarTrim((trimIzq_ * cantidad_ + muestra.trimIzq) / n);
    trimDer_ = limitarTrim((trimDer_ * cantidad_ + muestra.trimDer) / n);
    deadbandIzq8_ = limitarDeadband8(
        (deadbandIzq8_ * cantidad_ + muestra.deadbandIzq8) / n);
    deadbandDer8_ = limitarDeadband8(
        (deadbandDer8_ * cantidad_ + muestra.deadbandDer8) / n);
    if (cantidad_ < 255) ++cantidad_;
  }

  // Inyeccion directa desde la superficie activa del HMI.
  void establecer(float trimIzq, float trimDer, int deadbandIzq8,
                  int deadbandDer8) {
    trimIzq_ = limitarTrim(trimIzq);
    trimDer_ = limitarTrim(trimDer);
    deadbandIzq8_ = limitarDeadband8(deadbandIzq8);
    deadbandDer8_ = limitarDeadband8(deadbandDer8);
    cantidad_ = 1;
  }

  float getLadoIzq() const { return trimIzq_; }
  float getLadoDer() const { return trimDer_; }
  float getProm() const { return 0.5f * (trimIzq_ + trimDer_); }
  int getDeadbandIzq8() const { return deadbandIzq8_; }
  int getDeadbandDer8() const { return deadbandDer8_; }
  uint8_t getCantidad() const { return cantidad_; }

 private:
  float trimIzq_ = 1.0f;
  float trimDer_ = 1.0f;
  int deadbandIzq8_ = 0;
  int deadbandDer8_ = 0;
  uint8_t cantidad_ = 0;
};

}  // namespace ControlCompensacion
