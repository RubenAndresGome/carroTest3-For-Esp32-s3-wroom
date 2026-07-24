#include "PoseEstimator.h"
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

    float sumaL = 0.0f, sumaR = 0.0f;
    int cantidadL = 0, cantidadR = 0;
    if (encoderConfiableGlobal[0]) { sumaL += deltas[0]; ++cantidadL; }
    if (encoderConfiableGlobal[2]) { sumaL += deltas[2]; ++cantidadL; }
    if (encoderConfiableGlobal[1]) { sumaR += deltas[1]; ++cantidadR; }
    if (encoderConfiableGlobal[3]) { sumaR += deltas[3]; ++cantidadR; }
    float distL = cantidadL ? (sumaL / cantidadL) * cm_por_pulso : 0.0f;
    float distR = cantidadR ? (sumaR / cantidadR) * cm_por_pulso : 0.0f;

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
    }
}

void PoseEstimator::iniciarMedicionTraslacionGiro() {
    arco_centro_giro_cm = 0.0f;
    traslacion_giro_x_cm = 0.0f;
    traslacion_giro_y_cm = 0.0f;
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
