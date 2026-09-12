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

void test_mapeo_fisico_individual_conserva_gpio_y_signo_logico() {
  TEST_ASSERT_EQUAL_STRING("robot-s3-v3.5", FIRMWARE_VERSION);
  TEST_ASSERT_EQUAL_INT(6, PIN_FL_FWD);
  TEST_ASSERT_EQUAL_INT(7, PIN_FL_REV);
  TEST_ASSERT_EQUAL_INT(4, PIN_BL_FWD);
  TEST_ASSERT_EQUAL_INT(5, PIN_BL_REV);
  TEST_ASSERT_EQUAL_INT(17, PIN_FR_FWD);
  TEST_ASSERT_EQUAL_INT(18, PIN_FR_REV);
  TEST_ASSERT_EQUAL_INT(15, PIN_BR_FWD);
  TEST_ASSERT_EQUAL_INT(16, PIN_BR_REV);
  const auto avance = ControlMotores::resolverSalida(PIN_FL_FWD, PIN_FL_REV, 320);
  TEST_ASSERT_EQUAL_INT(6, avance.gpioActivo);
  TEST_ASSERT_EQUAL_INT(320, avance.duty);
  TEST_ASSERT_EQUAL_INT(0, avance.dutyReversa);
  const auto reversa = ControlMotores::resolverSalida(PIN_FL_FWD, PIN_FL_REV, -320);
  TEST_ASSERT_EQUAL_INT(7, reversa.gpioActivo);
  TEST_ASSERT_EQUAL_INT(320, reversa.duty);
  const auto cero = ControlMotores::resolverSalida(PIN_FL_FWD, PIN_FL_REV, 0);
  TEST_ASSERT_EQUAL_INT(-1, cero.gpioActivo);
  TEST_ASSERT_EQUAL_INT(0, cero.duty);
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

void test_rumbo_final_en_reversa_conserva_el_cuerpo_sin_pivote_parasito() {
  // En avance (+1), el rumbo final es el rumbo de trayecto ordenado
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 180.0f,
                           ControlRuta::rumboFinalParaPaso(180.0f, 1, 180.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f,
                           ControlRuta::rumboFinalParaPaso(90.0f, 1, 90.0f));
  // En reversa (-1), para viajar al Sur (180°), el cuerpo está alineado a 0°.
  // El rumbo final DEBE ser 0° (el cuerpo) para no ejecutar giro_fin parásito de 180°.
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f,
                           ControlRuta::rumboFinalParaPaso(180.0f, -1, 0.0f));
  // En reversa (-1), para viajar al Este (90°), el cuerpo está alineado a 270°.
  // El rumbo final DEBE ser 270° (el cuerpo), no 90°.
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 270.0f,
                           ControlRuta::rumboFinalParaPaso(90.0f, -1, 270.0f));
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

void test_presupuesto_unico_permite_21_reingresos_y_bloquea_el_22() {
  TEST_ASSERT_EQUAL_UINT8(21, ROUTE_RECOVERY_MAX_ATTEMPTS);
  TEST_ASSERT_FALSE(ControlRuta::agotoIntentosEndpoint(
      ROUTE_RECOVERY_MAX_ATTEMPTS - 1, INTENTOS_RECUPERACION_ENDPOINT_MAX));
  TEST_ASSERT_TRUE(ControlRuta::agotoIntentosEndpoint(
      ROUTE_RECOVERY_MAX_ATTEMPTS, INTENTOS_RECUPERACION_ENDPOINT_MAX));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlRuta::DecisionEndpoint::RECUPERAR),
      static_cast<uint8_t>(ControlRuta::decidirEndpoint(
          true, false, ROUTE_RECOVERY_MAX_ATTEMPTS - 1,
          INTENTOS_RECUPERACION_ENDPOINT_MAX)));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlRuta::DecisionEndpoint::FALLAR),
      static_cast<uint8_t>(ControlRuta::decidirEndpoint(
          true, false, ROUTE_RECOVERY_MAX_ATTEMPTS,
          INTENTOS_RECUPERACION_ENDPOINT_MAX)));
}

void test_endpoint_corto_falla_y_el_largo_se_recupera() {
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ControlRuta::DecisionEndpoint::FALLAR),
                          static_cast<uint8_t>(ControlRuta::decidirEndpointSeguro(
                              true, false, 0, 2, 1.8f, DISTANCIA_MINIMA_RECUPERACION_ENDPOINT_CM)));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ControlRuta::DecisionEndpoint::RECUPERAR),
                          static_cast<uint8_t>(ControlRuta::decidirEndpointSeguro(
                              true, false, 0, 2, 5.2f, DISTANCIA_MINIMA_RECUPERACION_ENDPOINT_CM)));
}

