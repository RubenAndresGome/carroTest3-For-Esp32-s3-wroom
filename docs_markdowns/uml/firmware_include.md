# UML funcional: `firmware/include`

Funciones detectadas: **55**. Tipos detectados: **10**.

## Grafo de llamadas

```mermaid
flowchart LR
  subgraph f0["Comandos.h"]
    n52fd560f83["tipo()"]
    ndfcaf3f3be["seq()"]
    n2723cbcf48["heading()"]
    ne704d1ed5d["distanciaCm()"]
    n2135af2d5f["factor()"]
  end
  subgraph f1["Eventos.h"]
    nae6550dc08["tipo()"]
    na459c2b651["seq()"]
    n8ee7331898["detalle()"]
    n7c591dc3c6["progreso()"]
  end
  subgraph f2["Mision.h"]
    nb9914d7cbb["x_cm()"]
    n95dbe740db["y_cm()"]
    nfd79d6b1ab["step_id()"]
  end
  subgraph f3["PoseEstimator.h"]
    n3bb4d1fc5e["getX() const"]
    n9130f7fc90["getY() const"]
    n1834fa7be3["getThetaRad() const"]
    n07aa956131["getThetaDeg() const"]
    n82c0e0074d["getArcoCentroGiroCm() const"]
    n2fc3d55854["getTraslacionGiroXCm() const"]
    nbe71efe7e7["getTraslacionGiroYCm() const"]
    nbedd71425a["x_global()"]
    nd7a4b30061["y_global()"]
    naaf8b63ce1["theta_rad()"]
    nb5a3b6f3c6["last_pulsos_FL()"]
    nb9bf88a428["last_pulsos_FR()"]
    n74d860e5dc["last_pulsos_BL()"]
    n6efa12ee47["last_pulsos_BR()"]
    nca786383e3["cm_por_pulso()"]
    nb936c76155["ultimo_signo_l()"]
    nb3c380a9a3["ultimo_signo_r()"]
    n1188c6a6bf["arco_centro_giro_cm()"]
    nf4fed91bea["traslacion_giro_x_cm()"]
    n6bc5cba68b["traslacion_giro_y_cm()"]
    nedb05230fb["DISTANCIA_EJES_CM()"]
  end
  subgraph f4["Seguridad.h"]
    nb8a33f8356["inicio_movimiento_ms()"]
    nfcd2379a57["pulsos_movimiento_iniciales()"]
  end
  subgraph f5["Sensores.h"]
    na1bb2d9854["pulsosFL()"]
    nd1a857052e["pulsosFR()"]
    n9d7f448fe0["pulsosBL()"]
    n74e4f2a87a["pulsosBR()"]
    n5b7748b089["delta_pulsos_filtrado_FL()"]
    n59e87ae3c9["delta_pulsos_filtrado_FR()"]
    n71156c1ab3["delta_pulsos_filtrado_BL()"]
    n0dc73f2365["delta_pulsos_filtrado_BR()"]
    n4620569983["velocidad_filtrada_L_cm_s()"]
    n0a82169272["velocidad_filtrada_R_cm_s()"]
    ndfa959ae0b["imu_deltaZ_rad()"]
    n887ffd1cbd["gyro_z_filtrado_rad_s()"]
    n28dd55e800["gyro_z_offset_rad_s()"]
    n6a9588871e["yaw_integrado_deg()"]
    n92b92815e1["timestamp_ms()"]
    n53765c5a44["timestamp_us()"]
    n6d2232461b["mpu_present()"]
    n588b6ba92e["mpu_stale()"]
    n7c7ebe4c92["mpu_calibrated()"]
    n19143883c9["sequence()"]
  end
  classDef alto fill:#5b1f2a,stroke:#ff7a7a,color:#fff
  classDef medio fill:#4a3717,stroke:#ffca67,color:#fff
  classDef bajo fill:#123b3a,stroke:#39e6aa,color:#fff
  class n52fd560f83 bajo
  class ndfcaf3f3be bajo
  class n2723cbcf48 bajo
  class ne704d1ed5d bajo
  class n2135af2d5f bajo
  class nae6550dc08 bajo
  class na459c2b651 bajo
  class n8ee7331898 bajo
  class n7c591dc3c6 bajo
  class nb9914d7cbb bajo
  class n95dbe740db bajo
  class nfd79d6b1ab bajo
  class n3bb4d1fc5e bajo
  class n9130f7fc90 bajo
  class n1834fa7be3 bajo
  class n07aa956131 bajo
  class n82c0e0074d bajo
  class n2fc3d55854 bajo
  class nbe71efe7e7 bajo
  class nbedd71425a bajo
  class nd7a4b30061 bajo
  class naaf8b63ce1 bajo
  class nb5a3b6f3c6 bajo
  class nb9bf88a428 bajo
  class n74d860e5dc bajo
  class n6efa12ee47 bajo
  class nca786383e3 bajo
  class nb936c76155 bajo
  class nb3c380a9a3 bajo
  class n1188c6a6bf bajo
  class nf4fed91bea bajo
  class n6bc5cba68b bajo
  class nedb05230fb bajo
  class nb8a33f8356 bajo
  class nfcd2379a57 bajo
  class na1bb2d9854 bajo
  class nd1a857052e bajo
  class n9d7f448fe0 bajo
  class n74e4f2a87a bajo
  class n5b7748b089 bajo
  class n59e87ae3c9 bajo
  class n71156c1ab3 bajo
  class n0dc73f2365 bajo
  class n4620569983 bajo
  class n0a82169272 bajo
  class ndfa959ae0b bajo
  class n887ffd1cbd bajo
  class n28dd55e800 bajo
  class n6a9588871e bajo
  class n92b92815e1 bajo
  class n53765c5a44 bajo
  class n6d2232461b bajo
  class n588b6ba92e bajo
  class n7c7ebe4c92 bajo
  class n19143883c9 bajo
```

