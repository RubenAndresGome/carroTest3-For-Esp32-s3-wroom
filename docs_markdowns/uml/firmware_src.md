# UML funcional: `firmware/src`

Funciones detectadas: **153**. Tipos detectados: **0**.

## Grafo de llamadas

```mermaid
flowchart LR
  subgraph f0["Cinematica.cpp"]
    ncb86f77747["errorAng360(float obj,float act)"]
    n2cdf0a00ad["aproximar(float v)"]
    n273e22764e["sensar()"]
    nb87b152c3e["copiarBase(int64_t dest[4],const SensorSnapshot & s)"]
    n4deacc3931["deltas(const int64_t base[4],const SensorSnapshot & s,int64_t out[4])"]
    ne39a515d5b["fin(TipoEvento t,const char * d)"]
    n277fc076c3["fallo(const char * d)"]
    n84d2f7a6e8["reiniciarDiagnosticoCalibracion()"]
    n354a162d68["actualizarDiagnosticoCalibracion(const int64_t deltasEncoder[4],const ControlCalibracion::EvaluacionEncoders & evaluacion)"]
    n985cca2261["conservarEncodersAisladosDelDiagnostico()"]
    n1536472918["iniciarFaseCal(Fase f)"]
    nb1ccae4e63["calCuenta()"]
    ndc77b4e738["calTorque(bool primera)"]
    n5a69d606a1["controlarCalibracion()"]
    na58f1ea422["iniciarBaseGiro(float objetivoDeg,Fase retorno)"]
    n1df2e7728e["reintentarGiro(const char * motivo)"]
    na82e5563dd["controlarGiro()"]
    n7d36f5ae00["completarGiro()"]
    na4ef79cfc7["actualizarErroresTrayectoria()"]
    n7a7c60f38a["objetivoAbsolutoAlcanzado()"]
    n95c11d51ff["mediana4(const int64_t v[4])"]
    ne8f5691d2c["hayPorLado()"]
    nfdea583895["promedioLado(const int64_t v[4],bool izq)"]
    nbbfae14d03["estimarTicksAvance(const int64_t v[4])"]
    ne51470b710["resetConfEncoders()"]
    ncf16254702["iniciarAvance(bool conservar)"]
    n8cdb6d1362["detectarOutliers(const int64_t v[4])"]
    n2e609e6682["iniciarPausaReeval(const int64_t v[4])"]
    n081f7dbdd3["completarPausaReeval()"]
    ndf8b62cfa6["pwmAntiFriccion(uint8_t indice)"]
    n7eae7f3bd5["iniciarAntiFriccion(const SensorSnapshot & s)"]
    nf6733ca3a5["controlarAntiFriccion(const SensorSnapshot & s)"]
    ne50f946368["controlarAvance()"]
    na14c9fa6e6["iniciarAsentamientoFinal(float distanciaAntesDeFrenarCm)"]
    n8a5e6c7716["controlarAsentamientoFinal()"]
    n2f2215762a["iniciarVerificacionFinal()"]
    nc76edf3b63["iniciarRecuperacionEndpoint()"]
    n5a0ebf1f93["verificarObjetivoFinal()"]
    n61261e0a0e["completarPaso()"]
    nf19c98de65["completarPasoConCorreccionPendiente()"]
    n8d85de75d9["iniciarPausaPreGiro()"]
    nb666539749["controlarPausaPreGiro()"]
    n30cebd1476["iniciarPausaPreAvance(bool conservar)"]
    nf6da1485f4["controlarPausaPreAvance()"]
    ndc64671e1f["iniciarPasoInterno()"]
    n111926673b["normalizar360(float a)"]
    n0a73dbd3ed["reiniciarControlRumbo()"]
    nbfa6c46bef["registrarMotivoFinalizacion(const char * detalle)"]
    nf7686d9760["enFaseAvance()"]
    n3722641434["enFaseTraslacion()"]
    n4c2a9b2060["enFaseGiro()"]
    nd210c4946f["enFaseCalibracion()"]
    n17ec3beebf["obtenerDiagnosticoCalibracion()"]
    n5b97d7b41b["iniciarCalibracion(int seq)"]
    n4571246892["aplicarCalibracionInyectada(int pwmPos8,int pwmNeg8,int candPos,int candNeg,int seq)"]
    n3eef13fd85["intentarAutoRestaurarCalibracion()"]
    nde63dc1780["iniciarPaso(float heading,float distanciaCm,int seq,float targetX,float targetY,bool objetivoAbsoluto,ModoPaso modoPaso)"]
    n59633ad4c9["iniciarGiroAbsoluto(float heading,int seq)"]
    n3912029e14["cancelarMovimiento(const char * detalle)"]
    n0fd4becfa6["controlarMovimiento()"]
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
    neecc643dc4["contenidoSinNul(const char * valor,size_t longitud)"]
    nb6d317a36e["cadenaConfiguracionValida(const char (& valor)[N])"]
  end
  subgraph f3["Eventos.cpp"]
    na2bfb4ac57["encolarEvento(TipoEvento tipo,int seq,const char * detalle,float progreso,uint32_t runId)"]
  end
  subgraph f4["MemoriaTorque.cpp"]
    n800bc2a36f["copiarEstado(const char * estado)"]
    nab0ce6938e["recalcularDiagnostico()"]
    nb4100066d3["leerArchivo(const char * ruta,ControlTorque::Historial & destino)"]
    n14c464bc84["escribirArchivo(const char * ruta,const ControlTorque::Historial & origen)"]
    n64cd1a02c7["guardarAtomico(const ControlTorque::Historial & origen)"]
    n4f632957c4["setup_MemoriaTorque()"]
    n4c39687486["baseTorqueParaPolaridad8(int polaridad,bool izquierda)"]
    nb48dd0e6af["solicitarGuardarTorque(int pwmPositivo8,int pwmNegativo8,int polaridadPositiva,int polaridadNegativa,int pwmPositivoDerecho8,int pwmNegativoDerecho8)"]
    n6d9bdca8a3["procesarPersistenciaTorque()"]
    n376ca993c8["persistenciaTorquePendiente()"]
    nf9d652685a["obtenerDiagnosticoMemoriaTorque()"]
    n2555e8b054["obtenerCalibracionVigente(int & pwmPos8,int & pwmNeg8,int & candPos,int & candNeg)"]
  end
  subgraph f5["Motores.cpp"]
    neee8549a3a["validarMapaMotores()"]
    n126a520596["establecerLimiteContinuoPwm(int limite)"]
    n3f1b7b5722["limiteContinuoPwm()"]
    n1789646a5e["aplicarVelocidades(int velIzq,int velDer)"]
    ndbd864cc3a["frenarMotores()"]
    n744443e7c2["frenarMotoresActivo()"]
    n077fadc1f4["actualizarFrenoActivo()"]
    n18cac09301["frenoActivoEnCurso()"]
    nd639d4916f["validarInterlockMotores()"]
    n870b3adfc4["estadoInterlockL()"]
    n643f343af5["estadoInterlockR()"]
    n9904230b64["signoEnergizadoL()"]
    ndc3aafbdb4["signoEnergizadoR()"]
    n07c6b03914["signoPendienteL()"]
    n7bc8c5c7d7["signoPendienteR()"]
    nb0f0bc72bd["motoresListos()"]
    na8ca3bcae3["estadoMotores()"]
    n5ea1aceaef["setup_MotorPinsLow()"]
    n0e9550e432["setup_Motores()"]
  end
  subgraph f6["PoseEstimator.cpp"]
    nb50d63cd8c["PoseEstimator()"]
    n49d925a63d["inicializar(float rueda_diametro_cm,int encoder_ppr)"]
    nfa3371d39f["reset()"]
    n3ff2a453a5["actualizarOdometria(int64_t pulsosFL,int64_t pulsosFR,int64_t pulsosBL,int64_t pulsosBR,bool avanzando)"]
    nfe73810903["iniciarMedicionTraslacionGiro()"]
    n9ec5b23819["aplicarCorreccionICR(float deltaThetaRad,float xIcrCm,float yIcrCm)"]
    neef862aa56["actualizarOrientacion(float imu_delta_z)"]
    n776d78ca65["distanciaAlObjetivo(float obj_x,float obj_y)"]
    n7439a3112e["anguloAlObjetivoRad(float obj_x,float obj_y)"]
  end
  subgraph f7["Red.cpp"]
    n784b6f77e2["setup_Red()"]
    n68b411cab1["procesarWebSockets()"]
    n2090a9dd09["pushTelemetria()"]
  end
  subgraph f8["Seguridad.cpp"]
    nf8836652dd["Seguridad()"]
    n0c00dc27c4["reiniciarSaludEncoders()"]
    nfc565f3c71["prepararRevalidacionEncoders()"]
    n9293df49e4["aplicarClasificacionEncoders(const bool confiables[4])"]
    n799e4d3e08["actualizarSaludEncoders(const SensorSnapshot & snap,int pwm_L,int pwm_R)"]
    n6e85a6b4bd["auditarSalud(const SensorSnapshot & snap,int pwm_L,int pwm_R)"]
    n8e431c2804["forzarEStop()"]
    n8527ad45db["resetFallo()"]
  end
  subgraph f9["Sensores.cpp"]
    nd5bd6fc755["setup_Sensores()"]
    nc2cca3f386["establecerEscalaGiro(float escala)"]
    n0f1d85cf10["obtenerEscalaGiro()"]
    n795bd94ffa["pcntInicializados()"]
    n7f61b0b0d7["diagnosticoInicializacionPCNT()"]
    nb9176f8738["resetFiltrosEncoder()"]
    n082dfb0939["resetOrientacionIMU()"]
    n109f04c292["recalibrarOffsetIMU(uint16_t muestras)"]
    n75e7bcda03["obtenerYawIMUDeg()"]
    n3caaf8dc39["recentrarYawIMUEnReposo()"]
    n355a7c9700["cantidadRecentradosYawIMU()"]
    n24c65c5f22["leerSensoresSincrono()"]
    n5d4a515c7a["snapshotSensoresControl()"]
    n27af0944aa["obtenerUltimoSnapshotSensores(SensorSnapshot & destino)"]
  end
  subgraph f10["main.cpp"]
    nb9845af75d["solicitarManualBegin(int seq)"]
    n0650261cfc["solicitarManualEnd(int seq)"]
    n23b62ea060["solicitarManualDesconexion()"]
    n12cb8df5ce["solicitarDesconexionControl()"]
    nf6e54ee365["tomarDesconexionControl()"]
    n4416c307d2["publicarManualDrive(float throttle,float steering,uint32_t stream,uint32_t frame)"]
    nd688da8237["tomarSolicitudManual(bool & comenzar,bool & terminar,bool & desconexion,int & seq)"]
    n65aaa19e30["leerManualDrive(ManualDriveFrame & trama)"]
    nfc84110464["limpiarManualDrive()"]
    n3382e08ee9["faseManualTexto()"]
    n334e48f242["inicioManualMs()"]
    nb8469f7ee2["procesarComandos()"]
    nd62de0f5b7["Task_Web(void *)"]
    na153aa42c8["setup()"]
    n9392b16053["loop()"]
  end
  n081f7dbdd3 --> n277fc076c3
  n081f7dbdd3 --> ncf16254702
  n0e9550e432 --> ndbd864cc3a
  n0e9550e432 --> neee8549a3a
  n0fd4becfa6 --> n2f2215762a
  n0fd4becfa6 --> n5a0ebf1f93
  n0fd4becfa6 --> n5a69d606a1
  n0fd4becfa6 --> n8a5e6c7716
  n0fd4becfa6 --> na82e5563dd
  n0fd4becfa6 --> nb666539749
  n0fd4becfa6 --> ne50f946368
  n0fd4becfa6 --> nf6da1485f4
  n1789646a5e --> nb0f0bc72bd
  n1789646a5e --> neee8549a3a
  n1b9377afe9 --> ne4a753795c
  n1df2e7728e --> n277fc076c3
  n1df2e7728e --> ndbd864cc3a
  n273e22764e --> n27af0944aa
  n277fc076c3 --> ne39a515d5b
  n2e609e6682 --> n0a73dbd3ed
  n2e609e6682 --> ndbd864cc3a
  n2f2215762a --> ndbd864cc3a
  n30cebd1476 --> ndbd864cc3a
  n3912029e14 -.-> n0a73dbd3ed
  n3912029e14 -.-> na2bfb4ac57
  n3912029e14 -.-> nbfa6c46bef
  n3912029e14 -.-> ndbd864cc3a
  n3eef13fd85 --> n2555e8b054
  n3eef13fd85 --> n273e22764e
  n3eef13fd85 --> n4571246892
  n4571246892 -.-> n082dfb0939
  n4571246892 -.-> n2555e8b054
  n4571246892 -.-> n273e22764e
  n4571246892 -.-> na2bfb4ac57
  n4571246892 -.-> nb48dd0e6af
  n4571246892 -.-> ndbd864cc3a
  n4571246892 -.-> nfa3371d39f
  n4f632957c4 --> n800bc2a36f
  n4f632957c4 --> nab0ce6938e
  n4f632957c4 --> nb4100066d3
  n59633ad4c9 -.-> n111926673b
  n59633ad4c9 -.-> na2bfb4ac57
  n59633ad4c9 -.-> na58f1ea422
  n59633ad4c9 -.-> nbfa6c46bef
  n5a0ebf1f93 --> n273e22764e
  n5a0ebf1f93 --> n61261e0a0e
  n5a0ebf1f93 --> na58f1ea422
  n5a0ebf1f93 --> ncb86f77747
  n5a0ebf1f93 --> ndbd864cc3a
  n5a69d606a1 --> n111926673b
  n5a69d606a1 --> n1536472918
  n5a69d606a1 --> n273e22764e
  n5a69d606a1 --> n277fc076c3
  n5a69d606a1 --> na58f1ea422
  n5a69d606a1 --> na82e5563dd
  n5a69d606a1 --> nb1ccae4e63
  n5a69d606a1 --> nb87b152c3e
  n5a69d606a1 --> ndbd864cc3a
  n5a69d606a1 --> ndc77b4e738
  n5b97d7b41b -.-> n126a520596
  n5b97d7b41b -.-> n273e22764e
  n5b97d7b41b -.-> n84d2f7a6e8
  n5b97d7b41b -.-> na2bfb4ac57
  n5b97d7b41b -.-> nbfa6c46bef
  n5ea1aceaef --> neee8549a3a
  n61261e0a0e --> ne39a515d5b
  n64cd1a02c7 --> n14c464bc84
  n64cd1a02c7 --> nb4100066d3
  n6d9bdca8a3 --> n64cd1a02c7
  n6d9bdca8a3 --> n800bc2a36f
  n6d9bdca8a3 --> nab0ce6938e
  n6e85a6b4bd -.-> n0a73dbd3ed
  n6e85a6b4bd -.-> n4c2a9b2060
  n6e85a6b4bd -.-> na2bfb4ac57
  n6e85a6b4bd -.-> nbfa6c46bef
  n6e85a6b4bd -.-> nd210c4946f
  n6e85a6b4bd -.-> ndbd864cc3a
  n6e85a6b4bd -.-> nf7686d9760
  n73252a2cfd --> ned6049114b
  n7a7c60f38a --> na4ef79cfc7
  n7a7c60f38a --> ncb86f77747
  n7d36f5ae00 --> n082dfb0939
  n7d36f5ae00 --> n1536472918
  n7d36f5ae00 --> n2f2215762a
  n7d36f5ae00 --> n30cebd1476
  n7d36f5ae00 --> nb48dd0e6af
  n7d36f5ae00 --> ndbd864cc3a
  n7d36f5ae00 --> ne39a515d5b
  n7d36f5ae00 --> nfa3371d39f
  n7eae7f3bd5 --> nb87b152c3e
  n7eae7f3bd5 --> ndbd864cc3a
  n7eae7f3bd5 --> ndf8b62cfa6
  n7fe919940e --> n13d5d5c8e7
  n8527ad45db --> n0a73dbd3ed
  n8527ad45db --> n795bd94ffa
  n8527ad45db --> nb0f0bc72bd
  n8527ad45db --> nbfa6c46bef
  n8527ad45db --> ndbd864cc3a
  n8527ad45db --> nfc565f3c71
  n8a5e6c7716 --> n273e22764e
  n8a5e6c7716 --> n277fc076c3
  n8a5e6c7716 --> n4deacc3931
  n8a5e6c7716 --> n744443e7c2
  n8a5e6c7716 --> na4ef79cfc7
  n8a5e6c7716 --> nbbfae14d03
  n8cdb6d1362 --> n95c11d51ff
  n8cdb6d1362 --> ncb86f77747
  n8d85de75d9 --> ndbd864cc3a
  n8e431c2804 --> n0a73dbd3ed
  n8e431c2804 --> nbfa6c46bef
  n8e431c2804 --> ndbd864cc3a
  n93202db9d6 --> n999b6f2133
  n9392b16053 --> n5d4a515c7a
  n9392b16053 --> n93202db9d6
  n9392b16053 --> ndf604692a1
  na14c9fa6e6 --> n126a520596
  na14c9fa6e6 --> n273e22764e
  na14c9fa6e6 --> n744443e7c2
  na14c9fa6e6 --> nb87b152c3e
  na153aa42c8 -.-> n0e9550e432
  na153aa42c8 -.-> n49d925a63d
  na153aa42c8 -.-> n4f632957c4
  na153aa42c8 -.-> n5ea1aceaef
  na153aa42c8 -.-> n73252a2cfd
  na153aa42c8 -.-> n784b6f77e2
  na153aa42c8 -.-> n795bd94ffa
  na153aa42c8 -.-> n7fe919940e
  na153aa42c8 -.-> na8ca3bcae3
  na153aa42c8 -.-> nbfa6c46bef
  na153aa42c8 -.-> nd5bd6fc755
  na153aa42c8 -.-> ndbd864cc3a
  na58f1ea422 --> n0a73dbd3ed
  na58f1ea422 --> n126a520596
  na58f1ea422 --> n273e22764e
  na58f1ea422 --> nb87b152c3e
  na58f1ea422 --> ncb86f77747
  na58f1ea422 --> nfe73810903
  na82e5563dd --> n1789646a5e
  na82e5563dd --> n1df2e7728e
  na82e5563dd --> n273e22764e
  na82e5563dd --> n277fc076c3
  na82e5563dd --> n2cdf0a00ad
  na82e5563dd --> n4deacc3931
  na82e5563dd --> n744443e7c2
  na82e5563dd --> n7d36f5ae00
  na82e5563dd --> nb48dd0e6af
  na82e5563dd --> nb87b152c3e
  na82e5563dd --> ncb86f77747
  na82e5563dd --> ndbd864cc3a
  nb1ccae4e63 --> n109f04c292
  nb1ccae4e63 --> n111926673b
  nb1ccae4e63 --> n1536472918
  nb1ccae4e63 --> n273e22764e
  nb1ccae4e63 --> n277fc076c3
  nb1ccae4e63 --> n84d2f7a6e8
  nb1ccae4e63 --> nb87b152c3e
  nb48dd0e6af --> n800bc2a36f
  nb48dd0e6af --> nab0ce6938e
  nb50d63cd8c --> nfa3371d39f
  nb666539749 --> n273e22764e
  nb666539749 --> na58f1ea422
  nb666539749 --> ncb86f77747
  nb666539749 --> ncf16254702
  nb666539749 --> ndbd864cc3a
  nb6d317a36e --> neecc643dc4
  nb8469f7ee2 -.-> n082dfb0939
  nb8469f7ee2 -.-> n3912029e14
  nb8469f7ee2 -.-> n4571246892
  nb8469f7ee2 -.-> n59633ad4c9
  nb8469f7ee2 -.-> n5b97d7b41b
  nb8469f7ee2 -.-> n795bd94ffa
  nb8469f7ee2 -.-> n8527ad45db
  nb8469f7ee2 -.-> n8e431c2804
  nb8469f7ee2 -.-> na2bfb4ac57
  nb8469f7ee2 -.-> nc2cca3f386
  nb8469f7ee2 -.-> nde63dc1780
  nb8469f7ee2 -.-> nfa3371d39f
  nc76edf3b63 --> n111926673b
  nc76edf3b63 --> n277fc076c3
  nc76edf3b63 --> n7439a3112e
  nc76edf3b63 --> n776d78ca65
  nc76edf3b63 --> na4ef79cfc7
  nc76edf3b63 --> na58f1ea422
  nc76edf3b63 --> ncb86f77747
  nc76edf3b63 --> ncf16254702
  nc76edf3b63 --> nf19c98de65
  ncf16254702 --> n0a73dbd3ed
  ncf16254702 --> n126a520596
  ncf16254702 --> n273e22764e
  ncf16254702 --> n776d78ca65
  ncf16254702 --> na4ef79cfc7
  ncf16254702 --> nb87b152c3e
  ncf16254702 --> ne51470b710
  nd62de0f5b7 --> n2090a9dd09
  nd62de0f5b7 --> n68b411cab1
  nd62de0f5b7 --> n6d9bdca8a3
  nd62de0f5b7 --> n93202db9d6
  ndc64671e1f --> n30cebd1476
  ndc64671e1f --> n8d85de75d9
  ndc64671e1f --> ncb86f77747
  ndc77b4e738 --> n111926673b
  ndc77b4e738 --> n1536472918
  ndc77b4e738 --> n1789646a5e
  ndc77b4e738 --> n273e22764e
  ndc77b4e738 --> n277fc076c3
  ndc77b4e738 --> n354a162d68
  ndc77b4e738 --> n4deacc3931
  ndc77b4e738 --> n985cca2261
  ndc77b4e738 --> na58f1ea422
  ndc77b4e738 --> nb87b152c3e
  ndc77b4e738 --> ndbd864cc3a
  nde63dc1780 -.-> n111926673b
  nde63dc1780 -.-> na2bfb4ac57
  nde63dc1780 -.-> na4ef79cfc7
  nde63dc1780 -.-> nbfa6c46bef
  nde63dc1780 -.-> ndc64671e1f
  ndf57242908 --> n13d5d5c8e7
  ndf57242908 --> ne4a753795c
  ndf57242908 --> ned6049114b
  ne39a515d5b -.-> n0a73dbd3ed
  ne39a515d5b -.-> na2bfb4ac57
  ne39a515d5b -.-> nbfa6c46bef
  ne39a515d5b -.-> ndbd864cc3a
  ne50f946368 --> n081f7dbdd3
  ne50f946368 --> n111926673b
  ne50f946368 --> n1789646a5e
  ne50f946368 --> n273e22764e
  ne50f946368 --> n277fc076c3
  ne50f946368 --> n2cdf0a00ad
  ne50f946368 --> n4deacc3931
  ne50f946368 --> n7439a3112e
  ne50f946368 --> n744443e7c2
  ne50f946368 --> n776d78ca65
  ne50f946368 --> n7eae7f3bd5
  ne50f946368 --> na14c9fa6e6
  ne50f946368 --> na4ef79cfc7
  ne50f946368 --> na58f1ea422
  ne50f946368 --> nbbfae14d03
  ne50f946368 --> nbfa6c46bef
  ne50f946368 --> ncb86f77747
  ne50f946368 --> ndbd864cc3a
  ne50f946368 --> ne39a515d5b
  ne50f946368 --> ne8f5691d2c
  ne50f946368 --> nf6733ca3a5
  ne50f946368 --> nfdea583895
  ne51470b710 --> nb9176f8738
  nf19c98de65 --> ne39a515d5b
  nf6733ca3a5 --> n1789646a5e
  nf6733ca3a5 --> n277fc076c3
  nf6733ca3a5 --> n4deacc3931
  nf6733ca3a5 --> nb87b152c3e
  nf6733ca3a5 --> ndbd864cc3a
  nf6733ca3a5 --> ndf8b62cfa6
  nf6733ca3a5 --> nfdea583895
  nf6da1485f4 --> n273e22764e
  nf6da1485f4 --> na58f1ea422
  nf6da1485f4 --> ncb86f77747
  nf6da1485f4 --> ncf16254702
  nf6da1485f4 --> ndbd864cc3a
  nfa3371d39f --> nfe73810903
  classDef alto fill:#5b1f2a,stroke:#ff7a7a,color:#fff
  classDef medio fill:#4a3717,stroke:#ffca67,color:#fff
  classDef bajo fill:#123b3a,stroke:#39e6aa,color:#fff
  class ncb86f77747 bajo
  class n2cdf0a00ad bajo
  class n273e22764e bajo
  class nb87b152c3e bajo
  class n4deacc3931 bajo
  class ne39a515d5b bajo
  class n277fc076c3 bajo
  class n84d2f7a6e8 bajo
  class n354a162d68 bajo
  class n985cca2261 bajo
  class n1536472918 bajo
  class nb1ccae4e63 bajo
  class ndc77b4e738 alto
  class n5a69d606a1 bajo
  class na58f1ea422 bajo
  class n1df2e7728e bajo
  class na82e5563dd alto
  class n7d36f5ae00 bajo
  class na4ef79cfc7 bajo
  class n7a7c60f38a bajo
  class n95c11d51ff bajo
  class ne8f5691d2c bajo
  class nfdea583895 bajo
  class nbbfae14d03 bajo
  class ne51470b710 bajo
  class ncf16254702 bajo
  class n8cdb6d1362 bajo
  class n2e609e6682 bajo
  class n081f7dbdd3 bajo
  class ndf8b62cfa6 bajo
  class n7eae7f3bd5 bajo
  class nf6733ca3a5 alto
  class ne50f946368 alto
  class na14c9fa6e6 bajo
  class n8a5e6c7716 bajo
  class n2f2215762a bajo
  class nc76edf3b63 bajo
  class n5a0ebf1f93 bajo
  class n61261e0a0e bajo
  class nf19c98de65 bajo
  class n8d85de75d9 bajo
  class nb666539749 bajo
  class n30cebd1476 bajo
  class nf6da1485f4 bajo
  class ndc64671e1f bajo
  class n111926673b bajo
  class n0a73dbd3ed bajo
  class nbfa6c46bef bajo
  class nf7686d9760 bajo
  class n3722641434 bajo
  class n4c2a9b2060 bajo
  class nd210c4946f bajo
  class n17ec3beebf bajo
  class n5b97d7b41b bajo
  class n4571246892 bajo
  class n3eef13fd85 bajo
  class nde63dc1780 bajo
  class n59633ad4c9 bajo
  class n3912029e14 medio
  class n0fd4becfa6 bajo
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
  class neecc643dc4 bajo
  class nb6d317a36e bajo
  class na2bfb4ac57 bajo
  class n800bc2a36f bajo
  class nab0ce6938e bajo
  class nb4100066d3 bajo
  class n14c464bc84 bajo
  class n64cd1a02c7 bajo
  class n4f632957c4 bajo
  class n4c39687486 bajo
  class nb48dd0e6af bajo
  class n6d9bdca8a3 bajo
  class n376ca993c8 bajo
  class nf9d652685a bajo
  class n2555e8b054 bajo
  class neee8549a3a bajo
  class n126a520596 bajo
  class n3f1b7b5722 bajo
  class n1789646a5e alto
  class ndbd864cc3a bajo
  class n744443e7c2 alto
  class n077fadc1f4 bajo
  class n18cac09301 bajo
  class nd639d4916f bajo
  class n870b3adfc4 bajo
  class n643f343af5 bajo
  class n9904230b64 bajo
  class ndc3aafbdb4 bajo
  class n07c6b03914 bajo
  class n7bc8c5c7d7 bajo
  class nb0f0bc72bd bajo
  class na8ca3bcae3 bajo
  class n5ea1aceaef bajo
  class n0e9550e432 alto
  class nb50d63cd8c bajo
  class n49d925a63d bajo
  class nfa3371d39f bajo
  class n3ff2a453a5 bajo
  class nfe73810903 bajo
  class n9ec5b23819 bajo
  class neef862aa56 bajo
  class n776d78ca65 bajo
  class n7439a3112e bajo
  class n784b6f77e2 medio
  class n68b411cab1 bajo
  class n2090a9dd09 bajo
  class nf8836652dd bajo
  class n0c00dc27c4 bajo
  class nfc565f3c71 bajo
  class n9293df49e4 bajo
  class n799e4d3e08 alto
  class n6e85a6b4bd alto
  class n8e431c2804 medio
  class n8527ad45db medio
  class nd5bd6fc755 bajo
  class nc2cca3f386 bajo
  class n0f1d85cf10 bajo
  class n795bd94ffa bajo
  class n7f61b0b0d7 bajo
  class nb9176f8738 bajo
  class n082dfb0939 bajo
  class n109f04c292 bajo
  class n75e7bcda03 bajo
  class n3caaf8dc39 bajo
  class n355a7c9700 bajo
  class n24c65c5f22 bajo
  class n5d4a515c7a bajo
  class n27af0944aa bajo
  class nb9845af75d bajo
  class n0650261cfc bajo
  class n23b62ea060 bajo
  class n12cb8df5ce bajo
  class nf6e54ee365 bajo
  class n4416c307d2 bajo
  class nd688da8237 bajo
  class n65aaa19e30 bajo
  class nfc84110464 bajo
  class n3382e08ee9 bajo
  class n334e48f242 bajo
  class nb8469f7ee2 bajo
  class nd62de0f5b7 bajo
  class na153aa42c8 alto
  class n9392b16053 bajo
```

