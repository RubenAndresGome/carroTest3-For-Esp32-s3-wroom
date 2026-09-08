#include <unity.h>

#include "ControlRuta.h"
#include "ControlSeguridad.h"
#include "ControlCalibracion.h"
#include "ControlManual.h"
#include "ControlTorque.h"
#include "ControlInicializacionPCNT.h"
#include "ControlConexion.h"

extern "C" void setUp() {}
extern "C" void tearDown() {}

namespace {

void test_desconexion_solo_cancela_calibracion() {
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(ControlConexion::AccionDesconexion::CANCELAR_CALIBRACION),
      static_cast<int>(ControlConexion::accionAlPerderWebSocket(true)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(ControlConexion::AccionDesconexion::NINGUNA),
      static_cast<int>(ControlConexion::accionAlPerderWebSocket(false)));
}

void test_lateral_derecha_corrige_hacia_izquierda() {
  const auto errores = ControlRuta::calcularErroresTrayectoria(6.0f, 50.0f, 0.0f, 100.0f, 0.0f, 100.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 6.0f, errores.lateralCm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -4.8f,
                           ControlRuta::correccionLateralRumboDeg(errores.lateralCm, 0.8f, 8.0f));
}

void test_lateral_izquierda_corrige_hacia_derecha() {
  const auto errores = ControlRuta::calcularErroresTrayectoria(-6.0f, 50.0f, 0.0f, 100.0f, 0.0f, 100.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -6.0f, errores.lateralCm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.8f,
                           ControlRuta::correccionLateralRumboDeg(errores.lateralCm, 0.8f, 8.0f));
}

void test_reversa_automatica_conserva_el_chasis_ante_objetivo_detras() {
  TEST_ASSERT_TRUE(ControlRuta::reversaAutomatica(180.0f, 0.0f, 135.0f));
  TEST_ASSERT_FALSE(ControlRuta::reversaAutomatica(90.0f, 0.0f, 135.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f,
                           ControlRuta::rumboCuerpoParaTrayecto(180.0f, -1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 180.0f,
                           ControlRuta::rumboCuerpoParaTrayecto(180.0f, 1));
}

void test_reversa_invierte_solo_la_correccion_lateral_del_chasis() {
  const float correccionTrayecto = ControlRuta::correccionLateralRumboDeg(5.0f, 0.8f, 8.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -4.0f, correccionTrayecto);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f,
                           ControlRuta::correccionLateralParaDireccion(correccionTrayecto, -1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -4.0f,
                           ControlRuta::correccionLateralParaDireccion(correccionTrayecto, 1));
}

void test_reversa_invierte_el_lado_frenado_por_el_pid_de_rumbo() {
  TEST_ASSERT_TRUE(ControlRuta::frenarLadoIzquierdoParaRumbo(1, 1));
  TEST_ASSERT_FALSE(ControlRuta::frenarLadoIzquierdoParaRumbo(1, -1));
  TEST_ASSERT_FALSE(ControlRuta::frenarLadoIzquierdoParaRumbo(-1, 1));
  TEST_ASSERT_TRUE(ControlRuta::frenarLadoIzquierdoParaRumbo(-1, -1));
}

void test_escala_y_freno_se_calculan_en_la_misma_unidad() {
  const float cmPorTick = ControlRuta::distanciaPorTick(6.6f * 1.15f, 20);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.192f, cmPorTick);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.02f,
                           ControlRuta::distanciaFrenoPrevista(920.0f, 1.5f, 0.006f, 8.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 8.0f,
                           ControlRuta::distanciaFrenoPrevista(2000.0f, 1.5f, 0.006f, 8.0f));
}

void test_integral_se_acota_y_no_crece_en_saturacion() {
  ControlRuta::EstadoPI estado = {};
  for (int i = 0; i < 500; ++i) {
    ControlRuta::actualizarPI(estado, 30.0f, 0.0f, 0.01f, 4.0f, 0.35f, 12.0f, 80.0f, 35.0f);
  }
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, estado.integralGradoS);
  const auto salida = ControlRuta::actualizarPI(estado, -30.0f, 0.0f, 0.01f,
                                                 4.0f, 0.35f, 12.0f, 80.0f, 35.0f);
  TEST_ASSERT_TRUE(salida.total < 0.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, estado.integralGradoS);
}

void test_integral_acumula_y_se_limita_fuera_de_saturacion() {
  ControlRuta::EstadoPI estado = {};
  for (int i = 0; i < 10000; ++i) {
    ControlRuta::actualizarPI(estado, 1.0f, 0.0f, 0.01f, 0.0f, 1.0f, 0.0f, 80.0f, 2.0f);
  }
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, estado.integralGradoS);
}

