#pragma once
#include <Arduino.h>

struct SensorSnapshot {
    int64_t pulsosFL;
    int64_t pulsosFR;
    int64_t pulsosBL;
    int64_t pulsosBR;
    float delta_pulsos_filtrado_FL;
    float delta_pulsos_filtrado_FR;
    float delta_pulsos_filtrado_BL;
    float delta_pulsos_filtrado_BR;
    float velocidad_filtrada_L_cm_s;
    float velocidad_filtrada_R_cm_s;
    float imu_deltaZ_rad;
    float gyro_z_filtrado_rad_s;
    float gyro_z_offset_rad_s;
    float yaw_integrado_deg;
    uint32_t timestamp_ms;
    uint64_t timestamp_us;
    bool mpu_present;
    bool mpu_stale;
    bool mpu_calibrated;
    uint32_t sequence;
};

void setup_Sensores();
SensorSnapshot leerSensoresSincrono();
const SensorSnapshot& snapshotSensoresControl();
bool obtenerUltimoSnapshotSensores(SensorSnapshot &destino);
void resetOrientacionIMU();
float obtenerYawIMUDeg();
bool recentrarYawIMUEnReposo();
uint32_t cantidadRecentradosYawIMU();
void resetFiltrosEncoder();