Fuentes: [Mermaid](mermaid/firmware_src.mmd) · [PlantUML](plantuml/firmware_src.puml). Las flechas continuas son llamadas síncronas; las discontinuas representan asincronía, eventos o colas. El color del nodo indica riesgo estático.

## Inventario

| Función | Archivo | CC | Propietario | Riesgo/estado | Entra desde | Sale hacia | Estado compartido |
|---|---|---:|---|---|---|---|---|
| `__anon3dcaec680111.errorAng360` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L81) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance`, `controlarGiro`, `controlarPausaPreAvance`, `controlarPausaPreGiro`, `detectarOutliers`, `iniciarBaseGiro`, `iniciarPasoInterno`, `iniciarRecuperacionEndpoint`, `objetivoAbsolutoAlcanzado`, `verificarObjetivoFinal` | — | — |
| `__anon3dcaec680111.aproximar` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L87) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance`, `controlarGiro` | — | — |
| `__anon3dcaec680111.sensar` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L90) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `aplicarCalibracionInyectada`, `calCuenta`, `calTorque`, `controlarAsentamientoFinal`, `controlarAvance`, `controlarCalibracion`, `controlarGiro`, `controlarPausaPreAvance`, `controlarPausaPreGiro`, `iniciarAsentamientoFinal`, `iniciarAvance`, `iniciarBaseGiro`, `iniciarCalibracion`, `intentarAutoRestaurarCalibracion`, `verificarObjetivoFinal` | `obtenerUltimoSnapshotSensores` | — |
| `__anon3dcaec680111.copiarBase` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L95) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `calCuenta`, `calTorque`, `controlarAntiFriccion`, `controlarCalibracion`, `controlarGiro`, `iniciarAntiFriccion`, `iniciarAsentamientoFinal`, `iniciarAvance`, `iniciarBaseGiro` | — | — |
| `__anon3dcaec680111.deltas` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L98) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `calTorque`, `controlarAntiFriccion`, `controlarAsentamientoFinal`, `controlarAvance`, `controlarGiro` | — | — |
| `__anon3dcaec680111.fin` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L104) | 3 | ESP32 / tiempo real | Bajo; interno; evento | `completarGiro`, `completarPaso`, `completarPasoConCorreccionPendiente`, `controlarAvance`, `fallo` | `encolarEvento`, `frenarMotores`, `registrarMotivoFinalizacion`, `reiniciarControlRumbo` | — |
| `__anon3dcaec680111.fallo` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L117) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `calCuenta`, `calTorque`, `completarPausaReeval`, `controlarAntiFriccion`, `controlarAsentamientoFinal`, `controlarAvance`, `controlarCalibracion`, `controlarGiro`, `iniciarRecuperacionEndpoint`, `reintentarGiro` | `fin` | — |
| `__anon3dcaec680111.reiniciarDiagnosticoCalibracion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L134) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `calCuenta`, `iniciarCalibracion` | `totalPasosRampa` | — |
| `__anon3dcaec680111.actualizarDiagnosticoCalibracion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L141) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `calTorque` | `pasoRampaActual` | — |
| `__anon3dcaec680111.conservarEncodersAisladosDelDiagnostico` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L163) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `calTorque` | — | — |
| `__anon3dcaec680111.iniciarFaseCal` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L176) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `calCuenta`, `calTorque`, `completarGiro`, `controlarCalibracion` | — | — |
| `__anon3dcaec680111.calCuenta` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L180) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarCalibracion` | `copiarBase`, `fallo`, `iniciarFaseCal`, `normalizar360`, `recalibrarOffsetIMU`, `reiniciarDiagnosticoCalibracion`, `sensar` | — |
| `__anon3dcaec680111.calTorque` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L197) | 29 | ESP32 / tiempo real | Alto; interno; síncrona | `controlarCalibracion` | `actualizarDiagnosticoCalibracion`, `aplicarVelocidades`, `conservarEncodersAisladosDelDiagnostico`, `copiarBase`, `deltas`, `evaluarEncoders`, `evaluarMovimiento`, `fallo`, `frenarMotores`, `iniciarBaseGiro`, `iniciarFaseCal`, `normalizar360`, `sensar` | — |
| `__anon3dcaec680111.controlarCalibracion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L304) | 14 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarMovimiento` | `calCuenta`, `calTorque`, `controlarGiro`, `copiarBase`, `fallo`, `frenarMotores`, `iniciarBaseGiro`, `iniciarFaseCal`, `normalizar360`, `sensar` | — |
| `__anon3dcaec680111.iniciarBaseGiro` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L377) | 11 | ESP32 / tiempo real | Bajo; interno; síncrona | `calTorque`, `controlarAvance`, `controlarCalibracion`, `controlarPausaPreAvance`, `controlarPausaPreGiro`, `iniciarGiroAbsoluto`, `iniciarRecuperacionEndpoint`, `verificarObjetivoFinal` | `copiarBase`, `errorAng360`, `establecerLimiteContinuoPwm`, `iniciarMedicionTraslacionGiro`, `reiniciar`, `reiniciarControlRumbo`, `sensar` | — |
| `__anon3dcaec680111.reintentarGiro` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L429) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarGiro` | `fallo`, `frenarMotores` | — |
| `__anon3dcaec680111.controlarGiro` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L439) | 132 | ESP32 / tiempo real | Alto; interno; síncrona | `controlarCalibracion`, `controlarMovimiento` | `aplicarVelocidades`, `aproximar`, `balancearGiroDiferencial`, `completarGiro`, `copiarBase`, `deltas`, `distanciaPorTick`, `errorAng360`, `evaluarDivergenciaGiro`, `fallo`, `frenarMotores`, `frenarMotoresActivo`, `pasoRampaActual`, `promedioConfiableLado`, `reiniciar`, `reintentarGiro`, `sensar`, `solicitarGuardarTorque` | — |
| `__anon3dcaec680111.completarGiro` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L784) | 9 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarGiro` | `fin`, `frenarMotores`, `iniciarFaseCal`, `iniciarPausaPreAvance`, `iniciarVerificacionFinal`, `reset`, `resetOrientacionIMU`, `solicitarGuardarTorque` | — |
| `__anon3dcaec680111.actualizarErroresTrayectoria` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L835) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAsentamientoFinal`, `controlarAvance`, `iniciarAvance`, `iniciarPaso`, `iniciarRecuperacionEndpoint`, `objetivoAbsolutoAlcanzado` | `calcularErroresTrayectoriaAnclado`, `getX`, `getY` | — |
| `__anon3dcaec680111.objetivoAbsolutoAlcanzado` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L850) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `actualizarErroresTrayectoria`, `endpointAceptable`, `errorAng360` | — |
| `__anon3dcaec680111.mediana4` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L857) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `detectarOutliers` | `medianaCuatro` | — |
| `__anon3dcaec680111.hayPorLado` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L860) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | `fuentesPorLadoValidas` | — |
| `__anon3dcaec680111.promedioLado` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L861) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAntiFriccion`, `controlarAvance` | `promedioConfiableLado` | — |
| `__anon3dcaec680111.estimarTicksAvance` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L864) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAsentamientoFinal`, `controlarAvance` | `mediaEncodersSaludables` | — |
| `__anon3dcaec680111.resetConfEncoders` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L869) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `iniciarAvance` | `resetFiltrosEncoder` | — |
| `__anon3dcaec680111.iniciarAvance` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L877) | 8 | ESP32 / tiempo real | Bajo; interno; síncrona | `completarPausaReeval`, `controlarPausaPreAvance`, `controlarPausaPreGiro`, `iniciarRecuperacionEndpoint` | `actualizarErroresTrayectoria`, `copiarBase`, `distanciaAlObjetivo`, `establecerLimiteContinuoPwm`, `getX`, `getY`, `reiniciarControlRumbo`, `resetConfEncoders`, `sensar` | — |
| `__anon3dcaec680111.detectarOutliers` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L930) | 12 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `encoderEsOutlier`, `errorAng360`, `mediana4` | — |
| `__anon3dcaec680111.iniciarPausaReeval` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L949) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `frenarMotores`, `reiniciarControlRumbo` | — |
| `__anon3dcaec680111.completarPausaReeval` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L958) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | `clasificarEncoders`, `fallo`, `iniciarAvance` | — |
| `__anon3dcaec680111.pwmAntiFriccion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L975) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAntiFriccion`, `iniciarAntiFriccion` | `nivelAntiFriccion8Bit` | — |
| `__anon3dcaec680111.iniciarAntiFriccion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L981) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | `copiarBase`, `frenarMotores`, `pwmAntiFriccion` | — |
| `__anon3dcaec680111.controlarAntiFriccion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L993) | 10 | ESP32 / tiempo real | Alto; interno; síncrona | `controlarAvance` | `aplicarVelocidades`, `copiarBase`, `deltas`, `fallo`, `frenarMotores`, `movimientoAntiFriccionConfirmado`, `promedioLado`, `pwmAntiFriccion` | — |
| `__anon3dcaec680111.controlarAvance` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1049) | 69 | ESP32 / tiempo real | Alto; interno; síncrona | `controlarMovimiento` | `actualizarErroresTrayectoria`, `actualizarPI`, `anguloAlObjetivoRad`, `aplicarVelocidades`, `aproximar`, `completarPausaReeval`, `controlarAntiFriccion`, `correccionLateralParaDireccion`, `correccionLateralRumboDeg`, `deltas`, `distanciaAlObjetivo`, `distanciaFrenoPrevista`, `distanciaPorTick`, `errorAng360`, `estimarTicksAvance`, `evaluarLazoAngular`, `fallo`, `fin`, `frenarLadoIzquierdoParaRumbo`, `frenarMotores`, `frenarMotoresActivo`, `getLadoDer`, `getLadoIzq`, `hayPorLado`, `iniciarAntiFriccion`, `iniciarAsentamientoFinal`, `iniciarBaseGiro`, `normalizar360`, `promedioLado`, `registrarMotivoFinalizacion`, `rumboCuerpoParaTrayecto`, `sensar` | — |
| `__anon3dcaec680111.iniciarAsentamientoFinal` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1345) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | `copiarBase`, `establecerLimiteContinuoPwm`, `frenarMotoresActivo`, `sensar` | — |
| `__anon3dcaec680111.controlarAsentamientoFinal` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1360) | 8 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarMovimiento` | `actualizarErroresTrayectoria`, `deltas`, `distanciaPorTick`, `estimarTicksAvance`, `fallo`, `frenarMotoresActivo`, `sensar` | — |
| `__anon3dcaec680111.iniciarVerificacionFinal` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1396) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `completarGiro`, `controlarMovimiento` | `frenarMotores` | — |
| `__anon3dcaec680111.iniciarRecuperacionEndpoint` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1403) | 9 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `actualizarErroresTrayectoria`, `anguloAlObjetivoRad`, `completarPasoConCorreccionPendiente`, `decidirEndpointSeguro`, `distanciaAlObjetivo`, `errorAng360`, `fallo`, `iniciarAvance`, `iniciarBaseGiro`, `normalizar360`, `reversaAutomatica`, `rumboCuerpoParaTrayecto` | — |
| `__anon3dcaec680111.verificarObjetivoFinal` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1457) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarMovimiento` | `completarPaso`, `errorAng360`, `frenarMotores`, `iniciarBaseGiro`, `sensar` | — |
| `__anon3dcaec680111.completarPaso` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1473) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `verificarObjetivoFinal` | `fin` | — |
| `__anon3dcaec680111.completarPasoConCorreccionPendiente` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1479) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `iniciarRecuperacionEndpoint` | `fin` | — |
| `__anon3dcaec680111.iniciarPausaPreGiro` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1490) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `iniciarPasoInterno` | `frenarMotores` | — |
| `__anon3dcaec680111.controlarPausaPreGiro` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1497) | 4 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarMovimiento` | `errorAng360`, `frenarMotores`, `iniciarAvance`, `iniciarBaseGiro`, `sensar` | — |
| `__anon3dcaec680111.iniciarPausaPreAvance` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1514) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `completarGiro`, `iniciarPasoInterno` | `frenarMotores` | — |
| `__anon3dcaec680111.controlarPausaPreAvance` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1522) | 4 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarMovimiento` | `errorAng360`, `frenarMotores`, `iniciarAvance`, `iniciarBaseGiro`, `sensar` | — |
| `__anon3dcaec680111.iniciarPasoInterno` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1538) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `iniciarPaso` | `errorAng360`, `iniciarPausaPreAvance`, `iniciarPausaPreGiro` | — |
| `normalizar360` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1551) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `calCuenta`, `calTorque`, `controlarAvance`, `controlarCalibracion`, `errorAngularDeg`, `iniciarGiroAbsoluto`, `iniciarPaso`, `iniciarRecuperacionEndpoint`, `rumboCuerpoParaTrayecto` | — | — |
| `reiniciarControlRumbo` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1556) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `auditarSalud`, `cancelarMovimiento`, `fin`, `forzarEStop`, `iniciarAvance`, `iniciarBaseGiro`, `iniciarPausaReeval`, `resetFallo` | — | — |
| `registrarMotivoFinalizacion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1567) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `auditarSalud`, `cancelarMovimiento`, `controlarAvance`, `fin`, `forzarEStop`, `iniciarCalibracion`, `iniciarGiroAbsoluto`, `iniciarPaso`, `resetFallo`, `setup` | — | — |
| `enFaseAvance` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1571) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `auditarSalud` | — | — |
| `enFaseTraslacion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1572) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `enFaseGiro` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1575) | 6 | ESP32 / tiempo real | Bajo; interno; síncrona | `auditarSalud` | — | — |
| `enFaseCalibracion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1580) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `auditarSalud` | — | — |
| `obtenerDiagnosticoCalibracion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1583) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `iniciarCalibracion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1589) | 6 | ESP32 / tiempo real | Bajo; interno; evento | `procesarComandos` | `encolarEvento`, `establecerLimiteContinuoPwm`, `registrarMotivoFinalizacion`, `reiniciarDiagnosticoCalibracion`, `sensar` | — |
| `aplicarCalibracionInyectada` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1608) | 19 | ESP32 / tiempo real | Bajo; interno; evento | `intentarAutoRestaurarCalibracion`, `procesarComandos` | `encolarEvento`, `frenarMotores`, `obtenerCalibracionVigente`, `reset`, `resetOrientacionIMU`, `sensar`, `solicitarGuardarTorque` | — |
| `intentarAutoRestaurarCalibracion` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1654) | 7 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `aplicarCalibracionInyectada`, `obtenerCalibracionVigente`, `sensar` | — |
| `iniciarPaso` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1664) | 27 | ESP32 / tiempo real | Bajo; interno; evento | `procesarComandos` | `actualizarErroresTrayectoria`, `encolarEvento`, `getX`, `getY`, `iniciarPasoInterno`, `normalizar360`, `registrarMotivoFinalizacion`, `reversaAutomatica`, `rumboCuerpoParaTrayecto` | estado |
| `iniciarGiroAbsoluto` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1764) | 5 | ESP32 / tiempo real | Bajo; interno; evento | `procesarComandos` | `encolarEvento`, `iniciarBaseGiro`, `normalizar360`, `registrarMotivoFinalizacion` | estado |
| `cancelarMovimiento` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1793) | 6 | ESP32 / tiempo real | Medio; interno; evento | `procesarComandos` | `encolarEvento`, `frenarMotores`, `registrarMotivoFinalizacion`, `reiniciarControlRumbo`, `stopDebePreservarFallo` | parada/cierre |
| `controlarMovimiento` | [`src/Cinematica.cpp`](../../src/Cinematica.cpp#L1815) | 24 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `controlarAsentamientoFinal`, `controlarAvance`, `controlarCalibracion`, `controlarGiro`, `controlarPausaPreAvance`, `controlarPausaPreGiro`, `iniciarVerificacionFinal`, `verificarObjetivoFinal` | — |
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
| `contenidoSinNul` | [`src/Estado.cpp`](../../src/Estado.cpp#L13) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `cadenaConfiguracionValida` | — | — |
| `cadenaConfiguracionValida` | [`src/Estado.cpp`](../../src/Estado.cpp#L19) | 3 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `contenidoSinNul` | — |
| `encolarEvento` | [`src/Eventos.cpp`](../../src/Eventos.cpp#L6) | 20 | ESP32 / tiempo real | Bajo; interno; cola/evento | `aplicarCalibracionInyectada`, `auditarSalud`, `cancelarMovimiento`, `fin`, `iniciarCalibracion`, `iniciarGiroAbsoluto`, `iniciarPaso`, `procesarComandos` | — | — |
| `__anonea91e7c40111.copiarEstado` | [`src/MemoriaTorque.cpp`](../../src/MemoriaTorque.cpp#L20) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `procesarPersistenciaTorque`, `setup_MemoriaTorque`, `solicitarGuardarTorque` | — | estado |
| `__anonea91e7c40111.recalcularDiagnostico` | [`src/MemoriaTorque.cpp`](../../src/MemoriaTorque.cpp#L25) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `procesarPersistenciaTorque`, `setup_MemoriaTorque`, `solicitarGuardarTorque` | `baseDesdePromedio`, `promedioDireccion` | — |
| `__anonea91e7c40111.leerArchivo` | [`src/MemoriaTorque.cpp`](../../src/MemoriaTorque.cpp#L38) | 13 | ESP32 / tiempo real | Bajo; interno; síncrona | `guardarAtomico`, `setup_MemoriaTorque` | `agregar`, `close`, `registroValido` | — |
| `__anonea91e7c40111.escribirArchivo` | [`src/MemoriaTorque.cpp`](../../src/MemoriaTorque.cpp#L71) | 4 | ESP32 / tiempo real | Bajo; interno; síncrona | `guardarAtomico` | `close` | — |
| `__anonea91e7c40111.guardarAtomico` | [`src/MemoriaTorque.cpp`](../../src/MemoriaTorque.cpp#L86) | 8 | ESP32 / tiempo real | Bajo; interno; síncrona | `procesarPersistenciaTorque` | `escribirArchivo`, `iguales`, `leerArchivo` | — |
| `setup_MemoriaTorque` | [`src/MemoriaTorque.cpp`](../../src/MemoriaTorque.cpp#L106) | 7 | ESP32 / tiempo real | Bajo; interno; síncrona | `setup` | `copiarEstado`, `leerArchivo`, `recalcularDiagnostico` | — |
| `baseTorqueParaPolaridad8` | [`src/MemoriaTorque.cpp`](../../src/MemoriaTorque.cpp#L136) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `baseParaPolaridad` | — |
| `solicitarGuardarTorque` | [`src/MemoriaTorque.cpp`](../../src/MemoriaTorque.cpp#L143) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `aplicarCalibracionInyectada`, `completarGiro`, `controlarGiro` | `agregar`, `copiarEstado`, `recalcularDiagnostico`, `registroValido` | — |
| `procesarPersistenciaTorque` | [`src/MemoriaTorque.cpp`](../../src/MemoriaTorque.cpp#L161) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `Task_Web` | `copiarEstado`, `guardarAtomico`, `recalcularDiagnostico` | — |
| `persistenciaTorquePendiente` | [`src/MemoriaTorque.cpp`](../../src/MemoriaTorque.cpp#L176) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `obtenerDiagnosticoMemoriaTorque` | [`src/MemoriaTorque.cpp`](../../src/MemoriaTorque.cpp#L183) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `obtenerCalibracionVigente` | [`src/MemoriaTorque.cpp`](../../src/MemoriaTorque.cpp#L190) | 6 | ESP32 / tiempo real | Bajo; interno; síncrona | `aplicarCalibracionInyectada`, `intentarAutoRestaurarCalibracion` | — | — |
| `validarMapaMotores` | [`src/Motores.cpp`](../../src/Motores.cpp#L108) | 9 | ESP32 / tiempo real | Bajo; interno; síncrona | `aplicarVelocidades`, `setup_MotorPinsLow`, `setup_Motores` | — | — |
| `establecerLimiteContinuoPwm` | [`src/Motores.cpp`](../../src/Motores.cpp#L205) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `iniciarAsentamientoFinal`, `iniciarAvance`, `iniciarBaseGiro`, `iniciarCalibracion` | — | — |
| `limiteContinuoPwm` | [`src/Motores.cpp`](../../src/Motores.cpp#L209) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `aplicarVelocidades` | [`src/Motores.cpp`](../../src/Motores.cpp#L211) | 17 | ESP32 / tiempo real | Alto; interno; síncrona | `calTorque`, `controlarAntiFriccion`, `controlarAvance`, `controlarGiro` | `getDeadbandDer8`, `getDeadbandIzq8`, `motoresListos`, `validarMapaMotores` | — |
| `frenarMotores` | [`src/Motores.cpp`](../../src/Motores.cpp#L274) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `aplicarCalibracionInyectada`, `auditarSalud`, `calTorque`, `cancelarMovimiento`, `completarGiro`, `controlarAntiFriccion`, `controlarAvance`, `controlarCalibracion`, `controlarGiro`, `controlarPausaPreAvance`, `controlarPausaPreGiro`, `fin`, `forzarEStop`, `iniciarAntiFriccion`, `iniciarPausaPreAvance`, `iniciarPausaPreGiro`, `iniciarPausaReeval`, `iniciarVerificacionFinal`, `reintentarGiro`, `resetFallo`, `setup`, `setup_Motores`, `verificarObjetivoFinal` | `reiniciar` | — |
| `frenarMotoresActivo` | [`src/Motores.cpp`](../../src/Motores.cpp#L288) | 3 | ESP32 / tiempo real | Alto; interno; síncrona | `controlarAsentamientoFinal`, `controlarAvance`, `controlarGiro`, `iniciarAsentamientoFinal` | — | — |
| `actualizarFrenoActivo` | [`src/Motores.cpp`](../../src/Motores.cpp#L306) | 3 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `frenoActivoEnCurso` | [`src/Motores.cpp`](../../src/Motores.cpp#L315) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `validarInterlockMotores` | [`src/Motores.cpp`](../../src/Motores.cpp#L319) | 24 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | estado |
| `estadoInterlockL` | [`src/Motores.cpp`](../../src/Motores.cpp#L365) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | estado |
| `estadoInterlockR` | [`src/Motores.cpp`](../../src/Motores.cpp#L366) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | estado |
| `signoEnergizadoL` | [`src/Motores.cpp`](../../src/Motores.cpp#L367) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `signoEnergizadoR` | [`src/Motores.cpp`](../../src/Motores.cpp#L368) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `signoPendienteL` | [`src/Motores.cpp`](../../src/Motores.cpp#L369) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `signoPendienteR` | [`src/Motores.cpp`](../../src/Motores.cpp#L370) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `motoresListos` | [`src/Motores.cpp`](../../src/Motores.cpp#L372) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `aplicarVelocidades`, `resetFallo` | — | — |
| `estadoMotores` | [`src/Motores.cpp`](../../src/Motores.cpp#L374) | 4 | ESP32 / tiempo real | Bajo; interno; síncrona | `setup` | — | — |
| `setup_MotorPinsLow` | [`src/Motores.cpp`](../../src/Motores.cpp#L383) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `setup` | `validarMapaMotores` | — |
| `setup_Motores` | [`src/Motores.cpp`](../../src/Motores.cpp#L395) | 4 | ESP32 / tiempo real | Alto; interno; síncrona | `setup` | `frenarMotores`, `validarMapaMotores` | — |
| `PoseEstimator.PoseEstimator` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L10) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `reset` | — |
| `PoseEstimator.inicializar` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L14) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `setup` | — | — |
| `PoseEstimator.reset` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L18) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `PoseEstimator`, `aplicarCalibracionInyectada`, `completarGiro`, `procesarComandos` | `iniciarMedicionTraslacionGiro` | — |
| `PoseEstimator.actualizarOdometria` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L30) | 6 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `promediosConfiableAcotados` | — |
| `PoseEstimator.iniciarMedicionTraslacionGiro` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L68) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `iniciarBaseGiro`, `reset` | — | — |
| `PoseEstimator.aplicarCorreccionICR` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L74) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `corregirTraslacionParasitaGlobal` | — |
| `PoseEstimator.actualizarOrientacion` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L84) | 3 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.distanciaAlObjetivo` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L91) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance`, `iniciarAvance`, `iniciarRecuperacionEndpoint` | — | — |
| `PoseEstimator.anguloAlObjetivoRad` | [`src/PoseEstimator.cpp`](../../src/PoseEstimator.cpp#L97) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance`, `iniciarRecuperacionEndpoint` | — | — |
| `setup_Red` | [`src/Red.cpp`](../../src/Red.cpp#L602) | 4 | ESP32 / tiempo real | Medio; interno; asíncrona | `setup` | — | — |
| `procesarWebSockets` | [`src/Red.cpp`](../../src/Red.cpp#L616) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `Task_Web` | — | — |
| `pushTelemetria` | [`src/Red.cpp`](../../src/Red.cpp#L622) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `Task_Web` | — | — |
| `Seguridad.Seguridad` | [`src/Seguridad.cpp`](../../src/Seguridad.cpp#L11) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Seguridad.reiniciarSaludEncoders` | [`src/Seguridad.cpp`](../../src/Seguridad.cpp#L22) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Seguridad.prepararRevalidacionEncoders` | [`src/Seguridad.cpp`](../../src/Seguridad.cpp#L37) | 4 | ESP32 / tiempo real | Bajo; interno; síncrona | `resetFallo` | — | — |
| `Seguridad.aplicarClasificacionEncoders` | [`src/Seguridad.cpp`](../../src/Seguridad.cpp#L53) | 3 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Seguridad.actualizarSaludEncoders` | [`src/Seguridad.cpp`](../../src/Seguridad.cpp#L67) | 48 | ESP32 / tiempo real | Alto; sin llamada interna detectada; síncrona | — | `encoderSinRespuestaAislada`, `medianaCuatro`, `promedioConfiableLado` | — |
| `Seguridad.auditarSalud` | [`src/Seguridad.cpp`](../../src/Seguridad.cpp#L206) | 21 | ESP32 / tiempo real | Alto; sin llamada interna detectada; evento | — | `enFaseAvance`, `enFaseCalibracion`, `enFaseGiro`, `encolarEvento`, `frenarMotores`, `ladoEnStall`, `registrarMotivoFinalizacion`, `reiniciarControlRumbo` | parada/cierre |
| `Seguridad.forzarEStop` | [`src/Seguridad.cpp`](../../src/Seguridad.cpp#L290) | 1 | ESP32 / tiempo real | Medio; interno; síncrona | `procesarComandos` | `frenarMotores`, `registrarMotivoFinalizacion`, `reiniciarControlRumbo` | parada/cierre |
| `Seguridad.resetFallo` | [`src/Seguridad.cpp`](../../src/Seguridad.cpp#L300) | 6 | ESP32 / tiempo real | Medio; interno; síncrona | `procesarComandos` | `frenarMotores`, `motoresListos`, `pcntInicializados`, `prepararRevalidacionEncoders`, `registrarMotivoFinalizacion`, `reiniciarControlRumbo` | parada/cierre |
| `setup_Sensores` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L99) | 8 | ESP32 / tiempo real | Bajo; interno; síncrona | `setup` | `textoEtapa` | — |
| `establecerEscalaGiro` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L164) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `procesarComandos` | — | — |
| `obtenerEscalaGiro` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L169) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `pcntInicializados` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L173) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `procesarComandos`, `resetFallo`, `setup` | `todosListos` | — |
| `diagnosticoInicializacionPCNT` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L177) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `resetFiltrosEncoder` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L222) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `resetConfEncoders` | — | — |
| `resetOrientacionIMU` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L229) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `aplicarCalibracionInyectada`, `completarGiro`, `procesarComandos` | — | — |
| `recalibrarOffsetIMU` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L239) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `calCuenta` | — | — |
| `obtenerYawIMUDeg` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L269) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `recentrarYawIMUEnReposo` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L276) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `cantidadRecentradosYawIMU` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L293) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `leerSensoresSincrono` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L353) | 4 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `snapshotSensoresControl` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L380) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `loop` | — | — |
| `obtenerUltimoSnapshotSensores` | [`src/Sensores.cpp`](../../src/Sensores.cpp#L384) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `sensar` | — | — |
| `solicitarManualBegin` | [`src/main.cpp`](../../src/main.cpp#L36) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `solicitarManualEnd` | [`src/main.cpp`](../../src/main.cpp#L37) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `solicitarManualDesconexion` | [`src/main.cpp`](../../src/main.cpp#L38) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `solicitarDesconexionControl` | [`src/main.cpp`](../../src/main.cpp#L39) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `tomarDesconexionControl` | [`src/main.cpp`](../../src/main.cpp#L45) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `publicarManualDrive` | [`src/main.cpp`](../../src/main.cpp#L52) | 4 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `tomarSolicitudManual` | [`src/main.cpp`](../../src/main.cpp#L61) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `leerManualDrive` | [`src/main.cpp`](../../src/main.cpp#L65) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `limpiarManualDrive` | [`src/main.cpp`](../../src/main.cpp#L66) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `faseManualTexto` | [`src/main.cpp`](../../src/main.cpp#L73) | 3 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `inicioManualMs` | [`src/main.cpp`](../../src/main.cpp#L77) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `procesarComandos` | [`src/main.cpp`](../../src/main.cpp#L135) | 29 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; cola/evento | — | `aplicarCalibracionInyectada`, `cancelarMovimiento`, `encolarEvento`, `establecer`, `establecerEscalaGiro`, `estopSolicitado`, `forzarEStop`, `iniciarCalibracion`, `iniciarGiroAbsoluto`, `iniciarPaso`, `pcntInicializados`, `reset`, `resetFallo`, `resetOrientacionIMU` | cola de comandos |
| `Task_Web` | [`src/main.cpp`](../../src/main.cpp#L229) | 8 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `procesarPersistenciaTorque`, `procesarWebSockets`, `pushTelemetria`, `registrarStackLibre` | — |
| `setup` | [`src/main.cpp`](../../src/main.cpp#L305) | 5 | ESP32 / tiempo real | Alto; entrada/framework; cola/evento | — | `estadoMotores`, `frenarMotores`, `inicializar`, `inicializarDiagnosticoRTOS`, `pcntInicializados`, `registrarMotivoFinalizacion`, `registrarResultadoArquitectura`, `setup_MemoriaTorque`, `setup_MotorPinsLow`, `setup_Motores`, `setup_Red`, `setup_Sensores` | cola de comandos |
| `loop` | [`src/main.cpp`](../../src/main.cpp#L344) | 6 | ESP32 / tiempo real | Bajo; entrada/framework; síncrona | `init` | `registrarCicloControl`, `registrarStackLibre`, `snapshotSensoresControl` | — |