void test_distancia_sola_no_acepta_endpoint() {
  TEST_ASSERT_FALSE(ControlRuta::endpointAceptable(6.0f, 6.1f, 0.0f, 5.0f, 3.0f));
  TEST_ASSERT_FALSE(ControlRuta::endpointAceptable(0.0f, 4.0f, 3.1f, 5.0f, 3.0f));
  TEST_ASSERT_TRUE(ControlRuta::endpointAceptable(4.9f, 5.0f, -3.0f, 5.0f, 3.0f));
}

void test_fallo_endpoint_despues_de_dos_intentos() {
  TEST_ASSERT_FALSE(ControlRuta::agotoIntentosEndpoint(1, 2));
  TEST_ASSERT_TRUE(ControlRuta::agotoIntentosEndpoint(2, 2));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ControlRuta::DecisionEndpoint::COMPLETAR),
                          static_cast<uint8_t>(ControlRuta::decidirEndpoint(false, false, 0, 2)));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ControlRuta::DecisionEndpoint::COMPLETAR),
                          static_cast<uint8_t>(ControlRuta::decidirEndpoint(true, true, 0, 2)));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ControlRuta::DecisionEndpoint::RECUPERAR),
                          static_cast<uint8_t>(ControlRuta::decidirEndpoint(true, false, 1, 2)));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ControlRuta::DecisionEndpoint::FALLAR),
                          static_cast<uint8_t>(ControlRuta::decidirEndpoint(true, false, 2, 2)));
}

void test_endpoint_corto_pide_calibracion_y_el_largo_se_recupera() {
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ControlRuta::DecisionEndpoint::CALIBRAR),
                          static_cast<uint8_t>(ControlRuta::decidirEndpointSeguro(
                              true, false, 0, 2, 6.7f, 13.0f)));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ControlRuta::DecisionEndpoint::RECUPERAR),
                          static_cast<uint8_t>(ControlRuta::decidirEndpointSeguro(
                              true, false, 0, 2, 24.0f, 13.0f)));
}

void test_imu_perdida_se_detecta() {
  TEST_ASSERT_TRUE(ControlSeguridad::imuApta(true, false));
  TEST_ASSERT_FALSE(ControlSeguridad::imuApta(false, false));
  TEST_ASSERT_FALSE(ControlSeguridad::imuApta(true, true));
}

void test_encoder_incoherente_se_detecta() {
  TEST_ASSERT_FALSE(ControlSeguridad::encoderEsOutlier(12, 10.0f, 0.40f));
  TEST_ASSERT_TRUE(ControlSeguridad::encoderEsOutlier(15, 10.0f, 0.40f));
}

void test_pico_pcnt_imposible_no_puede_contaminar_odometria() {
  TEST_ASSERT_TRUE(ControlSeguridad::deltaEncoderPlausible(64, 64));
  TEST_ASSERT_TRUE(ControlSeguridad::deltaEncoderPlausible(-64, 64));
  TEST_ASSERT_FALSE(ControlSeguridad::deltaEncoderPlausible(24793, 64));
  TEST_ASSERT_FALSE(ControlSeguridad::deltaEncoderPlausible(-32768, 64));
}

void test_fusion_descarta_cero_aislado_sin_sesgar_distancia() {
  const int64_t ticks[4] = {0, 70, 69, 70};
  const auto fusion = ControlSeguridad::clasificarEncoders(ticks, 0.40f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 69.5f, fusion.mediana);
  TEST_ASSERT_FALSE(fusion.confiable[0]);
  TEST_ASSERT_TRUE(fusion.confiable[1]);
  TEST_ASSERT_TRUE(fusion.confiable[2]);
  TEST_ASSERT_TRUE(fusion.confiable[3]);
  TEST_ASSERT_TRUE(fusion.ladoIzquierdoValido);
  TEST_ASSERT_TRUE(fusion.ladoDerechoValido);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 69.0f,
                           ControlSeguridad::promedioConfiableLado(ticks, fusion.confiable, true));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 70.0f,
                           ControlSeguridad::promedioConfiableLado(ticks, fusion.confiable, false));
}