void test_reingreso_se_proyecta_sobre_ejes_ortogonales_y_se_limita_al_endpoint() {
  const auto vertical = ControlRuta::calcularPuntoReingreso(
      6.0f, 40.0f, 0.0f, 100.0f, 0.0f, 100.0f, 6.0f, 10.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vertical.xCm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 52.0f, vertical.yCm);
  const auto horizontal = ControlRuta::calcularPuntoReingreso(
      92.0f, -7.0f, 100.0f, 0.0f, 90.0f, 100.0f, -7.0f, 10.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, horizontal.xCm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, horizontal.yCm);
}

void test_disparadores_recentrado_y_sentido_incorrecto_exigen_persistencia() {
  TEST_ASSERT_FALSE(ControlRuta::debeRecentrar(
      5.0f, 299, 5.0f, 300, 0.0f, 0, 1.0f, 500));
  TEST_ASSERT_TRUE(ControlRuta::debeRecentrar(
      5.1f, 300, 5.0f, 300, 0.0f, 0, 1.0f, 500));
  TEST_ASSERT_TRUE(ControlRuta::debeRecentrar(
      3.0f, 0, 5.0f, 300, 1.0f, 500, 1.0f, 500));
  TEST_ASSERT_FALSE(ControlRuta::progresoEnSentidoIncorrecto(
      52.0f, 50.0f, 399, 2.0f, 400));
  TEST_ASSERT_TRUE(ControlRuta::progresoEnSentidoIncorrecto(
      52.0f, 50.0f, 400, 2.0f, 400));
}

void test_guard_maniobra_acepta_reversa_convergente_y_frena_divergencia() {
  const float objetivos[4][2] = {
      {0.0f, 100.0f}, {100.0f, 0.0f}, {0.0f, -100.0f}, {-100.0f, 0.0f}};
  const float sobrepasos[4][2] = {
      {0.0f, 120.0f}, {120.0f, 0.0f}, {0.0f, -120.0f}, {-120.0f, 0.0f}};
  const float regresos[4][2] = {
      {0.0f, 116.0f}, {116.0f, 0.0f}, {0.0f, -116.0f}, {-116.0f, 0.0f}};

  for (int i = 0; i < 4; ++i) {
    ControlRuta::SeguimientoProgreso seguimiento{};
    const float inicial = hypotf(objetivos[i][0] - sobrepasos[i][0],
                                 objetivos[i][1] - sobrepasos[i][1]);
    const float convergente = hypotf(objetivos[i][0] - regresos[i][0],
                                     objetivos[i][1] - regresos[i][1]);
    ControlRuta::iniciarSeguimientoProgreso(seguimiento, inicial);
    TEST_ASSERT_FALSE(ControlRuta::actualizarSeguimientoProgreso(
        seguimiento, convergente, 100, 2.0f, 400));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 16.0f, seguimiento.mejorRestanteCm);
  }

  ControlRuta::SeguimientoProgreso divergente{};
  ControlRuta::iniciarSeguimientoProgreso(divergente, 16.0f);
  TEST_ASSERT_FALSE(ControlRuta::actualizarSeguimientoProgreso(
      divergente, 18.0f, 0, 2.0f, 400));
  TEST_ASSERT_FALSE(ControlRuta::actualizarSeguimientoProgreso(
      divergente, 18.0f, 399, 2.0f, 400));
  TEST_ASSERT_TRUE(ControlRuta::actualizarSeguimientoProgreso(
      divergente, 18.0f, 400, 2.0f, 400));
}

void test_realineacion_conserva_progreso_en_baselines_repetidos() {
  const float cmPorTick = 0.5f;
  float acumulada = ControlRuta::distanciaConBaseline(0.0f, 80.0f, cmPorTick);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 40.0f, acumulada);

  // El pivote ocurre antes de tomar el baseline siguiente: sus ticks no se
  // pasan a esta función. Sólo los ocho centímetros traducidos se agregan.
  acumulada = ControlRuta::distanciaConBaseline(acumulada, 16.0f, cmPorTick);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 48.0f, acumulada);
  acumulada = ControlRuta::distanciaConBaseline(acumulada, 4.0f, cmPorTick);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, acumulada);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, 100.0f - (100.0f - acumulada));
}

void test_episodio_recuperacion_no_renueva_reintentos_y_cubre_rollover() {
  ControlRuta::EpisodioRecuperacion episodio{};
  ControlRuta::abrirEpisodioRecuperacion(episodio, 100);
  ControlRuta::abrirEpisodioRecuperacion(episodio, 29000);  // verify_retry
  TEST_ASSERT_EQUAL_UINT32(100, episodio.inicioMs);
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlRuta::EstadoEpisodioRecuperacion::ACTIVO),
      static_cast<uint8_t>(ControlRuta::procesarEpisodioRecuperacion(
          episodio, 30099, 30000, false)));
  // La transición a resumed en el instante exacto del límite debe fallar antes
  // de poder cerrar y renovar el presupuesto.
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlRuta::EstadoEpisodioRecuperacion::VENCIDO),
      static_cast<uint8_t>(ControlRuta::procesarEpisodioRecuperacion(
          episodio, 30100, 30000, true)));
  TEST_ASSERT_TRUE(episodio.activo);

  ControlRuta::cancelarEpisodioRecuperacion(episodio);
  ControlRuta::abrirEpisodioRecuperacion(episodio, 0xFFFFFFF0u);
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlRuta::EstadoEpisodioRecuperacion::ACTIVO),
      static_cast<uint8_t>(ControlRuta::procesarEpisodioRecuperacion(
          episodio, 0x00000020u, 49)));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlRuta::EstadoEpisodioRecuperacion::VENCIDO),
      static_cast<uint8_t>(ControlRuta::procesarEpisodioRecuperacion(
          episodio, 0x00000021u, 49)));
}

