#include <Arduino.h>
#include <unity.h>

#include "ControlRuta.h"

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
}

}  // namespace

void setup() {
  delay(1000);
  UNITY_BEGIN();
  RUN_TEST(test_lateral_derecha_corrige_hacia_izquierda);
  RUN_TEST(test_lateral_izquierda_corrige_hacia_derecha);
  RUN_TEST(test_integral_se_acota_y_no_crece_en_saturacion);
  RUN_TEST(test_integral_acumula_y_se_limita_fuera_de_saturacion);
  RUN_TEST(test_distancia_sola_no_acepta_endpoint);
  RUN_TEST(test_fallo_endpoint_despues_de_dos_intentos);
  UNITY_END();
}

void loop() {}