void test_fusion_falla_si_un_lado_completo_no_es_confiable() {
  const int64_t ticks[4] = {0, 70, 0, 71};
  const auto fusion = ControlSeguridad::clasificarEncoders(ticks, 0.40f);
  TEST_ASSERT_FALSE(fusion.ladoIzquierdoValido);
  TEST_ASSERT_TRUE(fusion.ladoDerechoValido);
}

void test_stall_por_lado_exige_dos_encoders_sin_pulsos() {
  TEST_ASSERT_TRUE(ControlSeguridad::ladoEnStall(true, true, true));
  TEST_ASSERT_FALSE(ControlSeguridad::ladoEnStall(true, false, true));
  TEST_ASSERT_FALSE(ControlSeguridad::ladoEnStall(false, true, true));
}

void test_errores_vectoriales_son_consistentes_en_rumbos_diagonales() {
  const float rumbos[] = {45.0f, 135.0f, 225.0f, 315.0f};
  constexpr float kPi = 3.14159265358979323846f;
  for (float rumbo : rumbos) {
    const float rad = rumbo * kPi / 180.0f;
    const float ux = sinf(rad), uy = cosf(rad);
    const float objetivoX = 100.0f * ux, objetivoY = 100.0f * uy;
    const float posicionX = 50.0f * ux + 4.0f * uy;
    const float posicionY = 50.0f * uy - 4.0f * ux;
    const auto errores = ControlRuta::calcularErroresTrayectoria(
        posicionX, posicionY, objetivoX, objetivoY, rumbo, 100.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.002f, 50.0f, errores.longitudinalCm);
    TEST_ASSERT_FLOAT_WITHIN(0.002f, 4.0f, errores.lateralCm);
    TEST_ASSERT_FLOAT_WITHIN(0.002f, hypotf(50.0f, 4.0f), errores.euclidianoCm);
  }
}

void test_cualquier_encoder_individual_puede_fallar_sin_perder_un_lado() {
  for (int excluido = 0; excluido < 4; ++excluido) {
    bool confiable[4] = {true, true, true, true};
    confiable[excluido] = false;
    TEST_ASSERT_TRUE(ControlSeguridad::fuentesPorLadoValidas(confiable));
  }
  const bool solo_fr_bl[4] = {false, true, true, false};
  const bool solo_fl_br[4] = {true, false, false, true};
  const bool sin_izquierda[4] = {false, true, false, true};
  TEST_ASSERT_TRUE(ControlSeguridad::fuentesPorLadoValidas(solo_fr_bl));
  TEST_ASSERT_TRUE(ControlSeguridad::fuentesPorLadoValidas(solo_fl_br));
  TEST_ASSERT_FALSE(ControlSeguridad::fuentesPorLadoValidas(sin_izquierda));
}

void test_antifriccion_escala_siete_pulsos_y_exige_ambos_lados() {
  TEST_ASSERT_EQUAL_UINT8(161, ControlSeguridad::nivelAntiFriccion8Bit(1));
  TEST_ASSERT_EQUAL_UINT8(191, ControlSeguridad::nivelAntiFriccion8Bit(5));
  TEST_ASSERT_EQUAL_UINT8(242, ControlSeguridad::nivelAntiFriccion8Bit(7));
  TEST_ASSERT_EQUAL_UINT8(242, ControlSeguridad::nivelAntiFriccion8Bit(99));
  TEST_ASSERT_TRUE(ControlSeguridad::encoderSinRespuestaAislada(0, 6, true));
  TEST_ASSERT_FALSE(ControlSeguridad::encoderSinRespuestaAislada(3, 6, true));
  TEST_ASSERT_FALSE(ControlSeguridad::encoderSinRespuestaAislada(0, 6, false));
  TEST_ASSERT_TRUE(ControlSeguridad::movimientoAntiFriccionConfirmado(2, 2, 2));
  TEST_ASSERT_FALSE(ControlSeguridad::movimientoAntiFriccionConfirmado(2, 1, 2));
}

void test_stop_no_borra_fallo_o_estop_enclavado() {
  TEST_ASSERT_TRUE(ControlSeguridad::stopDebePreservarFallo(true, false));
  TEST_ASSERT_TRUE(ControlSeguridad::stopDebePreservarFallo(false, true));
  TEST_ASSERT_FALSE(ControlSeguridad::stopDebePreservarFallo(false, false));
}

void test_estop_se_reconoce() {
  TEST_ASSERT_TRUE(ControlSeguridad::estopSolicitado(true));
  TEST_ASSERT_FALSE(ControlSeguridad::estopSolicitado(false));
}

