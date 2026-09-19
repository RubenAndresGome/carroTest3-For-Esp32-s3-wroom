#include "PoseEstimator.h"
#include "Config.h"
#include "ControlRuta.h"
#include "ControlSeguridad.h"
#include "Motores.h"
#include <math.h>

PoseEstimator PoseGlobal;

PoseEstimator::PoseEstimator() {
    reset();
}

void PoseEstimator::inicializar(float rueda_diametro_cm, int encoder_ppr) {
    cm_por_pulso = (PI * rueda_diametro_cm) / (float)encoder_ppr;
}

void PoseEstimator::reset() {
    x_global = 0.0;
    y_global = 0.0;
    theta_rad = 0.0;
    last_pulsos_FL = 0;
    last_pulsos_FR = 0;
    last_pulsos_BL = 0;
    last_pulsos_BR = 0;
    ultimo_signo_l = ultimo_signo_r = 0;
    iniciarMedicionTraslacionGiro();
}

void PoseEstimator::actualizarOdometria(int64_t pulsosFL, int64_t pulsosFR, int64_t pulsosBL, int64_t pulsosBR, bool avanzando) {
    const int64_t deltas[4] = {
        pulsosFL - last_pulsos_FL, pulsosFR - last_pulsos_FR,
        pulsosBL - last_pulsos_BL, pulsosBR - last_pulsos_BR
    };

    last_pulsos_FL = pulsosFL;
    last_pulsos_FR = pulsosFR;
    last_pulsos_BL = pulsosBL;
    last_pulsos_BR = pulsosBR;

    const ControlSeguridad::PromediosLado promedios =
        ControlSeguridad::promediosConfiableAcotados(deltas, encoderConfiableGlobal,
                                                     DESACUERDO_MAXIMO_PAR);
    float distL = promedios.izquierdo * cm_por_pulso;
    float distR = promedios.derecho * cm_por_pulso;

    if (pwm_aplicado_L > 0) ultimo_signo_l = 1;
    else if (pwm_aplicado_L < 0) ultimo_signo_l = -1;
    if (pwm_aplicado_R > 0) ultimo_signo_r = 1;
    else if (pwm_aplicado_R < 0) ultimo_signo_r = -1;
    distL *= ultimo_signo_l;
    distR *= ultimo_signo_r;

    // La IMU es la única fuente de rumbo para no integrar dos veces el mismo giro.
    const bool navegando = avanzando;
    if (navegando) {
        const float distCentro = (distL + distR) / 2.0f;
        const float dx = distCentro * sin(theta_rad);
        const float dy = distCentro * cos(theta_rad);
        x_global += dx;
        y_global += dy;
        arco_centro_giro_cm += distCentro;
        traslacion_giro_x_cm += dx;
        traslacion_giro_y_cm += dy;
    }
}

void PoseEstimator::iniciarMedicionTraslacionGiro() {
    arco_centro_giro_cm = 0.0f;
    traslacion_giro_x_cm = 0.0f;
    traslacion_giro_y_cm = 0.0f;
}

void PoseEstimator::aplicarCorreccionICR(float deltaThetaRad, float xIcrCm, float yIcrCm) {
    if (deltaThetaRad == 0.0f) return;
    const ControlRuta::CorreccionICR correccion =
        ControlRuta::corregirTraslacionParasita(xIcrCm, yIcrCm, deltaThetaRad);
    x_global += correccion.dxCm;
    y_global += correccion.dyCm;
}

void PoseEstimator::actualizarOrientacion(float imu_delta_z) {
    theta_rad += imu_delta_z;

    while (theta_rad < 0) theta_rad += 2 * PI;
    while (theta_rad >= 2 * PI) theta_rad -= 2 * PI;
}

float PoseEstimator::distanciaAlObjetivo(float obj_x, float obj_y) {
    float dx = obj_x - x_global;
    float dy = obj_y - y_global;
    return sqrt(dx*dx + dy*dy);
}

float PoseEstimator::anguloAlObjetivoRad(float obj_x, float obj_y) {
    float dx = obj_x - x_global;
    float dy = obj_y - y_global;
    // Ángulo medido desde +Y, con signo positivo hacia +X.
    return atan2(dx, dy);
}
