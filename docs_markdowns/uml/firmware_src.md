# UML funcional: `firmware/src`

Funciones detectadas: **111**. Tipos detectados: **0**.

## Grafo de llamadas

```mermaid
flowchart LR
  subgraph f0["Cinematica.cpp"]
    n70febdc353["errorAng360(float obj,float act)"]
    n09afdf9f24["aproximar(float v)"]
    n62bd8dce20["sensar()"]
    n777f625201["copiarBase(int64_t dest[4],const SensorSnapshot & s)"]
    n74bef9ec97["deltas(const int64_t base[4],const SensorSnapshot & s,int64_t out[4])"]
    n76f5d54686["fin(TipoEvento t,const char * d)"]
    nac048ed817["fallo(const char * d)"]
    n2f1f9eaf55["reiniciarDiagnosticoCalibracion()"]
    ne8aede947c["actualizarDiagnosticoCalibracion(const int64_t deltasEncoder[4],const ControlCalibracion::EvaluacionEncoders & evaluacion)"]
    nc947d74ff9["conservarEncodersAisladosDelDiagnostico()"]
    nd80b0fe18b["iniciarFaseCal(Fase f)"]
    n2e20655113["calCuenta()"]
    n7238aff005["calTorque(bool primera)"]
    n5225cb0b35["controlarCalibracion()"]
    ne467f8aa00["iniciarBaseGiro(float objetivoDeg,Fase retorno)"]
    ndffff1b1ec["reintentarGiro(const char * motivo)"]
    n741cd3a5da["controlarGiro()"]
    n05b906f80d["completarGiro()"]
    ne71d6a6518["actualizarErroresTrayectoria()"]
    n00a71cd6b5["objetivoAbsolutoAlcanzado()"]
    n0b13825ac2["mediana4(const int64_t v[4])"]
    nc6927f7221["hayPorLado()"]
    n1ba7c4b710["promedioLado(const int64_t v[4],bool izq)"]
    nd9d2240a27["estimarTicksAvance(const int64_t v[4])"]
    n25985afa09["resetConfEncoders()"]
    n0b8d751463["iniciarAvance(bool conservar)"]
    n3e4496e20f["detectarOutliers(const int64_t v[4])"]
    n52070d710d["iniciarPausaReeval(const int64_t v[4])"]
    n79480154c5["completarPausaReeval()"]
    n3d19627fa0["pwmAntiFriccion(uint8_t indice)"]
    n43daa92e89["iniciarAntiFriccion(const SensorSnapshot & s)"]
    n309fd76b79["controlarAntiFriccion(const SensorSnapshot & s)"]
    n95748f542a["controlarAvance()"]
    nf7b94c6431["iniciarAsentamientoFinal(float distanciaAntesDeFrenarCm)"]
    nf1e51a74ec["controlarAsentamientoFinal()"]
    nb037dedf6f["iniciarVerificacionFinal()"]
    n626fe6de17["iniciarRecuperacionEndpoint()"]
    na5c5809c63["verificarObjetivoFinal()"]
    n8fadfd55d3["completarPaso()"]
    n71cd8c8280["completarPasoConCorreccionPendiente()"]
    n7644830987["iniciarPasoInterno()"]
    n621f9aafb2["normalizar360(float a)"]
    nd5d7783e05["reiniciarControlRumbo()"]
    n7669db856b["registrarMotivoFinalizacion(const char * detalle)"]
    n23ef84c8cf["enFaseAvance()"]
    n7565d251cf["enFaseTraslacion()"]
    n292f220864["enFaseGiro()"]
    n167a7a6661["enFaseCalibracion()"]
    n05f6cc3058["obtenerDiagnosticoCalibracion()"]
    n82b029599f["iniciarCalibracion(int seq)"]
    n7dc80ae99d["iniciarPaso(float heading,float distanciaCm,int seq,float targetX,float targetY,bool objetivoAbsoluto,ModoPaso modoPaso)"]
    na64ddcaea5["iniciarGiroAbsoluto(float heading,int seq)"]
    n047d72d288["cancelarMovimiento(const char * detalle)"]
    n8c9f994f30["controlarMovimiento()"]
  end
  subgraph f1["DiagnosticoRTOS.cpp"]
    ned6049114b["textoReset(esp_reset_reason_t motivo)"]
    n999b6f2133["actualizarMinimo(volatile uint32_t & destino,UBaseType_t valor)"]
    n13d5d5c8e7["resultadosValidos(BaseType_t web,bool controlEnCore1)"]
    ne4a753795c["stackBajoValores(uint32_t web,uint32_t control)"]
    n73252a2cfd["inicializarDiagnosticoRTOS()"]
    n93202db9d6["registrarStackLibre(TareaDiagnosticada tarea,UBaseType_t bytesLibres)"]
    n7fe919940e["registrarResultadoArquitectura(BaseType_t web,bool controlEnCore1)"]
    ndf604692a1["registrarCicloControl(uint32_t periodoUs,uint32_t jitterUs,uint32_t duracionUs,uint32_t antiguedadMuestraUs,uint32_t deadlinesPerdidos)"]
    n1b9377afe9["stackRTOSBajo()"]
    ndf57242908["validarLogicaDiagnosticoRTOS()"]
  end
  subgraph f2["Estado.cpp"]
    n0a3d7af388["contenidoSinNul(const char * valor,size_t longitud)"]
    nf52367ee09["cadenaConfiguracionValida(const char (& valor)[N])"]
  end
  subgraph f3["Eventos.cpp"]
    na2bfb4ac57["encolarEvento(TipoEvento tipo,int seq,const char * detalle,float progreso,uint32_t runId)"]
  end
  subgraph f4["Motores.cpp"]
    n685fd62caf["validarMapaMotores()"]
    ned8aac33b6["aplicarVelocidades(int velIzq,int velDer)"]
    n36b60d19ed["frenarMotores()"]
    n8fe1874de3["validarInterlockMotores()"]
    n2c56769c27["estadoInterlockL()"]
    ne28254bd9f["estadoInterlockR()"]
    n83e71ee961["signoEnergizadoL()"]
    n18507f5e40["signoEnergizadoR()"]
    n06ad789fef["signoPendienteL()"]
    nbb47799838["signoPendienteR()"]
    n44a0c6a4d1["motoresListos()"]
    n98bfb063e4["estadoMotores()"]
    nde95c6d768["setup_MotorPinsLow()"]
    nb51dcc7337["setup_Motores()"]
  end
  subgraph f5["PoseEstimator.cpp"]
    n6d0812ef64["PoseEstimator()"]
    naf7943a248["inicializar(float rueda_diametro_cm,int encoder_ppr)"]
    n09b485aec5["reset()"]
    n40419d6fc8["actualizarOdometria(int64_t pulsosFL,int64_t pulsosFR,int64_t pulsosBL,int64_t pulsosBR,bool avanzando)"]
    nbc5b045960["iniciarMedicionTraslacionGiro()"]
    nc8a9f799ef["actualizarOrientacion(float imu_delta_z)"]
    nf524a72017["distanciaAlObjetivo(float obj_x,float obj_y)"]
    n84102f3e2b["anguloAlObjetivoRad(float obj_x,float obj_y)"]
  end
  subgraph f6["Red.cpp"]
    n3ed3ca1aaf["setup_Red()"]
    n632a6dd401["procesarWebSockets()"]
    nbe773d4ff6["pushTelemetria()"]
  end
  subgraph f7["Seguridad.cpp"]
    nf8836652dd["Seguridad()"]
    n0c00dc27c4["reiniciarSaludEncoders()"]
    n250627f063["actualizarSaludEncoders(const SensorSnapshot & snap,int pwm_L,int pwm_R)"]
    n0effb4bec6["auditarSalud(const SensorSnapshot & snap,int pwm_L,int pwm_R)"]
    nce07096a9e["forzarEStop()"]
    n9817e5e1ef["resetFallo()"]
  end
  subgraph f8["Sensores.cpp"]
    ne07f96b0e0["setup_Sensores()"]
    nbcba3216f0["resetFiltrosEncoder()"]
    n30e90c2e57["resetOrientacionIMU()"]
    nce50232280["obtenerYawIMUDeg()"]
    na50b46ec42["recentrarYawIMUEnReposo()"]
    nbc954010bb["cantidadRecentradosYawIMU()"]
    naf53943060["leerSensoresSincrono()"]
    n8a65804026["snapshotSensoresControl()"]
    n9008852f41["obtenerUltimoSnapshotSensores(SensorSnapshot & destino)"]
  end
  subgraph f9["main.cpp"]
    n0efbc0872a["procesarComandos()"]
    nd718f7fd2b["Task_Web(void *)"]
    n59a58b0a69["setup()"]
    nad5a7b9e7a["loop()"]
  end
  n00a71cd6b5 --> n70febdc353
  n00a71cd6b5 --> ne71d6a6518
  n047d72d288 -.-> n36b60d19ed
  n047d72d288 -.-> n7669db856b
  n047d72d288 -.-> na2bfb4ac57
  n047d72d288 -.-> nd5d7783e05
  n05b906f80d --> n09b485aec5
  n05b906f80d --> n0b8d751463
  n05b906f80d --> n30e90c2e57
  n05b906f80d --> n36b60d19ed
  n05b906f80d --> n76f5d54686
  n05b906f80d --> nb037dedf6f
  n05b906f80d --> nd80b0fe18b
  n09b485aec5 --> nbc5b045960
  n0b8d751463 --> n25985afa09
  n0b8d751463 --> n62bd8dce20
  n0b8d751463 --> n777f625201
  n0b8d751463 --> nd5d7783e05
  n0b8d751463 --> ne71d6a6518
  n0b8d751463 --> nf524a72017
  n0efbc0872a -.-> n047d72d288
  n0efbc0872a -.-> n09b485aec5
  n0efbc0872a -.-> n30e90c2e57
  n0efbc0872a -.-> n7dc80ae99d
  n0efbc0872a -.-> n82b029599f
  n0efbc0872a -.-> n9817e5e1ef
  n0efbc0872a -.-> na2bfb4ac57
  n0efbc0872a -.-> na64ddcaea5
  n0efbc0872a -.-> nce07096a9e
  n0effb4bec6 -.-> n167a7a6661
  n0effb4bec6 -.-> n23ef84c8cf
  n0effb4bec6 -.-> n292f220864
  n0effb4bec6 -.-> n36b60d19ed
  n0effb4bec6 -.-> n7669db856b
  n0effb4bec6 -.-> na2bfb4ac57
  n0effb4bec6 -.-> nd5d7783e05
  n1b9377afe9 --> ne4a753795c
  n250627f063 --> n23ef84c8cf
  n25985afa09 --> nbcba3216f0
  n2e20655113 --> n2f1f9eaf55
  n2e20655113 --> n621f9aafb2
  n2e20655113 --> n62bd8dce20
  n2e20655113 --> n777f625201
  n2e20655113 --> nac048ed817
  n2e20655113 --> nd80b0fe18b
  n309fd76b79 --> n1ba7c4b710
  n309fd76b79 --> n36b60d19ed
  n309fd76b79 --> n3d19627fa0
  n309fd76b79 --> n74bef9ec97
  n309fd76b79 --> n777f625201
  n309fd76b79 --> nac048ed817
  n309fd76b79 --> ned8aac33b6
  n3e4496e20f --> n0b13825ac2
  n3e4496e20f --> n70febdc353
  n43daa92e89 --> n36b60d19ed
  n43daa92e89 --> n3d19627fa0
  n43daa92e89 --> n777f625201
  n52070d710d --> n36b60d19ed
  n52070d710d --> nd5d7783e05
  n5225cb0b35 --> n2e20655113
  n5225cb0b35 --> n36b60d19ed
  n5225cb0b35 --> n621f9aafb2
  n5225cb0b35 --> n62bd8dce20
  n5225cb0b35 --> n7238aff005
  n5225cb0b35 --> n741cd3a5da
  n5225cb0b35 --> n777f625201
  n5225cb0b35 --> nac048ed817
  n5225cb0b35 --> nd80b0fe18b
  n5225cb0b35 --> ne467f8aa00
  n59a58b0a69 -.-> n36b60d19ed
  n59a58b0a69 -.-> n3ed3ca1aaf
  n59a58b0a69 -.-> n73252a2cfd
  n59a58b0a69 -.-> n7fe919940e
  n59a58b0a69 -.-> n98bfb063e4
  n59a58b0a69 -.-> naf7943a248
  n59a58b0a69 -.-> nb51dcc7337
  n59a58b0a69 -.-> nde95c6d768
  n59a58b0a69 -.-> ne07f96b0e0
  n626fe6de17 --> n0b8d751463
  n626fe6de17 --> n621f9aafb2
  n626fe6de17 --> n70febdc353
  n626fe6de17 --> n71cd8c8280
  n626fe6de17 --> n84102f3e2b
  n626fe6de17 --> nac048ed817
  n626fe6de17 --> ne467f8aa00
  n626fe6de17 --> ne71d6a6518
  n626fe6de17 --> nf524a72017
  n62bd8dce20 --> n9008852f41
  n6d0812ef64 --> n09b485aec5
  n71cd8c8280 --> n76f5d54686
  n7238aff005 --> n36b60d19ed
  n7238aff005 --> n621f9aafb2
  n7238aff005 --> n62bd8dce20
  n7238aff005 --> n74bef9ec97
  n7238aff005 --> n777f625201
  n7238aff005 --> nac048ed817
  n7238aff005 --> nc947d74ff9
  n7238aff005 --> nd80b0fe18b
  n7238aff005 --> ne467f8aa00
  n7238aff005 --> ne8aede947c
  n7238aff005 --> ned8aac33b6
  n73252a2cfd --> ned6049114b
  n741cd3a5da --> n05b906f80d
  n741cd3a5da --> n09afdf9f24
  n741cd3a5da --> n36b60d19ed
  n741cd3a5da --> n62bd8dce20
  n741cd3a5da --> n70febdc353
  n741cd3a5da --> n74bef9ec97
  n741cd3a5da --> n777f625201
  n741cd3a5da --> nac048ed817
  n741cd3a5da --> ndffff1b1ec
  n741cd3a5da --> ned8aac33b6
  n7644830987 --> n0b8d751463
  n7644830987 --> n70febdc353
  n7644830987 --> ne467f8aa00
  n76f5d54686 -.-> n36b60d19ed
  n76f5d54686 -.-> n7669db856b
  n76f5d54686 -.-> na2bfb4ac57
  n76f5d54686 -.-> nd5d7783e05
  n79480154c5 --> n0b8d751463
  n79480154c5 --> nac048ed817
  n7dc80ae99d -.-> n621f9aafb2
  n7dc80ae99d -.-> n7644830987
  n7dc80ae99d -.-> n7669db856b
  n7dc80ae99d -.-> na2bfb4ac57
  n7dc80ae99d -.-> ne71d6a6518
  n7fe919940e --> n13d5d5c8e7
  n82b029599f -.-> n2f1f9eaf55
  n82b029599f -.-> n62bd8dce20
  n82b029599f -.-> n7669db856b
  n82b029599f -.-> na2bfb4ac57
  n8c9f994f30 --> n5225cb0b35
  n8c9f994f30 --> n741cd3a5da
  n8c9f994f30 --> n95748f542a
  n8c9f994f30 --> na5c5809c63
  n8c9f994f30 --> nb037dedf6f
  n8c9f994f30 --> nf1e51a74ec
  n8fadfd55d3 --> n00a71cd6b5
  n8fadfd55d3 --> n626fe6de17
  n8fadfd55d3 --> n71cd8c8280
  n8fadfd55d3 --> n76f5d54686
  n8fadfd55d3 --> nac048ed817
  n8fadfd55d3 --> nf524a72017
  n93202db9d6 --> n999b6f2133
  n95748f542a --> n09afdf9f24
  n95748f542a --> n1ba7c4b710
  n95748f542a --> n309fd76b79
  n95748f542a --> n36b60d19ed
  n95748f542a --> n43daa92e89
  n95748f542a --> n621f9aafb2
  n95748f542a --> n62bd8dce20
  n95748f542a --> n70febdc353
  n95748f542a --> n74bef9ec97
  n95748f542a --> n79480154c5
  n95748f542a --> n84102f3e2b
  n95748f542a --> nac048ed817
  n95748f542a --> nc6927f7221
  n95748f542a --> nd9d2240a27
  n95748f542a --> ne467f8aa00
  n95748f542a --> ne71d6a6518
  n95748f542a --> ned8aac33b6
  n95748f542a --> nf524a72017
  n95748f542a --> nf7b94c6431
  n9817e5e1ef --> n0c00dc27c4
  n9817e5e1ef --> n36b60d19ed
  n9817e5e1ef --> n44a0c6a4d1
  n9817e5e1ef --> n7669db856b
  n9817e5e1ef --> nd5d7783e05
  na5c5809c63 --> n00a71cd6b5
  na5c5809c63 --> n36b60d19ed
  na5c5809c63 --> n626fe6de17
  na5c5809c63 --> n70febdc353
  na5c5809c63 --> n8fadfd55d3
  na5c5809c63 --> ne467f8aa00
  na64ddcaea5 -.-> n621f9aafb2
  na64ddcaea5 -.-> n7669db856b
  na64ddcaea5 -.-> na2bfb4ac57
  na64ddcaea5 -.-> ne467f8aa00
  nac048ed817 --> n76f5d54686
  nad5a7b9e7a --> n8a65804026
  nad5a7b9e7a --> n93202db9d6
  nad5a7b9e7a --> ndf604692a1
  nb037dedf6f --> n36b60d19ed
  nb51dcc7337 --> n36b60d19ed
  nb51dcc7337 --> n685fd62caf
  nce07096a9e --> n36b60d19ed
  nce07096a9e --> n7669db856b
  nce07096a9e --> nd5d7783e05
  nd718f7fd2b --> n632a6dd401
  nd718f7fd2b --> n93202db9d6
  nd718f7fd2b --> nbe773d4ff6
  nd9d2240a27 --> n1ba7c4b710
  nd9d2240a27 --> nc6927f7221
  nde95c6d768 --> n685fd62caf
  ndf57242908 --> n13d5d5c8e7
  ndf57242908 --> ne4a753795c
  ndf57242908 --> ned6049114b
  ndffff1b1ec --> n36b60d19ed
  ndffff1b1ec --> nac048ed817
  ne467f8aa00 --> n62bd8dce20
  ne467f8aa00 --> n777f625201
  ne467f8aa00 --> nd5d7783e05
  ned8aac33b6 --> n44a0c6a4d1
  ned8aac33b6 --> n685fd62caf
  nf1e51a74ec --> n62bd8dce20
  nf1e51a74ec --> n74bef9ec97
  nf1e51a74ec --> nac048ed817
  nf1e51a74ec --> nd9d2240a27
  nf1e51a74ec --> ne71d6a6518
  nf52367ee09 --> n0a3d7af388
  nf7b94c6431 --> n36b60d19ed
  nf7b94c6431 --> n62bd8dce20
  nf7b94c6431 --> n777f625201
  classDef alto fill:#5b1f2a,stroke:#ff7a7a,color:#fff
  classDef medio fill:#4a3717,stroke:#ffca67,color:#fff
  classDef bajo fill:#123b3a,stroke:#39e6aa,color:#fff
  class n70febdc353 bajo
  class n09afdf9f24 bajo
  class n62bd8dce20 bajo
  class n777f625201 bajo
  class n74bef9ec97 bajo
  class n76f5d54686 bajo
  class nac048ed817 bajo
  class n2f1f9eaf55 bajo
  class ne8aede947c bajo
  class nc947d74ff9 bajo
  class nd80b0fe18b bajo
  class n2e20655113 bajo
  class n7238aff005 alto
  class n5225cb0b35 bajo
  class ne467f8aa00 bajo
  class ndffff1b1ec bajo
  class n741cd3a5da alto
  class n05b906f80d bajo
  class ne71d6a6518 bajo
  class n00a71cd6b5 bajo
  class n0b13825ac2 bajo
  class nc6927f7221 bajo
  class n1ba7c4b710 bajo
  class nd9d2240a27 bajo
  class n25985afa09 bajo
  class n0b8d751463 bajo
  class n3e4496e20f bajo
  class n52070d710d bajo
  class n79480154c5 bajo
  class n3d19627fa0 bajo
  class n43daa92e89 bajo
  class n309fd76b79 alto
  class n95748f542a alto
  class nf7b94c6431 bajo
  class nf1e51a74ec bajo
  class nb037dedf6f bajo
  class n626fe6de17 bajo
  class na5c5809c63 bajo
  class n8fadfd55d3 bajo
  class n71cd8c8280 bajo
  class n7644830987 bajo
  class n621f9aafb2 bajo
  class nd5d7783e05 bajo
  class n7669db856b bajo
  class n23ef84c8cf bajo
  class n7565d251cf bajo
  class n292f220864 bajo
  class n167a7a6661 bajo
  class n05f6cc3058 bajo
  class n82b029599f bajo
  class n7dc80ae99d bajo
  class na64ddcaea5 bajo
  class n047d72d288 medio
  class n8c9f994f30 bajo
  class ned6049114b bajo
  class n999b6f2133 bajo
  class n13d5d5c8e7 bajo
  class ne4a753795c bajo
  class n73252a2cfd bajo
  class n93202db9d6 bajo
  class n7fe919940e bajo
  class ndf604692a1 bajo
  class n1b9377afe9 bajo
  class ndf57242908 bajo
  class n0a3d7af388 bajo
  class nf52367ee09 bajo
  class na2bfb4ac57 bajo
  class n685fd62caf bajo
  class ned8aac33b6 alto
  class n36b60d19ed bajo
  class n8fe1874de3 bajo
  class n2c56769c27 bajo
  class ne28254bd9f bajo
  class n83e71ee961 bajo
  class n18507f5e40 bajo
  class n06ad789fef bajo
  class nbb47799838 bajo
  class n44a0c6a4d1 bajo
  class n98bfb063e4 bajo
  class nde95c6d768 bajo
  class nb51dcc7337 alto
  class n6d0812ef64 bajo
  class naf7943a248 bajo
  class n09b485aec5 bajo
  class n40419d6fc8 bajo
  class nbc5b045960 bajo
  class nc8a9f799ef bajo
  class nf524a72017 bajo
  class n84102f3e2b bajo
  class n3ed3ca1aaf medio
  class n632a6dd401 bajo
  class nbe773d4ff6 bajo
  class nf8836652dd bajo
  class n0c00dc27c4 bajo
  class n250627f063 alto
  class n0effb4bec6 alto
  class nce07096a9e medio
  class n9817e5e1ef medio
  class ne07f96b0e0 bajo
  class nbcba3216f0 bajo
  class n30e90c2e57 bajo
  class nce50232280 bajo
  class na50b46ec42 bajo
  class nbc954010bb bajo
  class naf53943060 bajo
  class n8a65804026 bajo
  class n9008852f41 bajo
  class n0efbc0872a bajo
  class nd718f7fd2b bajo
  class n59a58b0a69 alto
  class nad5a7b9e7a bajo
```