void test_calibracion_conserva_promedio_de_ambos_encoders_por_lado() {
  const int64_t deltas[4] = {0, 4, 4, 4};
  const auto evaluacion = ControlCalibracion::evaluarEncoders(deltas, 2);
  TEST_ASSERT_EQUAL_INT64(2, evaluacion.promedioIzquierdo);
  TEST_ASSERT_TRUE(evaluacion.ladoIzquierdoValido);
  TEST_ASSERT_TRUE(evaluacion.ladoDerechoValido);
  TEST_ASSERT_TRUE(evaluacion.sinRespuestaAislada[0]);
  TEST_ASSERT_FALSE(evaluacion.sinRespuestaAislada[2]);
}

void test_calibracion_rechaza_un_lado_completo_sin_pulsos() {
  const int64_t deltas[4] = {0, 8, 1, 9};
  const auto evaluacion = ControlCalibracion::evaluarEncoders(deltas, 2);
  TEST_ASSERT_FALSE(evaluacion.ladoIzquierdoValido);
  TEST_ASSERT_TRUE(evaluacion.ladoDerechoValido);
  TEST_ASSERT_FALSE(evaluacion.sinRespuestaAislada[0]);
  TEST_ASSERT_FALSE(evaluacion.sinRespuestaAislada[2]);
}

void test_calibracion_identifica_cualquier_encoder_aislado() {
  for (int aislado = 0; aislado < 4; ++aislado) {
    int64_t deltas[4] = {8, 8, 8, 8};
    deltas[aislado] = 0;
    const auto evaluacion = ControlCalibracion::evaluarEncoders(deltas, 2);
    TEST_ASSERT_TRUE(evaluacion.ladoIzquierdoValido);
    TEST_ASSERT_TRUE(evaluacion.ladoDerechoValido);
    for (int i = 0; i < 4; ++i)
      TEST_ASSERT_EQUAL(i == aislado, evaluacion.sinRespuestaAislada[i]);
  }
}

void test_calibracion_con_encoder_aislado_usa_promedio_confiable_sin_dividir_entre_dos() {
  const int64_t deltas[4] = {4, 0, 4, 4}; // FR = 0, BR = 4
  const bool confiable[4] = {true, false, true, true}; // FR aislado
  const auto evaluacion = ControlCalibracion::evaluarEncoders(deltas, 2, confiable);
  TEST_ASSERT_EQUAL_INT64(4, evaluacion.promedioIzquierdo);
  TEST_ASSERT_EQUAL_INT64(4, evaluacion.promedioDerecho); // 4 / 1 = 4, NO dividido entre 2
  TEST_ASSERT_TRUE(evaluacion.ladoIzquierdoValido);
  TEST_ASSERT_TRUE(evaluacion.ladoDerechoValido);
  TEST_ASSERT_TRUE(evaluacion.sinRespuestaAislada[1]); // FR detectado como aislado
}

void test_calibracion_aisla_encoder_con_ruido_menor_al_umbral_y_confirma_lado_sano() {
  // Caso real sesion 180: FR tiene 1 pulso de ruido/vibracion (< 2) mientras BR tiene 4
  const int64_t deltas[4] = {4, 1, 4, 4};
  const auto evaluacionPrevia = ControlCalibracion::evaluarEncoders(deltas, 2);
  TEST_ASSERT_TRUE(evaluacionPrevia.sinRespuestaAislada[1]); // FR detectado como aislado aun con 1 tick
  bool confiables[4] = {true, true, true, true};
  if (evaluacionPrevia.sinRespuestaAislada[1]) {
    confiables[1] = false;
  }
  const auto evaluacion = ControlCalibracion::evaluarEncoders(deltas, 2, confiables);
  TEST_ASSERT_EQUAL_INT64(4, evaluacion.promedioDerecho); // BR aporta 4 sin dividir entre 2
  TEST_ASSERT_TRUE(evaluacion.ladoDerechoValido);
}

void test_rampa_calibracion_expone_todos_los_niveles() {
  TEST_ASSERT_EQUAL_UINT8(23, ControlCalibracion::totalPasosRampa(140, 247, 5));
  TEST_ASSERT_EQUAL_UINT8(1, ControlCalibracion::pasoRampaActual(140, 140, 247, 5));
  TEST_ASSERT_EQUAL_UINT8(2, ControlCalibracion::pasoRampaActual(145, 140, 247, 5));
  TEST_ASSERT_EQUAL_UINT8(23, ControlCalibracion::pasoRampaActual(247, 140, 247, 5));
  TEST_ASSERT_EQUAL_UINT8(23, ControlCalibracion::pasoRampaActual(255, 140, 247, 5));
}

