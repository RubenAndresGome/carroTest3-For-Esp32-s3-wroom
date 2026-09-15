#pragma once
#include <Arduino.h>
#include "Config.h"
#include "Motores.h"
#include "ControlRuta.h"

enum class DireccionGiro : uint8_t {
  NINGUNA = 0,
  DERECHA_HORARIO = 1,      // +yaw: L ADELANTE, R ATRAS
  IZQUIERDA_ANTIHORARIO = 2  // -yaw: L ATRAS, R ADELANTE
};

const char* direccionGiroToString(DireccionGiro dir);

enum class EstadoLazoGiro : uint8_t {
  INACTIVO = 0,
  BUSQUEDA_TORQUE = 1,
  CRUCERO = 2,
  RAMPA_ADAPTATIVA = 3,
  MICRO_PULSOS_FINOS = 4,
  ASENTAMIENTO = 5,
  COMPLETADO = 6,
  FALLO = 7
};

class ControladorGiro {
private:
  float _yawObjetivoDeg = 0.0f;
  float _errorAngularDeg = 0.0f;
  float _errorAbsDeg = 0.0f;
  float _toleranciaDeg = TOLERANCIA_GIRO_DEG;

  DireccionGiro _direccion = DireccionGiro::NINGUNA;
  EstadoLazoGiro _estado = EstadoLazoGiro::INACTIVO;

  int _pwmMinPos = 0;
  int _pwmMinNeg = 0;
  int _pwmActual = 0;
  int _pwmBusqueda = 0;
  int _pwmBoostFrenado = 0;
  int _signoGiroAplicado = 0;

  bool _movGiroConfirmado = false;
  bool _pulsoFinoEncendido = false;
  bool _enTolerancia = false;
  bool _watchdogArmado = false;

  uint32_t _inicioGiroTotalMs = 0;
  uint32_t _inicioIntentoMs = 0;
  uint32_t _inicioPulsoFinoMs = 0;
  uint32_t _estableDesdeMs = 0;
  uint32_t _ultimoAumentoTorqueMs = 0;
  uint32_t _ultimoPulsoLadoMs[2] = {};
  int64_t _ticksLadoAnterior[2] = {};

  const char* _motivoFallo = nullptr;

  float normalizarError(float objDeg, float actDeg) const;

public:
  ControladorGiro();

  void iniciarGiroAbsoluto(float headingObjetivoDeg, int pwmMinPos, int pwmMinNeg,
                           float toleranciaDeg = TOLERANCIA_GIRO_DEG, uint32_t ahoraMs = 0);
  void iniciarGiroRelativo(float deltaDeg, float headingActualDeg, int pwmMinPos, int pwmMinNeg,
                           float toleranciaDeg = TOLERANCIA_GIRO_DEG, uint32_t ahoraMs = 0);

  // Ejecución síncrona en cada ciclo de 100 Hz con datos del MPU
  bool actualizar(float headingActualMPU, float gyroZFiltradoRadS,
                  const int64_t deltasEncoders[4], const bool encoderConfiable[4],
                  uint32_t ahoraMs);

  void cancelar();

  // Getters POO
  bool estaActivo() const { return _estado != EstadoLazoGiro::INACTIVO && _estado != EstadoLazoGiro::COMPLETADO && _estado != EstadoLazoGiro::FALLO; }
  bool estaCompletado() const { return _estado == EstadoLazoGiro::COMPLETADO; }
  bool estaEnFallo() const { return _estado == EstadoLazoGiro::FALLO; }
  bool estaEnTolerancia() const { return _enTolerancia; }

  float getHeadingObjetivo() const { return _yawObjetivoDeg; }
  float getErrorAngular() const { return _errorAngularDeg; }
  float getErrorAbsoluto() const { return _errorAbsDeg; }
  DireccionGiro getDireccion() const { return _direccion; }
  const char* getDireccionStr() const { return direccionGiroToString(_direccion); }
  EstadoLazoGiro getEstado() const { return _estado; }
  int getPWM() const { return _pwmActual; }
  int getPwmMinPos() const { return _pwmMinPos; }
  int getPwmMinNeg() const { return _pwmMinNeg; }
  bool esMovimientoConfirmado() const { return _movGiroConfirmado; }
  const char* getMotivoFallo() const { return _motivoFallo ? _motivoFallo : "none"; }
};

extern ControladorGiro controladorGiro;