void test_reingreso_exige_centro_y_mejora_real() {
  TEST_ASSERT_TRUE(ControlRuta::reingresoAceptable(2.0f, 5.0f, 2.0f, 5.0f));
  TEST_ASSERT_FALSE(ControlRuta::reingresoAceptable(2.1f, 4.0f, 2.0f, 5.0f));
  TEST_ASSERT_FALSE(ControlRuta::reingresoDivergente(
      6.0f, 5.2f, 2.0f, 900, 1.0f, 3.0f, 1000));
  TEST_ASSERT_TRUE(ControlRuta::reingresoDivergente(
      6.0f, 5.2f, 3.0f, 900, 1.0f, 3.0f, 1000));
  TEST_ASSERT_FALSE(ControlRuta::reingresoDivergente(
      6.0f, 4.9f, 3.0f, 1000, 1.0f, 3.0f, 1000));
  TEST_ASSERT_TRUE(ControlRuta::desviacionFueraDeRango(20.1f, 20.0f));
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
  TEST_ASSERT_EQUAL_UINT8(44, ControlCalibracion::totalPasosRampa(560, 990, 10));
  TEST_ASSERT_EQUAL_UINT8(1, ControlCalibracion::pasoRampaActual(560, 560, 990, 10));
  TEST_ASSERT_EQUAL_UINT8(2, ControlCalibracion::pasoRampaActual(570, 560, 990, 10));
  TEST_ASSERT_EQUAL_UINT8(44, ControlCalibracion::pasoRampaActual(990, 560, 990, 10));
  TEST_ASSERT_EQUAL_UINT8(44, ControlCalibracion::pasoRampaActual(1023, 560, 990, 10));
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
  TEST_ASSERT_EQUAL_INT(680, positivo.izquierda);
  TEST_ASSERT_EQUAL_INT(-720, positivo.derecha);
  TEST_ASSERT_EQUAL_INT(20, positivo.correccion);
  const auto negativo = ControlCalibracion::comandoPivotCentrado(
      -1, 700, 10.0f, 20.0f, 2.0f, 80, 990);
  TEST_ASSERT_EQUAL_INT(-720, negativo.izquierda);
  TEST_ASSERT_EQUAL_INT(680, negativo.derecha);
  TEST_ASSERT_EQUAL_INT(-20, negativo.correccion);
}

void test_guard_pivote_exige_signo_mpu_y_frena_fallos() {
  const int64_t ticks[4] = {4, 4, 4, 4};
  const bool confiable[4] = {true, true, true, true};
  const auto evidencia = ControlCalibracion::evaluarPivot(
      4.0f, ticks, confiable, 2, 0.45f);
  // Confirmación por velocidad gyro sostenida
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ControlCalibracion::ResultadoGuardPivot::CONFIRMADO),
      static_cast<uint8_t>(ControlCalibracion::evaluarGuardPivot(
          1, 0.20f, 2.0f, evidencia, 120, 100, 0, 0.08f, 100, 350, 350, 0.5f)));
  // Confirmación por ángulo acumulado del MPU (yaw >= 0.5° aún con gyro bajo)
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ControlCalibracion::ResultadoGuardPivot::CONFIRMADO),
      static_cast<uint8_t>(ControlCalibracion::evaluarGuardPivot(
          1, 0.04f, 0.6f, evidencia, 120, 0, 0, 0.08f, 100, 350, 350, 0.5f)));
  // En progreso de validación MPU (ticks ok, pero sin yaw suficiente ni gyro sostenido aún)
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ControlCalibracion::ResultadoGuardPivot::CONFIRMANDO_YAW),
      static_cast<uint8_t>(ControlCalibracion::evaluarGuardPivot(
          1, 0.04f, 0.3f, evidencia, 120, 0, 0, 0.08f, 100, 350, 350, 0.5f)));
  // Signo MPU opuesto detectado
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ControlCalibracion::ResultadoGuardPivot::SIGNO_INCORRECTO),
      static_cast<uint8_t>(ControlCalibracion::evaluarGuardPivot(
          1, -0.20f, -2.0f, evidencia, 20, 0, 0, 0.08f, 100, 350, 350, 0.5f)));
  // Rotación no confirmada al expirar ventana de tiempo
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ControlCalibracion::ResultadoGuardPivot::ROTACION_NO_CONFIRMADA),
      static_cast<uint8_t>(ControlCalibracion::evaluarGuardPivot(
          1, 0.0f, 0.0f, evidencia, 350, 0, 0, 0.08f, 100, 350, 350, 0.5f)));
}