void test_control_manual_mezcla_satura_y_respeta_lease() {
  const auto avance = ControlManual::mezclar(1.0f, 0.0f, 230);
  TEST_ASSERT_EQUAL_INT(230, avance.izquierdo);
  TEST_ASSERT_EQUAL_INT(230, avance.derecho);
  const auto pivote = ControlManual::mezclar(0.0f, 1.0f, 230);
  TEST_ASSERT_EQUAL_INT(230, pivote.izquierdo);
  TEST_ASSERT_EQUAL_INT(-230, pivote.derecho);
  const auto combinado = ControlManual::mezclar(1.0f, 1.0f, 230);
  TEST_ASSERT_TRUE(abs(combinado.izquierdo) <= 230);
  TEST_ASSERT_TRUE(abs(combinado.derecho) <= 230);
  TEST_ASSERT_TRUE(ControlManual::leaseVigente(1299, 1000, 300));
  TEST_ASSERT_FALSE(ControlManual::leaseVigente(1301, 1000, 300));
  TEST_ASSERT_TRUE(ControlManual::leaseVigente(20, 0xFFFFFFF0u, 300));
  TEST_ASSERT_TRUE(ControlManual::esperaPrimerFrameVigente(1500, 1000, 500));
  TEST_ASSERT_FALSE(ControlManual::esperaPrimerFrameVigente(1501, 1000, 500));
  TEST_ASSERT_EQUAL_INT(8, ControlManual::acercar(0, 230, 8));
  TEST_ASSERT_EQUAL_INT(-8, ControlManual::acercar(0, -230, 8));
  TEST_ASSERT_EQUAL_INT(0, ControlManual::acercar(200, 0, 8));
}

void test_calibracion_mpu_confirma_con_solo_bl_como_sesion_185() {
  const int64_t deltas[4] = {0, 0, 22, 0};
  const auto evidencia = ControlCalibracion::evaluarMovimiento(0.13f, 0.12f, deltas, 2);
  TEST_ASSERT_TRUE(evidencia.gyroConfirmado);
  TEST_ASSERT_TRUE(evidencia.pcntCorroborado);
  TEST_ASSERT_EQUAL_UINT8(1, evidencia.encodersQueResponden);
  TEST_ASSERT_TRUE(evidencia.confirmada());
}

void test_calibracion_rechaza_cero_un_tick_y_pcnt_sin_giro() {
  const int64_t cero[4] = {0, 0, 0, 0};
  const int64_t ruido[4] = {0, 0, 1, 0};
  const int64_t pcnt[4] = {0, 0, 2, 0};
  TEST_ASSERT_FALSE(ControlCalibracion::evaluarMovimiento(0.13f, 0.12f, cero, 2).confirmada());
  TEST_ASSERT_FALSE(ControlCalibracion::evaluarMovimiento(0.13f, 0.12f, ruido, 2).confirmada());
  TEST_ASSERT_FALSE(ControlCalibracion::evaluarMovimiento(0.11f, 0.12f, pcnt, 2).confirmada());
}

void test_calibracion_ordena_pivote_logico_sin_traslacion() {
  const auto positivo = ControlCalibracion::comandoPivot(1, 560);
  const auto negativo = ControlCalibracion::comandoPivot(-1, 560);
  TEST_ASSERT_EQUAL_INT(-positivo.izquierda, positivo.derecha);
  TEST_ASSERT_EQUAL_INT(-negativo.izquierda, negativo.derecha);
  TEST_ASSERT_EQUAL_INT(-positivo.izquierda, negativo.izquierda);
  // La polaridad física vigente (-1) conserva lados opuestos en el puente.
  TEST_ASSERT_EQUAL_INT(-(positivo.izquierda * -1), positivo.derecha * -1);
}

void test_pcnt_registra_la_primera_etapa_fallida_y_bloquea_el_conjunto() {
  using namespace ControlInicializacionPCNT;
  const Etapa etapas[] = {Etapa::CONFIGURACION, Etapa::FILTRO_VALOR,
      Etapa::FILTRO_HABILITAR, Etapa::PAUSA, Etapa::LIMPIEZA, Etapa::ARRANQUE};
  for (Etapa etapa : etapas) {
    Canal canales[4] = {};
    TEST_ASSERT_TRUE(todosListos(canales));
    TEST_ASSERT_FALSE(registrar(canales[2], etapa, -7));
    TEST_ASSERT_FALSE(todosListos(canales));
    TEST_ASSERT_EQUAL_INT(-7, canales[2].codigoError);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(etapa),
                          static_cast<int>(canales[2].etapaFallida));
    registrar(canales[2], Etapa::ARRANQUE, -9);
    TEST_ASSERT_EQUAL_INT(-7, canales[2].codigoError);
  }
}

