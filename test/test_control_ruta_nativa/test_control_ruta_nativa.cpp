#include <unity.h>

#include "Config.h"
#include "ControlRuta.h"
#include "ControlSeguridad.h"
#include "ControlCalibracion.h"
#include "ControlManual.h"
#include "ControlInicializacionPCNT.h"
#include "ControlSupervision.h"

extern "C" void setUp() {}
extern "C" void tearDown() {}

namespace {

void test_polaridad_fisica_invierte_avance_y_reversa_sin_alterar_magnitud() {
  TEST_ASSERT_EQUAL_INT(-320, ControlMotores::pwmElectricoDesdeLogico(320));
  TEST_ASSERT_EQUAL_INT(320, ControlMotores::pwmElectricoDesdeLogico(-320));
  TEST_ASSERT_EQUAL_INT(0, ControlMotores::pwmElectricoDesdeLogico(0));
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
  TEST_ASSERT_FALSE(ControlRuta::frenarLadoIzquierdoParaRumbo(1.0f, 1));
  TEST_ASSERT_TRUE(ControlRuta::frenarLadoIzquierdoParaRumbo(1.0f, -1));
  TEST_ASSERT_TRUE(ControlRuta::frenarLadoIzquierdoParaRumbo(-1.0f, 1));
  TEST_ASSERT_FALSE(ControlRuta::frenarLadoIzquierdoParaRumbo(-1.0f, -1));
}

void test_marco_cardinal_es_x_derecha_y_frente_y_yaw_horario() {
  const auto norte = ControlRuta::vectorUnitarioRumbo(0.0f);
  const auto este = ControlRuta::vectorUnitarioRumbo(90.0f);
  const auto sur = ControlRuta::vectorUnitarioRumbo(180.0f);
  const auto oeste = ControlRuta::vectorUnitarioRumbo(270.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, norte.x);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, norte.y);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, este.x);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, este.y);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, sur.x);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, sur.y);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, oeste.x);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, oeste.y);
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

void test_calibracion_usa_solo_fuentes_que_responden_por_lado() {
  const int64_t deltas[4] = {0, 4, 4, 4};
  const auto evaluacion = ControlCalibracion::evaluarEncoders(deltas, 2);
  TEST_ASSERT_EQUAL_INT64(4, evaluacion.promedioIzquierdo);
  TEST_ASSERT_EQUAL_UINT8(1, evaluacion.fuentesIzquierdas);
  TEST_ASSERT_EQUAL_UINT8(2, evaluacion.fuentesDerechas);
  TEST_ASSERT_TRUE(evaluacion.ladoIzquierdoValido);
  TEST_ASSERT_TRUE(evaluacion.ladoDerechoValido);
  TEST_ASSERT_TRUE(evaluacion.sinRespuestaAislada[0]);
  TEST_ASSERT_FALSE(evaluacion.sinRespuestaAislada[2]);
}

void test_calibracion_acepta_exactamente_un_encoder_sano_por_lado() {
  const int64_t deltas[4] = {3, 0, 0, 5};
  const auto evaluacion = ControlCalibracion::evaluarEncoders(deltas, 2);
  TEST_ASSERT_TRUE(evaluacion.ladoIzquierdoValido);
  TEST_ASSERT_TRUE(evaluacion.ladoDerechoValido);
  TEST_ASSERT_EQUAL_INT64(3, evaluacion.promedioIzquierdo);
  TEST_ASSERT_EQUAL_INT64(5, evaluacion.promedioDerecho);
  TEST_ASSERT_TRUE(evaluacion.sinRespuestaAislada[2]);
  TEST_ASSERT_TRUE(evaluacion.sinRespuestaAislada[1]);
}

void test_pcnt_permite_degradado_si_conserva_una_fuente_por_lado() {
  using namespace ControlInicializacionPCNT;
  Canal canales[4] = {};
  TEST_ASSERT_TRUE(todosListos(canales));
  registrar(canales[0], Etapa::CONFIGURACION, -7);
  registrar(canales[1], Etapa::ARRANQUE, -8);
  TEST_ASSERT_FALSE(todosListos(canales));
  TEST_ASSERT_TRUE(fuentesPorLadoDisponibles(canales));
  registrar(canales[2], Etapa::FILTRO_HABILITAR, -9);
  TEST_ASSERT_FALSE(fuentesPorLadoDisponibles(canales));
  TEST_ASSERT_EQUAL_STRING("config", textoEtapa(canales[0].etapaFallida));
  TEST_ASSERT_EQUAL_INT(-7, canales[0].codigoError);
}

void test_encoder_excluido_solo_reingresa_con_pulsos_coherentes_y_pwm() {
  TEST_ASSERT_TRUE(ControlSeguridad::encoderPuedeReingresar(
      10, 11, 10.5f, true, 0.40f));
  TEST_ASSERT_FALSE(ControlSeguridad::encoderPuedeReingresar(
      10, 11, 10.5f, false, 0.40f));
  TEST_ASSERT_FALSE(ControlSeguridad::encoderPuedeReingresar(
      0, 11, 10.5f, true, 0.40f));
  TEST_ASSERT_FALSE(ControlSeguridad::encoderPuedeReingresar(
      3, 11, 10.5f, true, 0.40f));
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

void test_rampa_calibracion_expone_todos_los_niveles() {
  TEST_ASSERT_EQUAL_UINT8(23, ControlCalibracion::totalPasosRampa(140, 247, 5));
  TEST_ASSERT_EQUAL_UINT8(1, ControlCalibracion::pasoRampaActual(140, 140, 247, 5));
  TEST_ASSERT_EQUAL_UINT8(2, ControlCalibracion::pasoRampaActual(145, 140, 247, 5));
  TEST_ASSERT_EQUAL_UINT8(23, ControlCalibracion::pasoRampaActual(247, 140, 247, 5));
  TEST_ASSERT_EQUAL_UINT8(23, ControlCalibracion::pasoRampaActual(255, 140, 247, 5));
}

void test_pivote_usa_una_fuente_sana_por_lado_y_no_suma_la_pareja() {
  const int64_t deltas[4] = {0, 12, 10, 0};
  const bool confiable[4] = {false, true, true, false};
  const auto evidencia = ControlCalibracion::evaluarPivot(
      4.0f, deltas, confiable, 2, 0.45f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, evidencia.izquierda);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.0f, evidencia.derecha);
  TEST_ASSERT_TRUE(evidencia.bilateral);
  TEST_ASSERT_TRUE(evidencia.equilibrado);
  TEST_ASSERT_TRUE(evidencia.torque);
}