void test_guard_pivote_detecta_desbalance_persistente() {
  const int64_t ticks[4] = {2, 8, 2, 8};
  const bool confiable[4] = {true, true, true, true};
  const auto evidencia = ControlCalibracion::evaluarPivot(
      4.0f, ticks, confiable, 2, 0.20f);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ControlCalibracion::ResultadoGuardPivot::DESBALANCEADO),
      static_cast<uint8_t>(ControlCalibracion::evaluarGuardPivot(
          1, 0.20f, 2.0f, evidencia, 120, 100, 300, 0.12f, 100, 300, 300)));
}

void test_guard_no_frena_por_desbalance_mientras_aun_busca_torque() {
  const int64_t ticks[4] = {2, 8, 2, 8};
  const bool confiable[4] = {true, true, true, true};
  const auto evidencia = ControlCalibracion::evaluarPivot(
      0.2f, ticks, confiable, 1, 0.20f);
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlCalibracion::AccionGuardPivot::CONTINUAR_RAMPA),
      static_cast<uint8_t>(ControlCalibracion::decidirAccionGuardPivot(
          ControlCalibracion::ResultadoGuardPivot::DESBALANCEADO,
          700, 990, 1, 2)));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlCalibracion::AccionGuardPivot::PAUSAR_REINTENTO),
      static_cast<uint8_t>(ControlCalibracion::decidirAccionGuardPivot(
          ControlCalibracion::ResultadoGuardPivot::DESBALANCEADO,
          990, 990, 1, 2)));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlCalibracion::AccionGuardPivot::FALLAR),
      static_cast<uint8_t>(ControlCalibracion::decidirAccionGuardPivot(
          ControlCalibracion::ResultadoGuardPivot::DESBALANCEADO,
          990, 990, 2, 2)));
  TEST_ASSERT_TRUE(evidencia.bilateral);
  TEST_ASSERT_FALSE(evidencia.equilibrado);
}

void test_guard_relativo_ignora_ticks_anteriores_al_baseline() {
  const int64_t sin_tick_nuevo[4] = {0, 0, 0, 0};
  const bool confiable[4] = {true, true, true, true};
  const auto evidencia = ControlCalibracion::evaluarPivot(
      -0.6f, sin_tick_nuevo, confiable, 1, 0.45f);
  TEST_ASSERT_FALSE(evidencia.bilateral);
  TEST_ASSERT_FALSE(evidencia.equilibrado);
}

void test_guard_calibracion_conserva_once_intentos_independientes() {
  TEST_ASSERT_EQUAL_UINT8(11, CAL_GUARD_MAX_ATTEMPTS);
  TEST_ASSERT_EQUAL_UINT8(21, ROUTE_RECOVERY_MAX_ATTEMPTS);
  TEST_ASSERT_NOT_EQUAL(CAL_GUARD_MAX_ATTEMPTS, ROUTE_RECOVERY_MAX_ATTEMPTS);
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlCalibracion::AccionGuardPivot::PAUSAR_REINTENTO),
      static_cast<uint8_t>(ControlCalibracion::decidirAccionGuardPivot(
          ControlCalibracion::ResultadoGuardPivot::ROTACION_NO_CONFIRMADA,
          CALIBRATION_PWM_END, CALIBRATION_PWM_END, 10,
          CAL_GUARD_MAX_ATTEMPTS)));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlCalibracion::AccionGuardPivot::FALLAR),
      static_cast<uint8_t>(ControlCalibracion::decidirAccionGuardPivot(
          ControlCalibracion::ResultadoGuardPivot::ROTACION_NO_CONFIRMADA,
          CALIBRATION_PWM_END, CALIBRATION_PWM_END, 11,
          CAL_GUARD_MAX_ATTEMPTS)));
}

void test_calibracion_no_acepta_retorno_fuera_del_origen() {
  TEST_ASSERT_TRUE(ControlCalibracion::retornoAlOrigenAceptable(
      1.0f, 1.0f, 0.5f, 3.0f, 1.0f));
  TEST_ASSERT_FALSE(ControlCalibracion::retornoAlOrigenAceptable(
      3.0f, 2.0f, 0.5f, 3.0f, 1.0f));
  TEST_ASSERT_FALSE(ControlCalibracion::retornoAlOrigenAceptable(
      0.0f, 0.0f, 1.1f, 3.0f, 1.0f));
  TEST_ASSERT_TRUE(ControlCalibracion::retornoAlOrigenAceptable(
      3.0f, 3.0f, 0.5f, 5.2f, 1.0f));
  TEST_ASSERT_FALSE(ControlCalibracion::retornoAlOrigenAceptable(
      4.0f, 4.0f, 0.5f, 5.2f, 1.0f));
  TEST_ASSERT_TRUE(ControlCalibracion::retornoAlOrigenAceptable(
      6.0f, 6.0f, 0.5f, 10.0f, 1.0f));
  TEST_ASSERT_FALSE(ControlCalibracion::retornoAlOrigenAceptable(
      8.0f, 8.0f, 0.5f, 10.0f, 1.0f));
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
  TEST_ASSERT_TRUE(ControlManual::leaseVigente(20, 0xFFFFFFF0u, 300));
  TEST_ASSERT_EQUAL_INT(8, ControlManual::acercar(0, 230, 8));
  TEST_ASSERT_EQUAL_INT(-8, ControlManual::acercar(0, -230, 8));
  TEST_ASSERT_EQUAL_INT(0, ControlManual::acercar(200, 0, 8));
}