void test_giro_sin_pcnt_aborta_a_500_ms_y_tolera_rollover() {
  const int64_t sinPulsos[4] = {0, 0, 0, 0};
  ControlCalibracion::VigilanciaSilencioEncoders vigilancia = {};
  TEST_ASSERT_FALSE(ControlCalibracion::actualizarSilencioConGiro(
      vigilancia, true, sinPulsos, 1000, 500));
  TEST_ASSERT_FALSE(ControlCalibracion::actualizarSilencioConGiro(
      vigilancia, true, sinPulsos, 1499, 500));
  TEST_ASSERT_TRUE(ControlCalibracion::actualizarSilencioConGiro(
      vigilancia, true, sinPulsos, 1500, 500));

  vigilancia = {};
  TEST_ASSERT_FALSE(ControlCalibracion::actualizarSilencioConGiro(
      vigilancia, true, sinPulsos, 0xFFFFFF00u, 500));
  TEST_ASSERT_TRUE(ControlCalibracion::actualizarSilencioConGiro(
      vigilancia, true, sinPulsos, 0x000000F4u, 500));
}

void test_silencio_pcnt_se_reinicia_con_ruido_imu_o_un_pulso() {
  const int64_t sinPulsos[4] = {0, 0, 0, 0};
  const int64_t conPulso[4] = {0, 1, 0, 0};
  ControlCalibracion::VigilanciaSilencioEncoders vigilancia = {};
  ControlCalibracion::actualizarSilencioConGiro(vigilancia, true, sinPulsos, 100, 500);
  TEST_ASSERT_FALSE(ControlCalibracion::actualizarSilencioConGiro(
      vigilancia, false, sinPulsos, 400, 500));
  TEST_ASSERT_FALSE(vigilancia.activa);
  ControlCalibracion::actualizarSilencioConGiro(vigilancia, true, sinPulsos, 500, 500);
  TEST_ASSERT_FALSE(ControlCalibracion::actualizarSilencioConGiro(
      vigilancia, true, conPulso, 900, 500));
  TEST_ASSERT_FALSE(vigilancia.activa);
}

void test_retorno_arma_vigilancia_al_alcanzar_torque_y_tolera_diez_segundos() {
  float referenciaError = 99.0f;
  uint32_t ultimoProgresoMs = 1000;
  ControlCalibracion::reiniciarVigilanciaRetorno(
      4.7f, 8000, referenciaError, ultimoProgresoMs);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.7f, referenciaError);
  TEST_ASSERT_EQUAL_UINT32(8000, ultimoProgresoMs);
  TEST_ASSERT_FALSE(ControlCalibracion::retornoSinProgreso(
      4.7f, 17999, 0.5f, 10000, referenciaError, ultimoProgresoMs));
  TEST_ASSERT_TRUE(ControlCalibracion::retornoSinProgreso(
      4.7f, 18000, 0.5f, 10000, referenciaError, ultimoProgresoMs));
}

void test_retorno_renueva_vigilancia_con_progreso_angular() {
  float referenciaError = 4.7f;
  uint32_t ultimoProgresoMs = 8000;
  TEST_ASSERT_FALSE(ControlCalibracion::retornoSinProgreso(
      4.1f, 17000, 0.5f, 10000, referenciaError, ultimoProgresoMs));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.1f, referenciaError);
  TEST_ASSERT_EQUAL_UINT32(17000, ultimoProgresoMs);
  TEST_ASSERT_FALSE(ControlCalibracion::retornoSinProgreso(
      4.0f, 26999, 0.5f, 10000, referenciaError, ultimoProgresoMs));
  TEST_ASSERT_TRUE(ControlCalibracion::retornoSinProgreso(
      4.0f, 27000, 0.5f, 10000, referenciaError, ultimoProgresoMs));
}

