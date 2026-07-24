#pragma once
#include <Arduino.h>
#include "Estado.h"

class PoseEstimator {
public:
    PoseEstimator();

    void inicializar(float rueda_diametro_cm, int encoder_ppr);

    void reset();

    void actualizarOdometria(int64_t pulsosFL, int64_t pulsosFR, int64_t pulsosBL, int64_t pulsosBR, bool avanzando);

    void actualizarOrientacion(float imu_delta_z);
    void iniciarMedicionTraslacionGiro();

    float getX() const { return x_global; }
    float getY() const { return y_global; }
    float getThetaRad() const { return theta_rad; }
    float getThetaDeg() const { return theta_rad * 180.0 / PI; }

    float distanciaAlObjetivo(float obj_x, float obj_y);
    float anguloAlObjetivoRad(float obj_x, float obj_y);
    float getArcoCentroGiroCm() const { return arco_centro_giro_cm; }
    float getTraslacionGiroXCm() const { return traslacion_giro_x_cm; }
    float getTraslacionGiroYCm() const { return traslacion_giro_y_cm; }

private:
    float x_global;
    float y_global;
    float theta_rad;

    int64_t last_pulsos_FL;
    int64_t last_pulsos_FR;
    int64_t last_pulsos_BL;
    int64_t last_pulsos_BR;

    float cm_por_pulso;
    int ultimo_signo_l;
    int ultimo_signo_r;
    float arco_centro_giro_cm;
    float traslacion_giro_x_cm;
    float traslacion_giro_y_cm;

    static constexpr float DISTANCIA_EJES_CM = 18.0f;
};

extern PoseEstimator PoseGlobal;