void test_modo_pulsado_umbral_y_decision() {
  TEST_ASSERT_TRUE(ControlRuta::tramoRequiereModoPulsado(15.0f, 15.0f));
  TEST_ASSERT_TRUE(ControlRuta::tramoRequiereModoPulsado(5.0f, 15.0f));
  TEST_ASSERT_TRUE(ControlRuta::tramoRequiereModoPulsado(0.5f, 15.0f));
  TEST_ASSERT_FALSE(ControlRuta::tramoRequiereModoPulsado(0.0f, 15.0f));
  TEST_ASSERT_FALSE(ControlRuta::tramoRequiereModoPulsado(-1.0f, 15.0f));
  TEST_ASSERT_FALSE(ControlRuta::tramoRequiereModoPulsado(15.1f, 15.0f));
  TEST_ASSERT_FALSE(ControlRuta::tramoRequiereModoPulsado(50.0f, 15.0f));
}

void test_calculo_pulso_traccion_y_correccion_rumbo() {
  const auto recto = ControlRuta::calcularPulsoTraccion(842, 0.0f, 12.0f, 50, 990);
  TEST_ASSERT_EQUAL_INT(842, recto.pwmIzquierdo);
  TEST_ASSERT_EQUAL_INT(842, recto.pwmDerecho);
  TEST_ASSERT_EQUAL_INT(0, recto.correccionDiferencial);

  const auto desvioPos = ControlRuta::calcularPulsoTraccion(842, 2.0f, 12.0f, 50, 990);
  TEST_ASSERT_EQUAL_INT(24, desvioPos.correccionDiferencial);
  TEST_ASSERT_EQUAL_INT(842 - 24, desvioPos.pwmIzquierdo);
  TEST_ASSERT_EQUAL_INT(842 + 24, desvioPos.pwmDerecho);

  const auto saturado = ControlRuta::calcularPulsoTraccion(842, 10.0f, 12.0f, 50, 990);
  TEST_ASSERT_EQUAL_INT(50, saturado.correccionDiferencial);
  TEST_ASSERT_EQUAL_INT(842 - 50, saturado.pwmIzquierdo);
  TEST_ASSERT_EQUAL_INT(842 + 50, saturado.pwmDerecho);

  const auto saturadoMax = ControlRuta::calcularPulsoTraccion(960, 10.0f, 12.0f, 50, 990);
  TEST_ASSERT_EQUAL_INT(960 - 50, saturadoMax.pwmIzquierdo);
  TEST_ASSERT_EQUAL_INT(990, saturadoMax.pwmDerecho);
}

void test_interlock_fin_pulsado_y_memoria_impulso() {
  TEST_ASSERT_TRUE(ControlRuta::interlockFinPulsado(2.5f, 2.5f, 1.0f));
  TEST_ASSERT_TRUE(ControlRuta::interlockFinPulsado(1.0f, 2.5f, 1.0f));
  TEST_ASSERT_TRUE(ControlRuta::interlockFinPulsado(0.0f, 2.5f, 1.0f));
  TEST_ASSERT_TRUE(ControlRuta::interlockFinPulsado(0.8f, 0.5f, 2.0f));
  TEST_ASSERT_FALSE(ControlRuta::interlockFinPulsado(3.5f, 2.5f, 1.0f));

  float memoria = 1.0f;
  memoria = ControlRuta::actualizarMemoriaImpulso(memoria, 2.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.3f, memoria);

  memoria = ControlRuta::actualizarMemoriaImpulso(memoria, 0.05f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.3f, memoria);

  memoria = ControlRuta::actualizarMemoriaImpulso(memoria, 8.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.3f, memoria);
}