void test_retorno_vigilancia_tolera_rollover_de_millis() {
  float referenciaError = 4.7f;
  uint32_t ultimoProgresoMs = 0xFFFFFF00u;
  TEST_ASSERT_FALSE(ControlCalibracion::retornoSinProgreso(
      4.7f, 0x0000260Fu, 0.5f, 10000,
      referenciaError, ultimoProgresoMs));
  TEST_ASSERT_TRUE(ControlCalibracion::retornoSinProgreso(
      4.7f, 0x00002610u, 0.5f, 10000,
      referenciaError, ultimoProgresoMs));
}

void test_historial_torque_conserva_diez_y_calcula_base_por_polaridad() {
  ControlTorque::Historial historial = {};
  TEST_ASSERT_EQUAL_INT(140, ControlTorque::baseParaPolaridad(historial, 1));
  ControlTorque::Registro invalido = {1, 139, 170, 1, -1};
  ControlTorque::agregar(historial, invalido);
  TEST_ASSERT_EQUAL_UINT8(0, historial.cantidad);
  ControlTorque::agregar(historial, {1, 150, 170, 1, -1});
  TEST_ASSERT_EQUAL_UINT8(1, historial.cantidad);
  TEST_ASSERT_EQUAL_INT(145, ControlTorque::baseParaPolaridad(historial, 1));
  historial = {};
  for (int i = 0; i < 11; ++i) {
    ControlTorque::Registro registro = {
        static_cast<uint32_t>(i + 1), 150 + i, 170 + i, 1, -1};
    ControlTorque::agregar(historial, registro);
  }
  TEST_ASSERT_EQUAL_UINT8(10, historial.cantidad);
  TEST_ASSERT_EQUAL_UINT32(2, historial.registros[0].secuencia);
  TEST_ASSERT_EQUAL_INT(151, historial.registros[0].pwmPositivo8);
  TEST_ASSERT_EQUAL_INT(151, ControlTorque::baseParaPolaridad(historial, 1));
  TEST_ASSERT_EQUAL_INT(171, ControlTorque::baseParaPolaridad(historial, -1));
  TEST_ASSERT_EQUAL_INT(140, ControlTorque::baseDesdePromedio(140));
}

void test_pivot_centrado_sesion_227_y_ventanas() {
  const int64_t a[4] = {0,0,3,2}, b[4] = {0,0,45,4}, unilateral[4] = {0,0,22,0};
  TEST_ASSERT_TRUE(ControlCalibracion::evaluarPivot(1.1f, a).equilibrado);
  TEST_ASSERT_TRUE(ControlCalibracion::evaluarPivot(1.1f, unilateral).torque);
  TEST_ASSERT_FALSE(ControlCalibracion::evaluarPivot(1.1f, unilateral).equilibrado);
  TEST_ASSERT_FALSE(ControlCalibracion::evaluarPivot(1.1f, b).equilibrado);
  TEST_ASSERT_FALSE(ControlCalibracion::evaluarPivot(0.9f, a).torque);
  ControlCalibracion::VigilanciaPivot v;
  const int64_t d0[4] = {0,0,2,0}, d1[4] = {0,0,8,0}, d2[4] = {0,0,15,0}, d3[4] = {0,0,22,0};
  TEST_ASSERT_NULL(ControlCalibracion::vigilarPivot(v, ControlCalibracion::evaluarPivot(0.1f,d0),0.1f,d0,10));
  TEST_ASSERT_FALSE(v.movimiento);
  TEST_ASSERT_NULL(ControlCalibracion::vigilarPivot(v, ControlCalibracion::evaluarPivot(1.0f,d1),1.0f,d1,260));
  TEST_ASSERT_TRUE(v.movimiento);
  TEST_ASSERT_NULL(ControlCalibracion::vigilarPivot(v, ControlCalibracion::evaluarPivot(1.1f,d2),1.1f,d2,510));
  TEST_ASSERT_EQUAL_STRING("cal_pivot_one_side_only", ControlCalibracion::vigilarPivot(v, ControlCalibracion::evaluarPivot(1.2f,d3),1.2f,d3,760));
  v = {};
  ControlCalibracion::vigilarPivot(v, ControlCalibracion::evaluarPivot(1.0f,b),1.0f,b,0xfffffff0U);
  TEST_ASSERT_EQUAL_STRING("cal_pivot_asymmetric", ControlCalibracion::vigilarPivot(v, ControlCalibracion::evaluarPivot(1.1f,b),1.1f,b,1484));
}