Fuentes: [Mermaid](mermaid/firmware_include.mmd) · [PlantUML](plantuml/firmware_include.puml). Las flechas continuas son llamadas síncronas; las discontinuas representan asincronía, eventos o colas. El color del nodo indica riesgo estático.

## Inventario

| Función | Archivo | CC | Propietario | Riesgo/estado | Entra desde | Sale hacia | Estado compartido |
|---|---|---:|---|---|---|---|---|
| `ComandoRed.tipo` | [`include/Comandos.h`](../../include/Comandos.h#L21) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.seq` | [`include/Comandos.h`](../../include/Comandos.h#L22) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.heading` | [`include/Comandos.h`](../../include/Comandos.h#L23) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.distanciaCm` | [`include/Comandos.h`](../../include/Comandos.h#L24) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.factor` | [`include/Comandos.h`](../../include/Comandos.h#L25) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `EventoRed.tipo` | [`include/Eventos.h`](../../include/Eventos.h#L14) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `EventoRed.seq` | [`include/Eventos.h`](../../include/Eventos.h#L15) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `EventoRed.detalle` | [`include/Eventos.h`](../../include/Eventos.h#L16) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `EventoRed.progreso` | [`include/Eventos.h`](../../include/Eventos.h#L17) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PuntoMision.x_cm` | [`include/Mision.h`](../../include/Mision.h#L8) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PuntoMision.y_cm` | [`include/Mision.h`](../../include/Mision.h#L9) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PuntoMision.step_id` | [`include/Mision.h`](../../include/Mision.h#L10) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.getX` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L18) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `iniciarPaso` | — | — |
| `PoseEstimator.getY` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L19) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `iniciarPaso` | — | — |
| `PoseEstimator.getThetaRad` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L20) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.getThetaDeg` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L21) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.getArcoCentroGiroCm` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L25) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.getTraslacionGiroXCm` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L26) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.getTraslacionGiroYCm` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L27) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.x_global` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L30) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.y_global` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L31) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.theta_rad` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L32) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.last_pulsos_FL` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L34) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.last_pulsos_FR` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L35) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.last_pulsos_BL` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L36) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.last_pulsos_BR` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L37) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.cm_por_pulso` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L39) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.ultimo_signo_l` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L40) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.ultimo_signo_r` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L41) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.arco_centro_giro_cm` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L42) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.traslacion_giro_x_cm` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L43) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.traslacion_giro_y_cm` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L44) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.DISTANCIA_EJES_CM` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L46) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Seguridad.inicio_movimiento_ms` | [`include/Seguridad.h`](../../include/Seguridad.h#L14) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Seguridad.pulsos_movimiento_iniciales` | [`include/Seguridad.h`](../../include/Seguridad.h#L15) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.pulsosFL` | [`include/Sensores.h`](../../include/Sensores.h#L5) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.pulsosFR` | [`include/Sensores.h`](../../include/Sensores.h#L6) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.pulsosBL` | [`include/Sensores.h`](../../include/Sensores.h#L7) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.pulsosBR` | [`include/Sensores.h`](../../include/Sensores.h#L8) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.delta_pulsos_filtrado_FL` | [`include/Sensores.h`](../../include/Sensores.h#L9) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.delta_pulsos_filtrado_FR` | [`include/Sensores.h`](../../include/Sensores.h#L10) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.delta_pulsos_filtrado_BL` | [`include/Sensores.h`](../../include/Sensores.h#L11) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.delta_pulsos_filtrado_BR` | [`include/Sensores.h`](../../include/Sensores.h#L12) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.velocidad_filtrada_L_cm_s` | [`include/Sensores.h`](../../include/Sensores.h#L13) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.velocidad_filtrada_R_cm_s` | [`include/Sensores.h`](../../include/Sensores.h#L14) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.imu_deltaZ_rad` | [`include/Sensores.h`](../../include/Sensores.h#L15) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.gyro_z_filtrado_rad_s` | [`include/Sensores.h`](../../include/Sensores.h#L16) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.gyro_z_offset_rad_s` | [`include/Sensores.h`](../../include/Sensores.h#L17) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.yaw_integrado_deg` | [`include/Sensores.h`](../../include/Sensores.h#L18) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.timestamp_ms` | [`include/Sensores.h`](../../include/Sensores.h#L19) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.timestamp_us` | [`include/Sensores.h`](../../include/Sensores.h#L20) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.mpu_present` | [`include/Sensores.h`](../../include/Sensores.h#L21) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.mpu_stale` | [`include/Sensores.h`](../../include/Sensores.h#L22) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.mpu_calibrated` | [`include/Sensores.h`](../../include/Sensores.h#L23) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.sequence` | [`include/Sensores.h`](../../include/Sensores.h#L24) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