void test_recentrado_ignora_desviaciones_pequenas_o_pasos_cortos() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f,
                           ControlRuta::umbralLateralSegmento(
                               10.0f, RECENTER_LATERAL_RATIO,
                               RECENTER_LATERAL_MIN_CM));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f,
                           ControlRuta::umbralLateralSegmento(
                               100.0f, RECENTER_LATERAL_RATIO,
                               RECENTER_LATERAL_MIN_CM));
  TEST_ASSERT_TRUE(ControlRuta::lateralEnRango(
      2.0f, 10.0f, RECENTER_LATERAL_RATIO, RECENTER_LATERAL_MIN_CM));
  TEST_ASSERT_FALSE(ControlRuta::lateralEnRango(
      2.1f, 10.0f, RECENTER_LATERAL_RATIO, RECENTER_LATERAL_MIN_CM));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.10f,
                           ControlRuta::proporcionLateralSegmento(6.0f, 60.0f));

  TEST_ASSERT_TRUE(ControlRuta::debeRecentrar(
      12.1f, 300, RECENTER_TRIGGER_CM, RECENTER_TRIGGER_MS,
      0.0f, 0, RECENTER_GROWTH_TRIGGER_CM, RECENTER_GROWTH_MS));

  TEST_ASSERT_TRUE(ControlRuta::debeRecentrar(
      5.0f, 0, RECENTER_TRIGGER_CM, RECENTER_TRIGGER_MS,
      6.1f, 500, RECENTER_GROWTH_TRIGGER_CM, RECENTER_GROWTH_MS));
  TEST_ASSERT_TRUE(ControlRuta::cruzoEjeConErrorBajo(1.0f, -1.5f, 2.0f));
  TEST_ASSERT_FALSE(ControlRuta::cruzoEjeConErrorBajo(1.0f, -2.1f, 2.0f));
  TEST_ASSERT_FALSE(ControlRuta::cruzoEjeConErrorBajo(1.0f, 1.5f, 2.0f));
}

void test_candidato_reingreso_elige_el_menor_coste_y_respeta_reversa() {
  const ControlRuta::PuntoReingreso punto = {0.0f, 52.0f, 10.0f};
  const auto avance = ControlRuta::evaluarCandidatoReingreso(
      6.0f, 40.0f, punto, 0.0f, 100.0f, 0.0f, 1, 1);
  const auto reversa = ControlRuta::evaluarCandidatoReingreso(
      6.0f, 40.0f, punto, 0.0f, 100.0f, 180.0f, -1, 1);
  TEST_ASSERT_TRUE(avance.valido);
  TEST_ASSERT_TRUE(reversa.valido);
  TEST_ASSERT_TRUE(avance.coste < reversa.coste);
  TEST_ASSERT_EQUAL_INT(1,
                        ControlRuta::elegirCandidatoReingreso(avance, reversa).direccion);

  const auto reversaPreferida = ControlRuta::evaluarCandidatoReingreso(
      6.0f, 40.0f, punto, 0.0f, 100.0f, 180.0f, -1, -1);
  const auto avanceLejano = ControlRuta::evaluarCandidatoReingreso(
      6.0f, 40.0f, punto, 0.0f, 100.0f, 180.0f, 1, -1);
  TEST_ASSERT_EQUAL_INT(-1,
                        ControlRuta::elegirCandidatoReingreso(
                            avanceLejano, reversaPreferida).direccion);
  TEST_ASSERT_TRUE(ControlRuta::costeReingreso(
      10.0f, 10.0f, 0.0f, 0.0f, true) >
                   ControlRuta::costeReingreso(
                       10.0f, 10.0f, 0.0f, 0.0f, false));
}

void test_coste_reingreso_usa_rumbo_planificado_para_retorno() {
  const ControlRuta::PuntoReingreso punto = {0.0f, 52.0f, 10.0f};
  const auto candidatoReversa = ControlRuta::evaluarCandidatoReingreso(
      0.0f, 40.0f, punto, 0.0f, 100.0f, 180.0f, -1, 1);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 180.0f,
                           fabsf(candidatoReversa.giroRetornoDeg));
  const auto candidatoMismaDireccion = ControlRuta::evaluarCandidatoReingreso(
      0.0f, 40.0f, punto, 0.0f, 100.0f, 180.0f, -1, -1);
  TEST_ASSERT_TRUE(candidatoReversa.coste > candidatoMismaDireccion.coste);
}

void test_endpoint_fino_permite_recuperacion_pulsada_y_evita_zona_muerta() {
  // Caso de la sesion real ADB #35: pose final a 5.18 cm del target (fuera de tolerancia 5.0 cm).
  // La distancia minima anterior (7.5 cm) causaba fallo prematuro; la nueva (2.5 cm)
  // autoriza la recuperacion mediante modo pulsado.
  const float errorResidualAdb35Cm = 5.18f;
  TEST_ASSERT_FALSE(ControlRuta::endpointAceptable(
      3.96f, errorResidualAdb35Cm, 0.0f, TOLERANCIA_ENDPOINT_CM, TOLERANCIA_GIRO_DEG));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlRuta::DecisionEndpoint::RECUPERAR),
      static_cast<uint8_t>(ControlRuta::decidirEndpointSeguro(
          true, false, 0, INTENTOS_RECUPERACION_ENDPOINT_MAX,
          errorResidualAdb35Cm, DISTANCIA_MINIMA_RECUPERACION_ENDPOINT_CM)));
  // Residual por debajo de la tolerancia de pulsos no es recuperable de forma segura
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlRuta::DecisionEndpoint::FALLAR),
      static_cast<uint8_t>(ControlRuta::decidirEndpointSeguro(
          true, false, 0, INTENTOS_RECUPERACION_ENDPOINT_MAX,
          1.8f, DISTANCIA_MINIMA_RECUPERACION_ENDPOINT_CM)));
  // El tramo fino de 5.18 cm se deriva deterministamente al modo de micropulsos
  TEST_ASSERT_TRUE(ControlRuta::tramoRequiereModoPulsado(
      errorResidualAdb35Cm, PULSE_DRIVE_THRESHOLD_CM));
}