void test_historial_torque_por_lado() {
  ControlTorque::Historial h;
  ControlTorque::Registro r = {1,150,170,1,-1};
  TEST_ASSERT_EQUAL_INT(150,r.pwmPositivoDerecho8);
  r.pwmPositivoDerecho8 = 200; r.pwmNegativoDerecho8 = 210;
  ControlTorque::agregar(h,r);
  TEST_ASSERT_EQUAL_INT(145,ControlTorque::baseParaPolaridad(h,1,true));
  TEST_ASSERT_EQUAL_INT(195,ControlTorque::baseParaPolaridad(h,1,false));
  TEST_ASSERT_EQUAL_INT(205,ControlTorque::baseParaPolaridad(h,-1,false));
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_desconexion_solo_cancela_calibracion);
  RUN_TEST(test_pivot_centrado_sesion_227_y_ventanas);
  RUN_TEST(test_historial_torque_por_lado);
  RUN_TEST(test_lateral_derecha_corrige_hacia_izquierda);
  RUN_TEST(test_lateral_izquierda_corrige_hacia_derecha);
  RUN_TEST(test_errores_vectoriales_son_consistentes_en_rumbos_diagonales);
  RUN_TEST(test_reversa_automatica_conserva_el_chasis_ante_objetivo_detras);
  RUN_TEST(test_reversa_invierte_solo_la_correccion_lateral_del_chasis);
  RUN_TEST(test_reversa_invierte_el_lado_frenado_por_el_pid_de_rumbo);
  RUN_TEST(test_escala_y_freno_se_calculan_en_la_misma_unidad);
  RUN_TEST(test_integral_se_acota_y_no_crece_en_saturacion);
  RUN_TEST(test_integral_acumula_y_se_limita_fuera_de_saturacion);
  RUN_TEST(test_distancia_sola_no_acepta_endpoint);
  RUN_TEST(test_fallo_endpoint_despues_de_dos_intentos);
  RUN_TEST(test_endpoint_corto_pide_calibracion_y_el_largo_se_recupera);
  RUN_TEST(test_imu_perdida_se_detecta);
  RUN_TEST(test_encoder_incoherente_se_detecta);
  RUN_TEST(test_pico_pcnt_imposible_no_puede_contaminar_odometria);
  RUN_TEST(test_fusion_descarta_cero_aislado_sin_sesgar_distancia);
  RUN_TEST(test_fusion_falla_si_un_lado_completo_no_es_confiable);
  RUN_TEST(test_stall_por_lado_exige_dos_encoders_sin_pulsos);
  RUN_TEST(test_cualquier_encoder_individual_puede_fallar_sin_perder_un_lado);
  RUN_TEST(test_antifriccion_escala_siete_pulsos_y_exige_ambos_lados);
  RUN_TEST(test_stop_no_borra_fallo_o_estop_enclavado);
  RUN_TEST(test_estop_se_reconoce);
  RUN_TEST(test_calibracion_conserva_promedio_de_ambos_encoders_por_lado);
  RUN_TEST(test_calibracion_rechaza_un_lado_completo_sin_pulsos);
  RUN_TEST(test_calibracion_identifica_cualquier_encoder_aislado);
  RUN_TEST(test_calibracion_con_encoder_aislado_usa_promedio_confiable_sin_dividir_entre_dos);
  RUN_TEST(test_calibracion_aisla_encoder_con_ruido_menor_al_umbral_y_confirma_lado_sano);
  RUN_TEST(test_rampa_calibracion_expone_todos_los_niveles);
  RUN_TEST(test_control_manual_mezcla_satura_y_respeta_lease);
  RUN_TEST(test_calibracion_mpu_confirma_con_solo_bl_como_sesion_185);
  RUN_TEST(test_calibracion_rechaza_cero_un_tick_y_pcnt_sin_giro);
  RUN_TEST(test_calibracion_ordena_pivote_logico_sin_traslacion);
  RUN_TEST(test_pcnt_registra_la_primera_etapa_fallida_y_bloquea_el_conjunto);
  RUN_TEST(test_giro_sin_pcnt_aborta_a_500_ms_y_tolera_rollover);
  RUN_TEST(test_silencio_pcnt_se_reinicia_con_ruido_imu_o_un_pulso);
  RUN_TEST(test_retorno_arma_vigilancia_al_alcanzar_torque_y_tolera_diez_segundos);
  RUN_TEST(test_retorno_renueva_vigilancia_con_progreso_angular);
  RUN_TEST(test_retorno_vigilancia_tolera_rollover_de_millis);
  RUN_TEST(test_historial_torque_conserva_diez_y_calcula_base_por_polaridad);
  return UNITY_END();
}