void test_balance_pivote_reduce_lado_adelantado_y_refuerza_rezagado() {
  const auto positivo = ControlCalibracion::comandoPivotCentrado(
      1, 700, 20.0f, 10.0f, 2.0f, 80, 990);
  TEST_ASSERT_EQUAL_INT(-680, positivo.izquierda);
  TEST_ASSERT_EQUAL_INT(720, positivo.derecha);
  TEST_ASSERT_EQUAL_INT(20, positivo.correccion);
  const auto negativo = ControlCalibracion::comandoPivotCentrado(
      -1, 700, 10.0f, 20.0f, 2.0f, 80, 990);
  TEST_ASSERT_EQUAL_INT(720, negativo.izquierda);
  TEST_ASSERT_EQUAL_INT(-680, negativo.derecha);
  TEST_ASSERT_EQUAL_INT(-20, negativo.correccion);
}

void test_calibracion_no_acepta_retorno_fuera_del_origen() {
  TEST_ASSERT_TRUE(ControlCalibracion::retornoAlOrigenAceptable(
      1.0f, 1.0f, 0.5f, 3.0f, 1.0f));
  TEST_ASSERT_FALSE(ControlCalibracion::retornoAlOrigenAceptable(
      3.0f, 2.0f, 0.5f, 3.0f, 1.0f));
  TEST_ASSERT_FALSE(ControlCalibracion::retornoAlOrigenAceptable(
      0.0f, 0.0f, 1.1f, 3.0f, 1.0f));
}

void test_supervision_vencida_detiene_cualquier_movimiento() {
  TEST_ASSERT_TRUE(ControlSupervision::movimientoRequiereLease(true, false, false));
  TEST_ASSERT_TRUE(ControlSupervision::movimientoRequiereLease(false, true, false));
  TEST_ASSERT_TRUE(ControlSupervision::movimientoRequiereLease(false, false, true));
  TEST_ASSERT_FALSE(ControlSupervision::movimientoRequiereLease(false, false, false));
  TEST_ASSERT_FALSE(ControlSupervision::leaseVencido(1499, 1000, 500));
  TEST_ASSERT_TRUE(ControlSupervision::leaseVencido(1501, 1000, 500));
  TEST_ASSERT_TRUE(ControlSupervision::leaseVencido(100, 0, 500));
  TEST_ASSERT_FALSE(ControlSupervision::leaseVencido(20, 0xFFFFFFF0u, 500));
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
  TEST_ASSERT_EQUAL_INT(8, ControlManual::acercar(0, 230, 8));
  TEST_ASSERT_EQUAL_INT(-8, ControlManual::acercar(0, -230, 8));
  TEST_ASSERT_EQUAL_INT(0, ControlManual::acercar(200, 0, 8));
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_polaridad_fisica_invierte_avance_y_reversa_sin_alterar_magnitud);
  RUN_TEST(test_lateral_derecha_corrige_hacia_izquierda);
  RUN_TEST(test_lateral_izquierda_corrige_hacia_derecha);
  RUN_TEST(test_errores_vectoriales_son_consistentes_en_rumbos_diagonales);
  RUN_TEST(test_reversa_automatica_conserva_el_chasis_ante_objetivo_detras);
  RUN_TEST(test_reversa_invierte_solo_la_correccion_lateral_del_chasis);
  RUN_TEST(test_reversa_invierte_el_lado_frenado_por_el_pid_de_rumbo);
  RUN_TEST(test_marco_cardinal_es_x_derecha_y_frente_y_yaw_horario);
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
  RUN_TEST(test_calibracion_usa_solo_fuentes_que_responden_por_lado);
  RUN_TEST(test_calibracion_acepta_exactamente_un_encoder_sano_por_lado);
  RUN_TEST(test_pcnt_permite_degradado_si_conserva_una_fuente_por_lado);
  RUN_TEST(test_encoder_excluido_solo_reingresa_con_pulsos_coherentes_y_pwm);
  RUN_TEST(test_calibracion_rechaza_un_lado_completo_sin_pulsos);
  RUN_TEST(test_calibracion_identifica_cualquier_encoder_aislado);
  RUN_TEST(test_rampa_calibracion_expone_todos_los_niveles);
  RUN_TEST(test_pivote_usa_una_fuente_sana_por_lado_y_no_suma_la_pareja);
  RUN_TEST(test_balance_pivote_reduce_lado_adelantado_y_refuerza_rezagado);
  RUN_TEST(test_calibracion_no_acepta_retorno_fuera_del_origen);
  RUN_TEST(test_supervision_vencida_detiene_cualquier_movimiento);
  RUN_TEST(test_control_manual_mezcla_satura_y_respeta_lease);
  return UNITY_END();
}
