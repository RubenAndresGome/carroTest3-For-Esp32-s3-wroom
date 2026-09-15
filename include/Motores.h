#pragma once
#include <Arduino.h>

extern int pwm_aplicado_L;
extern int pwm_aplicado_R;
extern int pwm_solicitado_L;
extern int pwm_solicitado_R;

// --- TIPOS ORIENTADOS A OBJETOS (POO) ---
enum class SentidoGiro : uint8_t {
  DETENIDO = 0,
  ADELANTE = 1,
  ATRAS    = 2
};

const char* sentidoToString(SentidoGiro sentido);

enum class EstadoInterlock : uint8_t {
  APAGADO,
  ACTIVO,
  ESPERANDO_INVERSION
};

class ControladorLado {
private:
  const char* _nombre;
  int _pinFwdF;
  int _pinRevF;
  int _polF;
  int _pinFwdB;
  int _pinRevB;
  int _polB;

  SentidoGiro _sentidoSolicitado = SentidoGiro::DETENIDO;
  int _pwmSolicitado = 0;

  SentidoGiro _sentidoAplicado = SentidoGiro::DETENIDO;
  int _pwmAplicado = 0;

  EstadoInterlock _estadoInterlock = EstadoInterlock::APAGADO;
  SentidoGiro _sentidoActivo = SentidoGiro::DETENIDO;
  SentidoGiro _ultimoSentidoEnergizado = SentidoGiro::DETENIDO;
  SentidoGiro _sentidoPendiente = SentidoGiro::DETENIDO;
  uint32_t _apagadoDesdeMs = 0;
  bool _apagadoRegistrado = false;

public:
  ControladorLado(const char* nombre = "LADO",
                  int pinFwdF = -1, int pinRevF = -1, int polF = 1,
                  int pinFwdB = -1, int pinRevB = -1, int polB = 1);

  void configurarPines(const char* nombre,
                       int pinFwdF, int pinRevF, int polF,
                       int pinFwdB, int pinRevB, int polB);

  // Setters POO
  void setComando(SentidoGiro sentido, int pwm);
  void setVelocidadFirmada(int velFirmada);
  void detener(uint32_t ahoraMs);
  void detener();

  // Getters POO
  SentidoGiro getSentido() const { return _sentidoAplicado; }
  SentidoGiro getSentidoSolicitado() const { return _sentidoSolicitado; }
  const char* getSentidoStr() const { return sentidoToString(_sentidoAplicado); }
  const char* getSentidoSolicitadoStr() const { return sentidoToString(_sentidoSolicitado); }

  int getPWM() const { return _pwmAplicado; }
  int getPWMSolicitado() const { return _pwmSolicitado; }
  int getVelocidadFirmada() const {
    if (_sentidoAplicado == SentidoGiro::ADELANTE) return _pwmAplicado;
    if (_sentidoAplicado == SentidoGiro::ATRAS) return -_pwmAplicado;
    return 0;
  }
  int getVelocidadFirmadaSolicitada() const {
    if (_sentidoSolicitado == SentidoGiro::ADELANTE) return _pwmSolicitado;
    if (_sentidoSolicitado == SentidoGiro::ATRAS) return -_pwmSolicitado;
    return 0;
  }

  EstadoInterlock getEstadoInterlock() const { return _estadoInterlock; }
  const char* getEstadoInterlockStr() const;
  int getSignoEnergizado() const {
    return _sentidoActivo == SentidoGiro::ADELANTE ? 1 : (_sentidoActivo == SentidoGiro::ATRAS ? -1 : 0);
  }
  int getSignoPendiente() const {
    return _sentidoPendiente == SentidoGiro::ADELANTE ? 1 : (_sentidoPendiente == SentidoGiro::ATRAS ? -1 : 0);
  }

  // Interlock puro (evaluación de tiempo muerto de 250 ms)
  int actualizarInterlock(uint32_t ahoraMs);

  // Salida a hardware
  bool escribirHardware();
  void apagarHardware();
};

extern ControladorLado ladoIzquierdo;
extern ControladorLado ladoDerecho;

// Comandos de tracción de alto nivel
void ordenarAvance(int pwm);
void ordenarRetroceso(int pwm);
void ordenarGiroPivote(bool sentidoHorario, int pwm);

// La salida PWM sólo queda habilitada después de setup_Motores(). El estado
// permite distinguir un mapa inválido de un controlador aún no inicializado.
bool validarMapaMotores();
bool motoresListos();
const char* estadoMotores();

void setup_MotorPinsLow();
bool validarInterlockMotores();
bool setup_Motores();
bool aplicarVelocidades(int velIzq, int velDer);
void frenarMotores();
const char *estadoInterlockL();
const char *estadoInterlockR();
int signoEnergizadoL();
int signoEnergizadoR();
int signoPendienteL();
int signoPendienteR();