void test_presupuesto_tiempo_recentrado_cubre_21_intentos() {
  // 21 intentos sostenidos x 3000 ms por intento = 63000 ms
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(63000, RECENTER_TIMEOUT_MS);
  ControlRuta::EpisodioRecuperacion episodio{};
  ControlRuta::abrirEpisodioRecuperacion(episodio, 1000);
  // Al cumplirse 21 intentos con 3s cada uno (t = 1000 + 62999 = 63999), debe seguir ACTIVO
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlRuta::EstadoEpisodioRecuperacion::ACTIVO),
      static_cast<uint8_t>(ControlRuta::procesarEpisodioRecuperacion(
          episodio, 63999, RECENTER_TIMEOUT_MS, false)));
  // Al expirar los 63000 ms completos, pasa a VENCIDO
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ControlRuta::EstadoEpisodioRecuperacion::VENCIDO),
      static_cast<uint8_t>(ControlRuta::procesarEpisodioRecuperacion(
          episodio, 64000, RECENTER_TIMEOUT_MS, false)));
}

void test_odometria_calibrada_marcas_suelo_sesion_41() {
  // Con el factor previo de 0.805 (-0.195), 50 cm requerían 120 ticks y sobrepasaban la marca física.
  // Con el factor afinado de 0.920 (-0.080), 50 cm corresponden a ~105 ticks y 100 cm a ~210 ticks.
  const float cmTickNominal = ControlRuta::distanciaPorTick(WHEEL_DIAMETER_CM, ENCODER_PPR);
  const float cmTickCalibrado = ControlRuta::distanciaPorTick(WHEEL_DIAMETER_ODOMETRY_CM, ENCODER_PPR);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.51836f, cmTickNominal);
  TEST_ASSERT_FLOAT_WITHIN(0.005f, 0.4769f, cmTickCalibrado);
  // La distancia calculada para 210 ticks debe ser ~100.1 cm
  const float distPara210Ticks = 210.0f * cmTickCalibrado;
  TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.15f, distPara210Ticks);
}

void test_reduccion_dinamica_asimetria_proporcional_y_acotada_30_pct() {
  // Error 0 deg -> 0 PWM de reducción
  TEST_ASSERT_EQUAL_INT(0, ControlRuta::calcularReduccionAsimetriaPwm(
      970, 0.0f, KP_ASYMMETRY_PWM_PER_DEG, ASYMMETRY_PWM_REDUCTION_MAX_RATIO));
  // Error 0.5 deg -> 7.5% de 970 = 72.75 -> 73 PWM
  TEST_ASSERT_EQUAL_INT(73, ControlRuta::calcularReduccionAsimetriaPwm(
      970, 0.5f, KP_ASYMMETRY_PWM_PER_DEG, ASYMMETRY_PWM_REDUCTION_MAX_RATIO));
  // Error 1.0 deg -> 15.0% de 970 = 145.5 -> 146 PWM
  TEST_ASSERT_EQUAL_INT(146, ControlRuta::calcularReduccionAsimetriaPwm(
      970, 1.0f, KP_ASYMMETRY_PWM_PER_DEG, ASYMMETRY_PWM_REDUCTION_MAX_RATIO));
  // Error 2.0 deg -> 30.0% de 970 = 291 PWM (alcanza el tope)
  TEST_ASSERT_EQUAL_INT(291, ControlRuta::calcularReduccionAsimetriaPwm(
      970, 2.0f, KP_ASYMMETRY_PWM_PER_DEG, ASYMMETRY_PWM_REDUCTION_MAX_RATIO));
  // Error 5.0 deg -> saturado al 30.0% de 970 = 291 PWM (nunca supera el 30%)
  TEST_ASSERT_EQUAL_INT(291, ControlRuta::calcularReduccionAsimetriaPwm(
      970, 5.0f, KP_ASYMMETRY_PWM_PER_DEG, ASYMMETRY_PWM_REDUCTION_MAX_RATIO));
  // Error negativo (-1.0 deg) -> magnitud proporcional idéntica
  TEST_ASSERT_EQUAL_INT(146, ControlRuta::calcularReduccionAsimetriaPwm(
      970, -1.0f, KP_ASYMMETRY_PWM_PER_DEG, ASYMMETRY_PWM_REDUCTION_MAX_RATIO));
}

