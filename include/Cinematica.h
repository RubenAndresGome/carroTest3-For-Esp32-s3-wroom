#pragma once

#include "Comandos.h"
#include "Estado.h"

extern unsigned long tiempoInicioGiro;
extern unsigned long tiempoInicioAvance;
extern float distanciaObjetivoCM;
extern float anguloObjetivoGrados;

constexpr unsigned long ESTABILIZACION_MS = 300;

bool calcularDestino(float x_destino, float y_destino, const char* command_id);
bool iniciarDistancia(float distancia_mm, const char* command_id);
bool iniciarGiroRelativo(float angulo_deg, const char* command_id,
                         ModoGiroSolicitado modo = ModoGiroSolicitado::AUTO);
bool iniciarCalibracion(const char* command_id);
bool habilitarControlSimpleSinCalibracion();
bool validarControlGiroSeguro();
void cancelarMovimiento(const char* detalle);
void abortarMovimientoPorSeguridad(const char* detalle);
void controlarGiro();
void controlarAvance();
void controlarCalibracion();
uint8_t intentoGiroActualTelemetria();
uint32_t pausaReintentoRestanteMs();
uint8_t intentoAvanceActualTelemetria();
uint32_t pausaReintentoAvanceRestanteMs();
const char* motivoRecuperacionAutonomaTelemetria();
int pwmCalibracionActual();
uint8_t intentoTorqueCalibracionTelemetria();
bool busquedaTorqueCalibracionActiva();
const char* modoGiroTexto();
const char* modoGiroSolicitadoTexto();
const char* motivoModoGiroArco();
uint32_t pausaEntradaArcoRestanteMs();
const char* faseCalibracionTexto();
int64_t ticksUltimoPulsoCalibracion(size_t indice);
int signoFijoGiroTelemetria();
float anguloSolicitadoGiroTelemetria();
float objetivoFisicoGiroTelemetria();
uint8_t vueltasExtendidasGiroTelemetria();
float rumboCardinalObjetivoTelemetria();
const char* faseMovimientoTexto();
const char* etapaAlineacionTexto();
float toleranciaGiroActualTelemetria();
float errorRumboCardinalTelemetria();
uint32_t estabilidadRumboActualMs();
bool rutaAlineadaGruesaTelemetria();
bool rutaAlineadaPrecisaTelemetria();
const char* faseArcoTexto();
uint16_t pulsosArcoTelemetria();
uint8_t pulsosArcoSinProgresoTelemetria();
uint8_t transicionesFallbackGiroTelemetria();
bool ladoGiroFisicamenteEnergizado(size_t lado);
uint32_t inicioEnergiaFisicaGiroMs(size_t lado);
const char* motivoUltimoRechazoMovimiento();
float progresoGiroTelemetria();
float errorGiroTelemetria();
float errorPredichoGiroTelemetria();
int pwmObjetivoGiroTelemetria();
const char* faseFrenadoGiroTelemetria();
uint32_t tiempoImuSinRespuestaGiroMs();
uint32_t tiempoDireccionIncorrectaGiroMs();
float distanciaRestanteEjeRutaTelemetria();
const char* saludEncoderTexto(size_t indice);
uint8_t encodersConfiablesLado(size_t lado);
const char* estimadorDistanciaTexto();
float errorRumboPredichoTelemetria();
float correccionRumboPwmTelemetria();
float correccionEncoderPwmTelemetria();
uint8_t intentoRecuperacionRumboTelemetria();
float velocidadFiltradaLadoTelemetria(size_t lado);