Fuentes: [Mermaid](mermaid/firmware_src.mmd) · [PlantUML](plantuml/firmware_src.puml). Las flechas continuas son llamadas síncronas; las discontinuas representan asincronía, eventos o colas. El color del nodo indica riesgo estático.

## Inventario

| Función | Archivo | CC | Propietario | Riesgo/estado | Entra desde | Sale hacia | Estado compartido |
|---|---|---:|---|---|---|---|---|
| `__anon3dcaec680111.errorAng360` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L64) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance`, `controlarGiro`, `detectarOutliers`, `iniciarPasoInterno`, `iniciarRecuperacionEndpoint`, `objetivoAbsolutoAlcanzado`, `verificarObjetivoFinal` | — | — |
| `__anon3dcaec680111.aproximar` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L70) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance`, `controlarGiro` | — | — |
| `__anon3dcaec680111.sensar` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L73) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `calCuenta`, `calTorque`, `controlarAsentamientoFinal`, `controlarAvance`, `controlarCalibracion`, `controlarGiro`, `iniciarAsentamientoFinal`, `iniciarAvance`, `iniciarBaseGiro`, `iniciarCalibracion` | `obtenerUltimoSnapshotSensores` | — |
| `__anon3dcaec680111.copiarBase` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L78) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `calCuenta`, `calTorque`, `controlarAntiFriccion`, `controlarCalibracion`, `controlarGiro`, `iniciarAntiFriccion`, `iniciarAsentamientoFinal`, `iniciarAvance`, `iniciarBaseGiro` | — | — |
| `__anon3dcaec680111.deltas` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L81) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `calTorque`, `controlarAntiFriccion`, `controlarAsentamientoFinal`, `controlarAvance`, `controlarGiro` | — | — |
| `__anon3dcaec680111.fin` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L87) | 3 | ESP32 / tiempo real | Bajo; interno; evento | `completarGiro`, `completarPaso`, `completarPasoConCorreccionPendiente`, `fallo` | `encolarEvento`, `frenarMotores`, `registrarMotivoFinalizacion`, `reiniciarControlRumbo` | — |
| `__anon3dcaec680111.fallo` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L98) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `calCuenta`, `calTorque`, `completarPaso`, `completarPausaReeval`, `controlarAntiFriccion`, `controlarAsentamientoFinal`, `controlarAvance`, `controlarCalibracion`, `controlarGiro`, `iniciarRecuperacionEndpoint`, `reintentarGiro` | `fin` | — |
| `__anon3dcaec680111.reiniciarDiagnosticoCalibracion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L115) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `calCuenta`, `iniciarCalibracion` | `totalPasosRampa` | — |
| `__anon3dcaec680111.actualizarDiagnosticoCalibracion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L122) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `calTorque` | `pasoRampaActual` | — |
| `__anon3dcaec680111.conservarEncodersAisladosDelDiagnostico` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L144) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `calTorque` | — | — |
| `__anon3dcaec680111.iniciarFaseCal` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L157) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `calCuenta`, `calTorque`, `completarGiro`, `controlarCalibracion` | — | — |
| `__anon3dcaec680111.calCuenta` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L159) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarCalibracion` | `copiarBase`, `fallo`, `iniciarFaseCal`, `normalizar360`, `reiniciarDiagnosticoCalibracion`, `sensar` | — |
| `__anon3dcaec680111.calTorque` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L173) | 20 | ESP32 / tiempo real | Alto; interno; síncrona | `controlarCalibracion` | `actualizarDiagnosticoCalibracion`, `aplicarVelocidades`, `conservarEncodersAisladosDelDiagnostico`, `copiarBase`, `deltas`, `evaluarEncoders`, `fallo`, `frenarMotores`, `iniciarBaseGiro`, `iniciarFaseCal`, `normalizar360`, `sensar` | — |
| `__anon3dcaec680111.controlarCalibracion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L245) | 13 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarMovimiento` | `calCuenta`, `calTorque`, `controlarGiro`, `copiarBase`, `fallo`, `frenarMotores`, `iniciarBaseGiro`, `iniciarFaseCal`, `normalizar360`, `sensar` | — |
| `__anon3dcaec680111.iniciarBaseGiro` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L295) | 6 | ESP32 / tiempo real | Bajo; interno; síncrona | `calTorque`, `controlarAvance`, `controlarCalibracion`, `iniciarGiroAbsoluto`, `iniciarPasoInterno`, `iniciarRecuperacionEndpoint`, `verificarObjetivoFinal` | `copiarBase`, `reiniciarControlRumbo`, `sensar` | — |
| `__anon3dcaec680111.reintentarGiro` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L318) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarGiro` | `fallo`, `frenarMotores` | — |
| `__anon3dcaec680111.controlarGiro` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L327) | 77 | ESP32 / tiempo real | Alto; interno; síncrona | `controlarCalibracion`, `controlarMovimiento` | `aplicarVelocidades`, `aproximar`, `completarGiro`, `copiarBase`, `deltas`, `errorAng360`, `fallo`, `frenarMotores`, `reintentarGiro`, `sensar` | — |
| `__anon3dcaec680111.completarGiro` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L481) | 7 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarGiro` | `fin`, `frenarMotores`, `iniciarAvance`, `iniciarFaseCal`, `iniciarVerificacionFinal`, `reset`, `resetOrientacionIMU` | — |
| `__anon3dcaec680111.actualizarErroresTrayectoria` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L521) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAsentamientoFinal`, `controlarAvance`, `iniciarAvance`, `iniciarPaso`, `iniciarRecuperacionEndpoint`, `objetivoAbsolutoAlcanzado` | `calcularErroresTrayectoria`, `getX`, `getY` | — |
| `__anon3dcaec680111.objetivoAbsolutoAlcanzado` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L531) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `completarPaso`, `verificarObjetivoFinal` | `actualizarErroresTrayectoria`, `endpointAceptable`, `errorAng360` | — |
| `__anon3dcaec680111.mediana4` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L538) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `detectarOutliers` | `medianaCuatro` | — |
| `__anon3dcaec680111.hayPorLado` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L541) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance`, `estimarTicksAvance` | `fuentesPorLadoValidas` | — |
| `__anon3dcaec680111.promedioLado` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L542) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAntiFriccion`, `controlarAvance`, `estimarTicksAvance` | `promedioConfiableLado` | — |
| `__anon3dcaec680111.estimarTicksAvance` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L545) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAsentamientoFinal`, `controlarAvance` | `hayPorLado`, `promedioLado` | — |
| `__anon3dcaec680111.resetConfEncoders` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L551) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `iniciarAvance` | `resetFiltrosEncoder` | — |
| `__anon3dcaec680111.iniciarAvance` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L559) | 4 | ESP32 / tiempo real | Bajo; interno; síncrona | `completarGiro`, `completarPausaReeval`, `iniciarPasoInterno`, `iniciarRecuperacionEndpoint` | `actualizarErroresTrayectoria`, `copiarBase`, `distanciaAlObjetivo`, `reiniciarControlRumbo`, `resetConfEncoders`, `sensar` | — |
| `__anon3dcaec680111.detectarOutliers` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L595) | 12 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `encoderEsOutlier`, `errorAng360`, `mediana4` | — |
| `__anon3dcaec680111.iniciarPausaReeval` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L614) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `frenarMotores`, `reiniciarControlRumbo` | — |
| `__anon3dcaec680111.completarPausaReeval` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L623) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | `clasificarEncoders`, `fallo`, `iniciarAvance` | — |
| `__anon3dcaec680111.pwmAntiFriccion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L640) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAntiFriccion`, `iniciarAntiFriccion` | `nivelAntiFriccion8Bit` | — |
| `__anon3dcaec680111.iniciarAntiFriccion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L646) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | `copiarBase`, `frenarMotores`, `pwmAntiFriccion` | — |
| `__anon3dcaec680111.controlarAntiFriccion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L658) | 9 | ESP32 / tiempo real | Alto; interno; síncrona | `controlarAvance` | `aplicarVelocidades`, `copiarBase`, `deltas`, `fallo`, `frenarMotores`, `movimientoAntiFriccionConfirmado`, `promedioLado`, `pwmAntiFriccion` | — |
| `__anon3dcaec680111.controlarAvance` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L710) | 48 | ESP32 / tiempo real | Alto; interno; síncrona | `controlarMovimiento` | `actualizarErroresTrayectoria`, `actualizarPI`, `anguloAlObjetivoRad`, `aplicarVelocidades`, `aproximar`, `completarPausaReeval`, `controlarAntiFriccion`, `correccionLateralParaDireccion`, `correccionLateralRumboDeg`, `deltas`, `distanciaAlObjetivo`, `distanciaFrenoPrevista`, `distanciaPorTick`, `errorAng360`, `estimarTicksAvance`, `fallo`, `frenarLadoIzquierdoParaRumbo`, `frenarMotores`, `hayPorLado`, `iniciarAntiFriccion`, `iniciarAsentamientoFinal`, `iniciarBaseGiro`, `normalizar360`, `promedioLado`, `rumboCuerpoParaTrayecto`, `sensar` | — |
| `__anon3dcaec680111.iniciarAsentamientoFinal` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L910) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | `copiarBase`, `frenarMotores`, `sensar` | — |
| `__anon3dcaec680111.controlarAsentamientoFinal` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L924) | 7 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarMovimiento` | `actualizarErroresTrayectoria`, `deltas`, `distanciaPorTick`, `estimarTicksAvance`, `fallo`, `sensar` | — |
| `__anon3dcaec680111.iniciarVerificacionFinal` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L954) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `completarGiro`, `controlarMovimiento` | `frenarMotores` | — |
| `__anon3dcaec680111.iniciarRecuperacionEndpoint` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L961) | 9 | ESP32 / tiempo real | Bajo; interno; síncrona | `completarPaso`, `verificarObjetivoFinal` | `actualizarErroresTrayectoria`, `anguloAlObjetivoRad`, `completarPasoConCorreccionPendiente`, `decidirEndpointSeguro`, `distanciaAlObjetivo`, `errorAng360`, `fallo`, `iniciarAvance`, `iniciarBaseGiro`, `normalizar360`, `reversaAutomatica`, `rumboCuerpoParaTrayecto` | — |
| `__anon3dcaec680111.verificarObjetivoFinal` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1015) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarMovimiento` | `completarPaso`, `errorAng360`, `frenarMotores`, `iniciarBaseGiro`, `iniciarRecuperacionEndpoint`, `objetivoAbsolutoAlcanzado` | — |
| `__anon3dcaec680111.completarPaso` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1033) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `verificarObjetivoFinal` | `completarPasoConCorreccionPendiente`, `decidirEndpointSeguro`, `distanciaAlObjetivo`, `fallo`, `fin`, `iniciarRecuperacionEndpoint`, `objetivoAbsolutoAlcanzado` | — |
| `__anon3dcaec680111.completarPasoConCorreccionPendiente` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1059) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `completarPaso`, `iniciarRecuperacionEndpoint` | `fin` | — |
| `__anon3dcaec680111.iniciarPasoInterno` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1067) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `iniciarPaso` | `errorAng360`, `iniciarAvance`, `iniciarBaseGiro` | — |
| `normalizar360` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1079) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `calCuenta`, `calTorque`, `controlarAvance`, `controlarCalibracion`, `errorAngularDeg`, `iniciarGiroAbsoluto`, `iniciarPaso`, `iniciarRecuperacionEndpoint`, `rumboCuerpoParaTrayecto` | — | — |
| `reiniciarControlRumbo` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1084) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `auditarSalud`, `cancelarMovimiento`, `fin`, `forzarEStop`, `iniciarAvance`, `iniciarBaseGiro`, `iniciarPausaReeval`, `resetFallo` | — | — |
| `registrarMotivoFinalizacion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1095) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `auditarSalud`, `cancelarMovimiento`, `fin`, `forzarEStop`, `iniciarCalibracion`, `iniciarGiroAbsoluto`, `iniciarPaso`, `resetFallo` | — | — |
| `enFaseAvance` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1099) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `actualizarSaludEncoders`, `auditarSalud` | — | — |
| `enFaseTraslacion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1100) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `enFaseGiro` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1103) | 6 | ESP32 / tiempo real | Bajo; interno; síncrona | `auditarSalud` | — | — |
| `enFaseCalibracion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1108) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `auditarSalud` | — | — |
| `obtenerDiagnosticoCalibracion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1111) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `iniciarCalibracion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1117) | 6 | ESP32 / tiempo real | Bajo; interno; evento | `procesarComandos` | `encolarEvento`, `registrarMotivoFinalizacion`, `reiniciarDiagnosticoCalibracion`, `sensar` | — |
| `iniciarPaso` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1134) | 23 | ESP32 / tiempo real | Bajo; interno; evento | `procesarComandos` | `actualizarErroresTrayectoria`, `encolarEvento`, `getX`, `getY`, `iniciarPasoInterno`, `normalizar360`, `registrarMotivoFinalizacion`, `reversaAutomatica`, `rumboCuerpoParaTrayecto` | estado |
| `iniciarGiroAbsoluto` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1222) | 5 | ESP32 / tiempo real | Bajo; interno; evento | `procesarComandos` | `encolarEvento`, `iniciarBaseGiro`, `normalizar360`, `registrarMotivoFinalizacion` | estado |
| `cancelarMovimiento` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1251) | 6 | ESP32 / tiempo real | Medio; interno; evento | `procesarComandos` | `encolarEvento`, `frenarMotores`, `registrarMotivoFinalizacion`, `reiniciarControlRumbo`, `stopDebePreservarFallo` | parada/cierre |
| `controlarMovimiento` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1272) | 20 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `controlarAsentamientoFinal`, `controlarAvance`, `controlarCalibracion`, `controlarGiro`, `iniciarVerificacionFinal`, `verificarObjetivoFinal` | — |
| `__anon3946a1160111.textoReset` | [`src/DiagnosticoRTOS.cpp`](../../src/DiagnosticoRTOS.cpp#L19) | 11 | ESP32 / tiempo real | Bajo; interno; síncrona | `inicializarDiagnosticoRTOS`, `validarLogicaDiagnosticoRTOS` | — | — |
| `__anon3946a1160111.actualizarMinimo` | [`src/DiagnosticoRTOS.cpp`](../../src/DiagnosticoRTOS.cpp#L35) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `registrarStackLibre` | — | — |
| `__anon3946a1160111.resultadosValidos` | [`src/DiagnosticoRTOS.cpp`](../../src/DiagnosticoRTOS.cpp#L40) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `registrarResultadoArquitectura`, `validarLogicaDiagnosticoRTOS` | — | — |
| `__anon3946a1160111.stackBajoValores` | [`src/DiagnosticoRTOS.cpp`](../../src/DiagnosticoRTOS.cpp#L44) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `stackRTOSBajo`, `validarLogicaDiagnosticoRTOS` | — | — |
| `inicializarDiagnosticoRTOS` | [`src/DiagnosticoRTOS.cpp`](../../src/DiagnosticoRTOS.cpp#L52) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `setup` | `textoReset` | — |
| `registrarStackLibre` | [`src/DiagnosticoRTOS.cpp`](../../src/DiagnosticoRTOS.cpp#L57) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `Task_Web`, `loop` | `actualizarMinimo` | — |
| `registrarResultadoArquitectura` | [`src/DiagnosticoRTOS.cpp`](../../src/DiagnosticoRTOS.cpp#L64) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `setup` | `resultadosValidos` | — |
| `registrarCicloControl` | [`src/DiagnosticoRTOS.cpp`](../../src/DiagnosticoRTOS.cpp#L72) | 4 | ESP32 / tiempo real | Bajo; interno; síncrona | `loop` | — | — |
| `stackRTOSBajo` | [`src/DiagnosticoRTOS.cpp`](../../src/DiagnosticoRTOS.cpp#L82) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `stackBajoValores` | — |
| `validarLogicaDiagnosticoRTOS` | [`src/DiagnosticoRTOS.cpp`](../../src/DiagnosticoRTOS.cpp#L86) | 11 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `resultadosValidos`, `stackBajoValores`, `textoReset` | — |
| `contenidoSinNul` | [`src/Estado.cpp`](../../src/Estado.cpp#L12) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `cadenaConfiguracionValida` | — | — |
| `cadenaConfiguracionValida` | [`src/Estado.cpp`](../../src/Estado.cpp#L18) | 3 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `contenidoSinNul` | — |
| `encolarEvento` | [`src/Eventos.cpp`](../../src/Eventos.cpp#L6) | 13 | ESP32 / tiempo real | Bajo; interno; cola/evento | `auditarSalud`, `cancelarMovimiento`, `fin`, `iniciarCalibracion`, `iniciarGiroAbsoluto`, `iniciarPaso`, `procesarComandos` | — | — |
| `validarMapaMotores` | [`src/Motores.cpp`](../../src/Motores.cpp#L107) | 9 | ESP32 / tiempo real | Bajo; interno; síncrona | `aplicarVelocidades`, `setup_MotorPinsLow`, `setup_Motores` | — | — |
| `aplicarVelocidades` | [`src/Motores.cpp`](../../src/Motores.cpp#L152) | 14 | ESP32 / tiempo real | Alto; interno; síncrona | `calTorque`, `controlarAntiFriccion`, `controlarAvance`, `controlarGiro` | `motoresListos`, `validarMapaMotores` | — |
| `frenarMotores` | [`src/Motores.cpp`](../../src/Motores.cpp#L197) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `auditarSalud`, `calTorque`, `cancelarMovimiento`, `completarGiro`, `controlarAntiFriccion`, `controlarAvance`, `controlarCalibracion`, `controlarGiro`, `fin`, `forzarEStop`, `iniciarAntiFriccion`, `iniciarAsentamientoFinal`, `iniciarPausaReeval`, `iniciarVerificacionFinal`, `reintentarGiro`, `resetFallo`, `setup`, `setup_Motores`, `verificarObjetivoFinal` | — | — |
| `validarInterlockMotores` | [`src/Motores.cpp`](../../src/Motores.cpp#L208) | 24 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | estado |
| `estadoInterlockL` | [`src/Motores.cpp`](../../src/Motores.cpp#L254) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | estado |
| `estadoInterlockR` | [`src/Motores.cpp`](../../src/Motores.cpp#L255) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | estado |
| `signoEnergizadoL` | [`src/Motores.cpp`](../../src/Motores.cpp#L256) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `signoEnergizadoR` | [`src/Motores.cpp`](../../src/Motores.cpp#L257) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `signoPendienteL` | [`src/Motores.cpp`](../../src/Motores.cpp#L258) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `signoPendienteR` | [`src/Motores.cpp`](../../src/Motores.cpp#L259) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `motoresListos` | [`src/Motores.cpp`](../../src/Motores.cpp#L261) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `aplicarVelocidades`, `resetFallo` | — | — |
| `estadoMotores` | [`src/Motores.cpp`](../../src/Motores.cpp#L263) | 4 | ESP32 / tiempo real | Bajo; interno; síncrona | `setup` | — | — |
| `setup_MotorPinsLow` | [`src/Motores.cpp`](../../src/Motores.cpp#L272) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `setup` | `validarMapaMotores` | — |
| `setup_Motores` | [`src/Motores.cpp`](../../src/Motores.cpp#L284) | 4 | ESP32 / tiempo real | Alto; interno; síncrona | `setup` | `frenarMotores`, `validarMapaMotores` | — |
| `PoseEstimator.PoseEstimator` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L7) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `reset` | — |
| `PoseEstimator.inicializar` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L11) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `setup` | — | — |
| `PoseEstimator.reset` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L15) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `PoseEstimator`, `completarGiro`, `procesarComandos` | `iniciarMedicionTraslacionGiro` | — |
| `PoseEstimator.actualizarOdometria` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L27) | 12 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.iniciarMedicionTraslacionGiro` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L65) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `reset` | — | — |
| `PoseEstimator.actualizarOrientacion` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L71) | 3 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.distanciaAlObjetivo` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L78) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `completarPaso`, `controlarAvance`, `iniciarAvance`, `iniciarRecuperacionEndpoint` | — | — |
| `PoseEstimator.anguloAlObjetivoRad` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L84) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance`, `iniciarRecuperacionEndpoint` | — | — |
| `setup_Red` | [`src/Red.cpp`](../../src/Red.cpp#L423) | 4 | ESP32 / tiempo real | Medio; interno; asíncrona | `setup` | — | — |
| `procesarWebSockets` | [`src/Red.cpp`](../../src/Red.cpp#L437) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `Task_Web` | — | — |
| `pushTelemetria` | [`src/Red.cpp`](../../src/Red.cpp#L443) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `Task_Web` | — | — |
| `Seguridad.Seguridad` | [`src/Seguridad.cpp`](../../src/Seguridad.cpp#L11) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Seguridad.reiniciarSaludEncoders` | [`src/Seguridad.cpp`](../../src/Seguridad.cpp#L22) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `resetFallo` | — | — |
| `Seguridad.actualizarSaludEncoders` | [`src/Seguridad.cpp`](../../src/Seguridad.cpp#L37) | 33 | ESP32 / tiempo real | Alto; sin llamada interna detectada; síncrona | — | `enFaseAvance`, `encoderSinRespuestaAislada`, `medianaCuatro`, `promedioConfiableLado` | — |
| `Seguridad.auditarSalud` | [`src/Seguridad.cpp`](../../src/Seguridad.cpp#L150) | 19 | ESP32 / tiempo real | Alto; sin llamada interna detectada; evento | — | `enFaseAvance`, `enFaseCalibracion`, `enFaseGiro`, `encolarEvento`, `frenarMotores`, `ladoEnStall`, `registrarMotivoFinalizacion`, `reiniciarControlRumbo` | parada/cierre |
| `Seguridad.forzarEStop` | [`src/Seguridad.cpp`](../../src/Seguridad.cpp#L226) | 1 | ESP32 / tiempo real | Medio; interno; síncrona | `procesarComandos` | `frenarMotores`, `registrarMotivoFinalizacion`, `reiniciarControlRumbo` | parada/cierre |
| `Seguridad.resetFallo` | [`src/Seguridad.cpp`](../../src/Seguridad.cpp#L236) | 5 | ESP32 / tiempo real | Medio; interno; síncrona | `procesarComandos` | `frenarMotores`, `motoresListos`, `registrarMotivoFinalizacion`, `reiniciarControlRumbo`, `reiniciarSaludEncoders` | parada/cierre |
| `setup_Sensores` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L82) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `setup` | — | — |
| `resetFiltrosEncoder` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L158) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `resetConfEncoders` | — | — |
| `resetOrientacionIMU` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L165) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `completarGiro`, `procesarComandos` | — | — |
| `obtenerYawIMUDeg` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L174) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `recentrarYawIMUEnReposo` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L181) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `cantidadRecentradosYawIMU` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L198) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `leerSensoresSincrono` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L253) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `snapshotSensoresControl` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L267) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `loop` | — | — |
| `obtenerUltimoSnapshotSensores` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L271) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `sensar` | — | — |
| `procesarComandos` | [`src/main.cpp`](../../src/main.cpp#L22) | 26 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; cola/evento | — | `cancelarMovimiento`, `encolarEvento`, `estopSolicitado`, `forzarEStop`, `iniciarCalibracion`, `iniciarGiroAbsoluto`, `iniciarPaso`, `reset`, `resetFallo`, `resetOrientacionIMU` | cola de comandos |
| `Task_Web` | [`src/main.cpp`](../../src/main.cpp#L99) | 3 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `procesarWebSockets`, `pushTelemetria`, `registrarStackLibre` | — |
| `setup` | [`src/main.cpp`](../../src/main.cpp#L151) | 3 | ESP32 / tiempo real | Alto; entrada/framework; cola/evento | — | `estadoMotores`, `frenarMotores`, `inicializar`, `inicializarDiagnosticoRTOS`, `registrarResultadoArquitectura`, `setup_MotorPinsLow`, `setup_Motores`, `setup_Red`, `setup_Sensores` | cola de comandos |
| `loop` | [`src/main.cpp`](../../src/main.cpp#L182) | 6 | ESP32 / tiempo real | Bajo; entrada/framework; síncrona | `init` | `registrarCicloControl`, `registrarStackLibre`, `snapshotSensoresControl` | — |