void test_autoridad_correccion_lateral_ampliada_a_8_grados() {
  // Con KP_LATERAL_RUMBO_DEG_POR_CM = 1.2 y límite 8.0:
  // Error 3.0 cm -> -3.6 deg
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -3.6f,
                           ControlRuta::correccionLateralRumboDeg(3.0f, KP_LATERAL_RUMBO_DEG_POR_CM, CORRECCION_LATERAL_RUMBO_MAX_DEG));
  // Error 6.0 cm -> -7.2 deg
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -7.2f,
                           ControlRuta::correccionLateralRumboDeg(6.0f, KP_LATERAL_RUMBO_DEG_POR_CM, CORRECCION_LATERAL_RUMBO_MAX_DEG));
  // Error 10.0 cm -> saturado a -8.0 deg
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -8.0f,
                           ControlRuta::correccionLateralRumboDeg(10.0f, KP_LATERAL_RUMBO_DEG_POR_CM, CORRECCION_LATERAL_RUMBO_MAX_DEG));
  // Error -10.0 cm -> saturado a +8.0 deg
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 8.0f,
                           ControlRuta::correccionLateralRumboDeg(-10.0f, KP_LATERAL_RUMBO_DEG_POR_CM, CORRECCION_LATERAL_RUMBO_MAX_DEG));
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_mapeo_fisico_individual_conserva_gpio_y_signo_logico);
  RUN_TEST(test_lateral_derecha_corrige_hacia_izquierda);
  RUN_TEST(test_lateral_izquierda_corrige_hacia_derecha);
  RUN_TEST(test_errores_vectoriales_son_consistentes_en_rumbos_diagonales);
  RUN_TEST(test_reversa_automatica_conserva_el_chasis_ante_objetivo_detras);
  RUN_TEST(test_reversa_invierte_solo_la_correccion_lateral_del_chasis);
  RUN_TEST(test_reversa_invierte_el_lado_frenado_por_el_pid_de_rumbo);
  RUN_TEST(test_rumbo_final_en_reversa_conserva_el_cuerpo_sin_pivote_parasito);
  RUN_TEST(test_marco_cardinal_es_x_derecha_y_frente_y_yaw_horario);
  RUN_TEST(test_escala_y_freno_se_calculan_en_la_misma_unidad);
  RUN_TEST(test_integral_se_acota_y_no_crece_en_saturacion);
  RUN_TEST(test_integral_acumula_y_se_limita_fuera_de_saturacion);
  RUN_TEST(test_distancia_sola_no_acepta_endpoint);
  RUN_TEST(test_fallo_endpoint_despues_de_dos_intentos);
  RUN_TEST(test_presupuesto_unico_permite_21_reingresos_y_bloquea_el_22);
  RUN_TEST(test_endpoint_corto_falla_y_el_largo_se_recupera);
  RUN_TEST(test_reingreso_se_proyecta_sobre_ejes_ortogonales_y_se_limita_al_endpoint);
  RUN_TEST(test_disparadores_recentrado_y_sentido_incorrecto_exigen_persistencia);
  RUN_TEST(test_guard_maniobra_acepta_reversa_convergente_y_frena_divergencia);
  RUN_TEST(test_realineacion_conserva_progreso_en_baselines_repetidos);
  RUN_TEST(test_episodio_recuperacion_no_renueva_reintentos_y_cubre_rollover);
  RUN_TEST(test_reingreso_exige_centro_y_mejora_real);
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
  RUN_TEST(test_guard_pivote_exige_signo_mpu_y_frena_fallos);
  RUN_TEST(test_guard_pivote_detecta_desbalance_persistente);
  RUN_TEST(test_guard_no_frena_por_desbalance_mientras_aun_busca_torque);
  RUN_TEST(test_guard_relativo_ignora_ticks_anteriores_al_baseline);
  RUN_TEST(test_guard_calibracion_conserva_once_intentos_independientes);
  RUN_TEST(test_supervision_vencida_detiene_cualquier_movimiento);
  RUN_TEST(test_control_manual_mezcla_satura_y_respeta_lease);
  RUN_TEST(test_modo_pulsado_umbral_y_decision);
  RUN_TEST(test_calculo_pulso_traccion_y_correccion_rumbo);
  RUN_TEST(test_interlock_fin_pulsado_y_memoria_impulso);
  RUN_TEST(test_recentrado_ignora_desviaciones_pequenas_o_pasos_cortos);
  RUN_TEST(test_candidato_reingreso_elige_el_menor_coste_y_respeta_reversa);
  RUN_TEST(test_coste_reingreso_usa_rumbo_planificado_para_retorno);
  RUN_TEST(test_endpoint_fino_permite_recuperacion_pulsada_y_evita_zona_muerta);
  RUN_TEST(test_presupuesto_tiempo_recentrado_cubre_21_intentos);
  RUN_TEST(test_odometria_calibrada_marcas_suelo_sesion_41);
  RUN_TEST(test_reduccion_dinamica_asimetria_proporcional_y_acotada_30_pct);
  RUN_TEST(test_autoridad_correccion_lateral_ampliada_a_8_grados);
  return UNITY_END();
}
