#include <unity.h>

#include "ControlRuta.h"
#include "ControlSeguridad.h"

extern "C" void setUp() {}
extern "C" void tearDown() {}

namespace {

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

void test_estop_se_reconoce() {
  TEST_ASSERT_TRUE(ControlSeguridad::estopSolicitado(true));
  TEST_ASSERT_FALSE(ControlSeguridad::estopSolicitado(false));
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_lateral_derecha_corrige_hacia_izquierda);
  RUN_TEST(test_lateral_izquierda_corrige_hacia_derecha);
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
  RUN_TEST(test_fusion_descarta_cero_aislado_sin_sesgar_distancia);
  RUN_TEST(test_fusion_falla_si_un_lado_completo_no_es_confiable);
  RUN_TEST(test_stall_por_lado_exige_dos_encoders_sin_pulsos);
  RUN_TEST(test_estop_se_reconoce);
  return UNITY_END();
}
