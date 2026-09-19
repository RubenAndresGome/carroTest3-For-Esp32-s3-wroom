# UML funcional: `firmware/include`

Funciones detectadas: **283**. Tipos detectados: **40**.

## Grafo de llamadas

```mermaid
flowchart LR
  subgraph f0["Cinematica.h"]
    n3aa0313b9f["yawFaseDeltaDeg()"]
    n3bf9ee8c5e["yawFaseInicioDeg()"]
    n75cd180b40["pivotValidado()"]
    n8378ae414a["torqueDetectado()"]
    nf5f87e1f97["pwmLado8()"]
    ndae7a414f5["movimientoFaseMs()"]
    n76ba3d0e68["activa()"]
    nf5ce123982["pasoRampa()"]
    n0efa03d230["pasosRampaTotal()"]
    n59218b075f["pwmObjetivo()"]
    n3958adf576["candidatoDireccion()"]
    n5e2b527190["deltaEncoders()"]
    ne5526dfff1["promedioLados()"]
    n518f7a91a5["ladosValidos()"]
    n59aee03c76["encoderResponde()"]
    n8a794b6817["encoderAislado()"]
    nb4cce1e86b["stallAcumuladoMs()"]
    ne50a5718ec["giroDetectadoSinEncoders()"]
    n1e4339fc62["silencioEncodersMs()"]
    nfa445bfc9d["limiteSilencioEncodersMs()"]
    n5ddd68d439["gyroConfirmado()"]
    nb379fd2fac["pcntCorroborado()"]
    n8bd3150f8a["encodersQueResponden()"]
    ncf920954e5["respuestaFaseA()"]
    n36c0dd10ad["respuestaFaseB()"]
    n03b2658a4d["yawInicioDeg()"]
    n6d636fed1a["yawActualDeg()"]
    nf08dfb4888["errorRetornoDeg()"]
    nc518a8ef5b["retornoWatchdogArmado()"]
    n8251a83c20["retornoSinProgresoMs()"]
    n653fe65d9c["ventanaMaxPwmMs()"]
    n346f7da703["pwmBase8()"]
    n70727c7784["pwmLogicoIzquierdo()"]
    n22a162a2e3["pwmLogicoDerecho()"]
    n93d1ca7b60["pwmPuenteIzquierdo()"]
    n0575fe31eb["pwmPuenteDerecho()"]
    n69f4702a80["espera()"]
  end
  subgraph f1["Comandos.h"]
    n16236834e2["throttle()"]
    n874ac541cf["steering()"]
    n11fbb607cf["stream()"]
    n25bd93d145["frame()"]
    na4b210d593["recibidoMs()"]
    n3db668c698["tipo()"]
    n1cd885e859["seq()"]
    na8eb70fef4["heading()"]
    n97a01c41aa["distanciaCm()"]
    ncaefa53d86["factor()"]
    nca386029a6["trimIzq()"]
    nb32bf06d5d["trimDer()"]
    n74fd32b5f0["deadbandIzq8()"]
    n3e920dc1e3["deadbandDer8()"]
    nfab86333b8["targetXCm()"]
    nd19be5daa9["targetYCm()"]
    nd0487d22ad["tieneObjetivoAbsoluto()"]
    ne10be54664["modoPaso()"]
    n365bffe1da["pwmPositivo8()"]
    n0e6d16d762["pwmNegativo8()"]
    n153d4f89d3["polaridadPositiva()"]
    n46d714dfc5["polaridadNegativa()"]
    n16b17b0977["icrXCm()"]
    n0855e222c3["icrYCm()"]
    ncc27e3c8ff["gyroScale()"]
  end
  subgraph f2["ControlAngular.h"]
    nf26244801b["modo()"]
    n87c7a8b205["modLadoIzq()"]
    n355f82f8c6["modLadoDer()"]
    nad4fc9b0ac["solicitarPivote()"]
    ndbb55711d8["limitarFactor(float valor,float minimo,float maximo)"]
    n4c4985c55c["evaluarLazoAngular(float errorAngDeg,float restanteDistCm,uint32_t msEnError,float umbralContinuoDeg,float umbralTransitorioDeg,float distanciaCierreCm,uint32_t msHisteresis)"]
  end
  subgraph f3["ControlCalibracion.h"]
    n8e5cdcf64e["derecha()"]
    n842f94a38a["izquierda()"]
    n8496a04154["bilateral()"]
    ndccebecfc6["equilibrado()"]
    n4608cb7d79["torque()"]
    n105ab83cda["evaluarPivot(float yawDelta,const int64_t d[4])"]
    n7a8a83a568["movimiento()"]
    n2239ddf9fe["inicioMs()"]
    nccd5c1b580["ventanaMs()"]
    n47973578b8["ventanasAsimetricas()"]
    nd45c19608e["anterior()"]
    nd974be94a5["vigilarPivot(VigilanciaPivot & v,const EvidenciaPivot & e,float yawDelta,const int64_t d[4],uint32_t ahora)"]
    n21704e3450["promedioIzquierdo()"]
    n7dd95b19b3["promedioDerecho()"]
    n485bbb3772["ladoIzquierdoValido()"]
    nbbd6826507["ladoDerechoValido()"]
    n135223fafb["responde()"]
    n1fb504f91f["sinRespuestaAislada()"]
    nfc6bae17a5["activa()"]
    n5d501cf5a9["inicioMs()"]
    ne9cbf2c806["transcurridoMs()"]
    n8bc79b0ab0["gyroConfirmado()"]
    n2cb40b78ba["pcntCorroborado()"]
    nb3ceea2b4c["encodersQueResponden()"]
    n5500dff122["confirmada() const"]
    n6a825a8498["izquierda()"]
    n1a77d8793b["derecha()"]
    n81a8c5698b["comandoPivot(int candidato,int pwm)"]
    n0a4f2e1608["evaluarMovimiento(float gyroAbsRadS,float gyroMinimoRadS,const int64_t deltas[4],int64_t ticksMinimos)"]
    n8b5b63f455["actualizarSilencioConGiro(VigilanciaSilencioEncoders & vigilancia,bool giroConfirmado,const int64_t deltas[4],uint32_t ahoraMs,uint32_t limiteMs)"]
    n5dcfbf291a["reiniciarVigilanciaRetorno(float errorAbsDeg,uint32_t ahoraMs,float & referenciaErrorDeg,uint32_t & ultimoProgresoMs)"]
    nea6f15cfa7["retornoSinProgreso(float errorAbsDeg,uint32_t ahoraMs,float progresoMinimoDeg,uint32_t limiteMs,float & referenciaErrorDeg,uint32_t & ultimoProgresoMs)"]
    n9715276ede["evaluarEncoders(const int64_t deltas[4],int64_t ticksMinimos,const bool confiable[4]=nullptr)"]
    n5e5431ed38["totalPasosRampa(int inicio,int fin,int paso)"]
    n3f6bbe365f["pasoRampaActual(int pwm,int inicio,int fin,int paso)"]
  end
  subgraph f4["ControlCompensacion.h"]
    nb19b6b0733["trimIzq()"]
    nf74c4bc52a["trimDer()"]
    n060e89c1a9["deadbandIzq8()"]
    n62db5a8b1f["deadbandDer8()"]
    n57b56ba33b["aplicarPisoDeadband(int vel,int deadband,int umbralCrucero)"]
    ne9a5ae9234["limitarTrim(float valor)"]
    n5aff059ec7["limitarDeadband8(int valor8)"]
    n6bb5d261e7["reiniciar()"]
    n7fb6d0c102["agregar(const Muestra & muestra)"]
    nea7867a151["establecer(float trimIzq,float trimDer,int deadbandIzq8,int deadbandDer8)"]
    n29ed41e68e["getLadoIzq() const"]
    n73d8341834["getLadoDer() const"]
    nde2ac992b1["getProm() const"]
    nf3b4fd7db5["getDeadbandIzq8() const"]
    n87364083ce["getDeadbandDer8() const"]
    n64775da460["getCantidad() const"]
    n5222fa26f5["trimIzq_()"]
    n276e2dd258["trimDer_()"]
    n6356359dfd["deadbandIzq8_()"]
    nb6e9e34d73["deadbandDer8_()"]
    n2b0f3afa1f["cantidad_()"]
  end
  subgraph f5["ControlConexion.h"]
    n32b5b0656f["accionAlPerderWebSocket(bool calibrando)"]
  end
  subgraph f6["ControlInicializacionPCNT.h"]
    ncd3a618bee["inicializado()"]
    na72e11f638["etapaFallida()"]
    nc7ccfb1756["codigoError()"]
    nc3ce7a9fb4["registrar(Canal & canal,Etapa etapa,int codigoError)"]
    n9934a6e3ba["todosListos(const Canal canales[4])"]
    n97f322c4ca["textoEtapa(Etapa etapa)"]
  end
  subgraph f7["ControlManual.h"]
    n3b4ac7c7e5["izquierdo()"]
    n720f87bc76["derecho()"]
    nd65f256f9d["mezclar(float throttle,float steering,int limite)"]
    n2082f92a61["leaseVigente(uint32_t ahoraMs,uint32_t recibidoMs,uint32_t limiteMs)"]
    n2ee2698e9d["esperaPrimerFrameVigente(uint32_t ahoraMs,uint32_t inicioMs,uint32_t graciaMs)"]
    n3b720fa805["acercar(int actual,int objetivo,int paso)"]
  end
  subgraph f8["ControlRuta.h"]
    naf5b9be116["longitudinalCm()"]
    n7d8fcdccb4["lateralCm()"]
    n56c5928aa3["euclidianoCm()"]
    n443a3bfa6b["integralGradoS()"]
    nd35097c3a5["p()"]
    n7314fc80d8["i()"]
    nbb3bcd73ea["d()"]
    n05c7a68a3c["total()"]
    n2e3822c0b7["integralGradoS()"]
    nfd68ebbf51["limitar(float valor,float minimo,float maximo)"]
    n055e6556b4["normalizar360(float grados)"]
    n40c8cb2eef["errorAngularDeg(float objetivoDeg,float actualDeg)"]
    nbe93f15ded["reversaAutomatica(float rumboTrayectoDeg,float rumboActualDeg,float umbralDeg)"]
    nb3b8b073ec["rumboCuerpoParaTrayecto(float rumboTrayectoDeg,int direccion)"]
    n0fcdf5f124["correccionLateralParaDireccion(float correccionRumboDeg,int)"]
    n83ccfc8b5e["frenarLadoIzquierdoParaRumbo(int candidatoGiro,int direccion)"]
    n0d890751b6["distanciaPorTick(float diametroEfectivoCm,int pulsosPorRevolucion)"]
    neab0d37517["distanciaFrenoPrevista(float pwm,float baseCm,float cmPorPwm,float maximoCm)"]
    nde8d4a8d76["dxCm()"]
    n60d9a70d0c["dyCm()"]
    na9de66bbea["corregirTraslacionParasita(float xIcrCm,float yIcrCm,float deltaThetaRad)"]
    n563c096ba8["corregirTraslacionParasitaGlobal(float xIcrCm,float yIcrCm,float deltaThetaRad,float thetaRad)"]
    nc5ea7c7121["calcularErroresTrayectoriaAnclado(float posicionXCm,float posicionYCm,float origenXCm,float origenYCm,float objetivoXCm,float objetivoYCm,float rumboPlanificadoDeg,float distanciaPlanificadaCm)"]
    n7f70feabff["calcularErroresTrayectoria(float posicionXCm,float posicionYCm,float objetivoXCm,float objetivoYCm,float rumboPlanificadoDeg,float distanciaPlanificadaCm)"]
    n38193be335["correccionLateralRumboDeg(float errorLateralCm,float gananciaDegPorCm,float limiteDeg)"]
    n17f772aa89["actualizarPI(EstadoPI & estado,float errorDeg,float gyroRadS,float dtS,float kp,float ki,float kd,float limitePwm,float limiteIntegralGradoS)"]
    nf4f7b06303["endpointAceptable(float errorLateralCm,float errorEuclidianoCm,float errorYawDeg,float toleranciaEndpointCm,float toleranciaYawDeg)"]
    n03b6c15711["agotoIntentosEndpoint(uint8_t intentosRealizados,uint8_t maximoIntentos)"]
    na93127da48["decidirEndpoint(bool objetivoAbsoluto,bool endpointAceptado,uint8_t intentosRealizados,uint8_t maximoIntentos)"]
    n6d56729432["decidirEndpointSeguro(bool objetivoAbsoluto,bool endpointAceptado,uint8_t intentosRealizados,uint8_t maximoIntentos,float distanciaErrorCm,float distanciaMinimaRecuperableCm)"]
    n4255c64c61["pwmL()"]
    n236174be0a["pwmR()"]
    n16b9ef9cdf["compensacionPwm()"]
    ne57a5f160d["desplazamientoCentroCm()"]
    nf41610dbe0["balancearGiroDiferencial(int pwmBase,int cand,float ticksIzq,float ticksDer,float cmPorTick,float kp,float limitePwm,int maxPwm)"]
  end
  subgraph f9["ControlSeguridad.h"]
    nc112944dbf["imuApta(bool presente,bool obsoleta)"]
    neefa84e65c["estopSolicitado(bool solicitado)"]
    na3d5c6af71["encoderEsOutlier(int64_t delta,float mediana,float desacuerdoMaximo)"]
    n43dfc22137["deltaEncoderPlausible(int64_t delta,int64_t maximoAbsoluto)"]
    n71c75d5282["saturarDeltaEncoder(int64_t delta,int64_t maximoAbsoluto)"]
    n6e70133094["confiable()"]
    n2962f5392b["mediana()"]
    n5cfbf8cf49["modoDegradado()"]
    n11d5054f75["ladoIzquierdoValido()"]
    n90886550c6["ladoDerechoValido()"]
    nff5db8414e["medianaCuatro(const int64_t valores[4])"]
    n8b24e6cd48["clasificarEncoders(const int64_t valores[4],float desacuerdoMaximo)"]
    n8abb50201e["promedioConfiableLado(const int64_t valores[4],const bool confiable[4],bool izquierdo)"]
    n6aa4f9cadc["acotarLadoFuenteUnica(float valorLado,float referenciaLadoOpuesto,float desacuerdoMaximo)"]
    n7841f9a8b6["izquierdo()"]
    nac3233cbca["derecho()"]
    nb47a4d8778["promediosConfiableAcotados(const int64_t valores[4],const bool confiable[4],float desacuerdoMaximo)"]
    nf3ce73e5b1["mediaEncodersSaludables(const int64_t valores[4],const bool confiable[4])"]
    nf6ec66e2e5["ladoEnStall(bool ladoExigido,bool pulsoFrontalCero,bool pulsoPosteriorCero)"]
    nea0febbbff["fuentesPorLadoValidas(const bool confiable[4])"]
    nafaa6c347f["encoderSinRespuestaAislada(int64_t delta,int64_t deltaPareja,bool ladoExigido)"]
    nf6dc87da2a["nivelAntiFriccion8Bit(uint8_t indice)"]
    ne5db5a99ce["movimientoAntiFriccionConfirmado(float deltaIzq,float deltaDer,float minimoTicks)"]
    n6217a6dfb0["stopDebePreservarFallo(bool fallo,bool estop)"]
    na8545d2dcb["menorErrorAbs()"]
    n604e35b998["inicioDivergenciaMs()"]
    na64ade02e7["reiniciar(float errorAbsInicial)"]
    n08ea8c7ec8["evaluarDivergenciaGiro(EstadoVigilanciaDivergenciaGiro & estado,float errorAbsActual,uint32_t ahoraMs,float umbralDivergenciaDeg,uint32_t tiempoLimiteMs,bool movimientoPresente)"]
  end
  subgraph f10["ControlTorque.h"]
    n08c8aea920["secuencia()"]
    na36ca36aa3["pwmPositivo8()"]
    n057516ee67["pwmNegativo8()"]
    n6d9ed5afd3["polaridadPositiva()"]
    nb21d97b544["polaridadNegativa()"]
    n0143eb9bcd["pwmPositivoDerecho8()"]
    n074e7c7422["pwmNegativoDerecho8()"]
    n2516e3ac06["Registro(uint32_t secuenciaValor=0,int pwmPositivoValor=0,int pwmNegativoValor=0,int polaridadPositivaValor=0,int polaridadNegativaValor=0)"]
    ne181a4f0e6["registros()"]
    n2443055d6c["cantidad()"]
    n5c214e136f["siguienteSecuencia()"]
    n2cc061f77d["registroValido(const Registro & registro)"]
    n3126729334["agregar(Historial & historial,Registro registro)"]
    n8407ecbc10["promedioDireccion(const Historial & historial,bool positiva,bool izquierda=true)"]
    nc27e7694b9["baseDesdePromedio(int promedio)"]
    nc44c21432c["baseParaPolaridad(const Historial & historial,int polaridad,bool izquierda=true)"]
    n7642828021["iguales(const Historial & primero,const Historial & segundo)"]
  end
  subgraph f11["Eventos.h"]
    nae6550dc08["tipo()"]
    na459c2b651["seq()"]
    nf13fbcc40f["run_id()"]
    n76ea421fa2["detalle()"]
    n62d9f5cb98["progreso()"]
  end
  subgraph f12["MemoriaTorque.h"]
    ndd532631c9["montada()"]
    nfc3d9c8795["cargada()"]
    n5f4447e40f["persistenciaPendiente()"]
    n79fb835f83["cantidad()"]
    nb8470830cf["promedioPositivo8()"]
    n5918a206e0["promedioNegativo8()"]
    n7ba6801160["promedioPositivoDerecho8()"]
    n1e15a19b52["promedioNegativoDerecho8()"]
    n5be541acbc["basePositiva8()"]
    nffd501f233["baseNegativa8()"]
    nc5cb624ee8["basePositivaDerecha8()"]
    n2774780dd0["baseNegativaDerecha8()"]
    nc498a4613a["estado()"]
  end
  subgraph f13["PoseEstimator.h"]
    naa9da578f8["getX() const"]
    naf8f41e631["getY() const"]
    n20980b6f30["getThetaRad() const"]
    nac36fa6663["getThetaDeg() const"]
    nff974756ad["getArcoCentroGiroCm() const"]
    n82db83bb77["getTraslacionGiroXCm() const"]
    n9d4e73c7ad["getTraslacionGiroYCm() const"]
    n3dbfe4d0d9["x_global()"]
    nc5a12c778c["y_global()"]
    necee4cf49d["theta_rad()"]
    nb315bb1a18["last_pulsos_FL()"]
    n762f3deb2f["last_pulsos_FR()"]
    n720bd38ab2["last_pulsos_BL()"]
    n1c88521f52["last_pulsos_BR()"]
    n0e2f0d781b["cm_por_pulso()"]
    nd0a347894e["ultimo_signo_l()"]
    na5d37f3dca["ultimo_signo_r()"]
    nbe33aaafd3["arco_centro_giro_cm()"]
    n264e386f24["traslacion_giro_x_cm()"]
    nc3757ed9ff["traslacion_giro_y_cm()"]
    n4618808b9b["DISTANCIA_EJES_CM()"]
  end
  subgraph f14["Seguridad.h"]
    nd82675d7a2["inicio_movimiento_ms()"]
    nab14b99593["pulsos_movimiento_iniciales()"]
    n2ef907a2ab["inicio_ventana_encoder_ms()"]
    nbdc37de407["pulsos_ventana_encoder()"]
    n907986a977["pulsos_lado_anteriores()"]
    n2fc7bb45f0["ultimo_progreso_lado_ms()"]
  end
  subgraph f15["Sensores.h"]
    n82657c9903["pulsosFL()"]
    n3075dc8d7a["pulsosFR()"]
    n1e0778db97["pulsosBL()"]
    n8c3a1c8af0["pulsosBR()"]
    ne2b0b8ee3a["delta_pulsos_filtrado_FL()"]
    ndb347ca2e8["delta_pulsos_filtrado_FR()"]
    na24e42f535["delta_pulsos_filtrado_BL()"]
    n5ecd3e147d["delta_pulsos_filtrado_BR()"]
    n179a7b7eec["velocidad_filtrada_L_cm_s()"]
    n489373e2c4["velocidad_filtrada_R_cm_s()"]
    nc75758f5a0["imu_deltaZ_rad()"]
    nadbe2b09d9["gyro_z_filtrado_rad_s()"]
    n769545aedf["gyro_z_offset_rad_s()"]
    n7792cac4c0["yaw_integrado_deg()"]
    n5cccdea709["timestamp_ms()"]
    n16b79c39c7["timestamp_us()"]
    ne5a2ed5fa5["mpu_present()"]
    nceefbeb6ce["mpu_stale()"]
    nbea7d2baa2["mpu_calibrated()"]
    ndce4385538["fuente5vOk()"]
    n8129f4b27b["sequence()"]
  end
  n17f772aa89 --> nfd68ebbf51
  n2516e3ac06 --> n0143eb9bcd
  n2516e3ac06 --> n074e7c7422
  n2516e3ac06 --> n08c8aea920
  n2516e3ac06 --> n0e6d16d762
  n2516e3ac06 --> n153d4f89d3
  n2516e3ac06 --> n365bffe1da
  n2516e3ac06 --> n46d714dfc5
  n3126729334 --> n2cc061f77d
  n38193be335 --> nfd68ebbf51
  n3f6bbe365f --> n5e5431ed38
  n40c8cb2eef --> n055e6556b4
  n4c4985c55c --> ndbb55711d8
  n563c096ba8 --> na9de66bbea
  n6d56729432 --> na93127da48
  n7f70feabff --> nc5ea7c7121
  n7fb6d0c102 --> n5aff059ec7
  n7fb6d0c102 --> ne9a5ae9234
  n8b24e6cd48 --> na3d5c6af71
  n8b24e6cd48 --> nff5db8414e
  n9715276ede --> n8abb50201e
  na93127da48 --> n03b6c15711
  nb3b8b073ec --> n055e6556b4
  nb47a4d8778 --> n6aa4f9cadc
  nb47a4d8778 --> n8abb50201e
  nbe93f15ded --> n40c8cb2eef
  nc44c21432c --> nc27e7694b9
  nd974be94a5 --> n105ab83cda
  nea7867a151 --> n5aff059ec7
  nea7867a151 --> ne9a5ae9234
  neab0d37517 --> nfd68ebbf51
  nf41610dbe0 --> nfd68ebbf51
  classDef alto fill:#5b1f2a,stroke:#ff7a7a,color:#fff
  classDef medio fill:#4a3717,stroke:#ffca67,color:#fff
  classDef bajo fill:#123b3a,stroke:#39e6aa,color:#fff
  class n3aa0313b9f bajo
  class n3bf9ee8c5e bajo
  class n75cd180b40 bajo
  class n8378ae414a bajo
  class nf5f87e1f97 bajo
  class ndae7a414f5 bajo
  class n76ba3d0e68 bajo
  class nf5ce123982 bajo
  class n0efa03d230 bajo
  class n59218b075f bajo
  class n3958adf576 bajo
  class n5e2b527190 bajo
  class ne5526dfff1 bajo
  class n518f7a91a5 bajo
  class n59aee03c76 bajo
  class n8a794b6817 bajo
  class nb4cce1e86b bajo
  class ne50a5718ec bajo
  class n1e4339fc62 bajo
  class nfa445bfc9d bajo
  class n5ddd68d439 bajo
  class nb379fd2fac bajo
  class n8bd3150f8a bajo
  class ncf920954e5 bajo
  class n36c0dd10ad bajo
  class n03b2658a4d bajo
  class n6d636fed1a bajo
  class nf08dfb4888 bajo
  class nc518a8ef5b bajo
  class n8251a83c20 bajo
  class n653fe65d9c bajo
  class n346f7da703 bajo
  class n70727c7784 bajo
  class n22a162a2e3 bajo
  class n93d1ca7b60 bajo
  class n0575fe31eb bajo
  class n69f4702a80 bajo
  class n16236834e2 bajo
  class n874ac541cf bajo
  class n11fbb607cf bajo
  class n25bd93d145 bajo
  class na4b210d593 bajo
  class n3db668c698 bajo
  class n1cd885e859 bajo
  class na8eb70fef4 bajo
  class n97a01c41aa bajo
  class ncaefa53d86 bajo
  class nca386029a6 bajo
  class nb32bf06d5d bajo
  class n74fd32b5f0 bajo
  class n3e920dc1e3 bajo
  class nfab86333b8 bajo
  class nd19be5daa9 bajo
  class nd0487d22ad bajo
  class ne10be54664 bajo
  class n365bffe1da bajo
  class n0e6d16d762 bajo
  class n153d4f89d3 bajo
  class n46d714dfc5 bajo
  class n16b17b0977 bajo
  class n0855e222c3 bajo
  class ncc27e3c8ff bajo
  class nf26244801b bajo
  class n87c7a8b205 bajo
  class n355f82f8c6 bajo
  class nad4fc9b0ac bajo
  class ndbb55711d8 bajo
  class n4c4985c55c bajo
  class n8e5cdcf64e bajo
  class n842f94a38a bajo
  class n8496a04154 bajo
  class ndccebecfc6 bajo
  class n4608cb7d79 bajo
  class n105ab83cda bajo
  class n7a8a83a568 bajo
  class n2239ddf9fe bajo
  class nccd5c1b580 bajo
  class n47973578b8 bajo
  class nd45c19608e bajo
  class nd974be94a5 bajo
  class n21704e3450 bajo
  class n7dd95b19b3 bajo
  class n485bbb3772 bajo
  class nbbd6826507 bajo
  class n135223fafb bajo
  class n1fb504f91f bajo
  class nfc6bae17a5 bajo
  class n5d501cf5a9 bajo
  class ne9cbf2c806 bajo
  class n8bc79b0ab0 bajo
  class n2cb40b78ba bajo
  class nb3ceea2b4c bajo
  class n5500dff122 bajo
  class n6a825a8498 bajo
  class n1a77d8793b bajo
  class n81a8c5698b bajo
  class n0a4f2e1608 bajo
  class n8b5b63f455 bajo
  class n5dcfbf291a bajo
  class nea6f15cfa7 bajo
  class n9715276ede bajo
  class n5e5431ed38 bajo
  class n3f6bbe365f bajo
  class nb19b6b0733 bajo
  class nf74c4bc52a bajo
  class n060e89c1a9 bajo
  class n62db5a8b1f bajo
  class n57b56ba33b bajo
  class ne9a5ae9234 bajo
  class n5aff059ec7 bajo
  class n6bb5d261e7 bajo
  class n7fb6d0c102 bajo
  class nea7867a151 bajo
  class n29ed41e68e bajo
  class n73d8341834 bajo
  class nde2ac992b1 bajo
  class nf3b4fd7db5 bajo
  class n87364083ce bajo
  class n64775da460 bajo
  class n5222fa26f5 bajo
  class n276e2dd258 bajo
  class n6356359dfd bajo
  class nb6e9e34d73 bajo
  class n2b0f3afa1f bajo
  class n32b5b0656f bajo
  class ncd3a618bee bajo
  class na72e11f638 bajo
  class nc7ccfb1756 bajo
  class nc3ce7a9fb4 bajo
  class n9934a6e3ba bajo
  class n97f322c4ca bajo
  class n3b4ac7c7e5 bajo
  class n720f87bc76 bajo
  class nd65f256f9d bajo
  class n2082f92a61 bajo
  class n2ee2698e9d bajo
  class n3b720fa805 bajo
  class naf5b9be116 bajo
  class n7d8fcdccb4 bajo
  class n56c5928aa3 bajo
  class n443a3bfa6b bajo
  class nd35097c3a5 bajo
  class n7314fc80d8 bajo
  class nbb3bcd73ea bajo
  class n05c7a68a3c bajo
  class n2e3822c0b7 bajo
  class nfd68ebbf51 bajo
  class n055e6556b4 bajo
  class n40c8cb2eef bajo
  class nbe93f15ded bajo
  class nb3b8b073ec bajo
  class n0fcdf5f124 bajo
  class n83ccfc8b5e bajo
  class n0d890751b6 bajo
  class neab0d37517 bajo
  class nde8d4a8d76 bajo
  class n60d9a70d0c bajo
  class na9de66bbea bajo
  class n563c096ba8 bajo
  class nc5ea7c7121 bajo
  class n7f70feabff bajo
  class n38193be335 bajo
  class n17f772aa89 bajo
  class nf4f7b06303 bajo
  class n03b6c15711 bajo
  class na93127da48 bajo
  class n6d56729432 bajo
  class n4255c64c61 bajo
  class n236174be0a bajo
  class n16b9ef9cdf bajo
  class ne57a5f160d bajo
  class nf41610dbe0 bajo
  class nc112944dbf bajo
  class neefa84e65c bajo
  class na3d5c6af71 bajo
  class n43dfc22137 bajo
  class n71c75d5282 bajo
  class n6e70133094 bajo
  class n2962f5392b bajo
  class n5cfbf8cf49 bajo
  class n11d5054f75 bajo
  class n90886550c6 bajo
  class nff5db8414e bajo
  class n8b24e6cd48 alto
  class n8abb50201e bajo
  class n6aa4f9cadc bajo
  class n7841f9a8b6 bajo
  class nac3233cbca bajo
  class nb47a4d8778 bajo
  class nf3ce73e5b1 bajo
  class nf6ec66e2e5 bajo
  class nea0febbbff bajo
  class nafaa6c347f bajo
  class nf6dc87da2a bajo
  class ne5db5a99ce bajo
  class n6217a6dfb0 medio
  class na8545d2dcb bajo
  class n604e35b998 bajo
  class na64ade02e7 bajo
  class n08ea8c7ec8 bajo
  class n08c8aea920 bajo
  class na36ca36aa3 bajo
  class n057516ee67 bajo
  class n6d9ed5afd3 bajo
  class nb21d97b544 bajo
  class n0143eb9bcd bajo
  class n074e7c7422 bajo
  class n2516e3ac06 bajo
  class ne181a4f0e6 bajo
  class n2443055d6c bajo
  class n5c214e136f bajo
  class n2cc061f77d bajo
  class n3126729334 bajo
  class n8407ecbc10 bajo
  class nc27e7694b9 bajo
  class nc44c21432c bajo
  class n7642828021 bajo
  class nae6550dc08 bajo
  class na459c2b651 bajo
  class nf13fbcc40f bajo
  class n76ea421fa2 bajo
  class n62d9f5cb98 bajo
  class ndd532631c9 bajo
  class nfc3d9c8795 bajo
  class n5f4447e40f bajo
  class n79fb835f83 bajo
  class nb8470830cf bajo
  class n5918a206e0 bajo
  class n7ba6801160 bajo
  class n1e15a19b52 bajo
  class n5be541acbc bajo
  class nffd501f233 bajo
  class nc5cb624ee8 bajo
  class n2774780dd0 bajo
  class nc498a4613a bajo
  class naa9da578f8 bajo
  class naf8f41e631 bajo
  class n20980b6f30 bajo
  class nac36fa6663 bajo
  class nff974756ad bajo
  class n82db83bb77 bajo
  class n9d4e73c7ad bajo
  class n3dbfe4d0d9 bajo
  class nc5a12c778c bajo
  class necee4cf49d bajo
  class nb315bb1a18 bajo
  class n762f3deb2f bajo
  class n720bd38ab2 bajo
  class n1c88521f52 bajo
  class n0e2f0d781b bajo
  class nd0a347894e bajo
  class na5d37f3dca bajo
  class nbe33aaafd3 bajo
  class n264e386f24 bajo
  class nc3757ed9ff bajo
  class n4618808b9b bajo
  class nd82675d7a2 bajo
  class nab14b99593 bajo
  class n2ef907a2ab bajo
  class nbdc37de407 bajo
  class n907986a977 bajo
  class n2fc7bb45f0 bajo
  class n82657c9903 bajo
  class n3075dc8d7a bajo
  class n1e0778db97 bajo
  class n8c3a1c8af0 bajo
  class ne2b0b8ee3a bajo
  class ndb347ca2e8 bajo
  class na24e42f535 bajo
  class n5ecd3e147d bajo
  class n179a7b7eec bajo
  class n489373e2c4 bajo
  class nc75758f5a0 bajo
  class nadbe2b09d9 bajo
  class n769545aedf bajo
  class n7792cac4c0 bajo
  class n5cccdea709 bajo
  class n16b79c39c7 bajo
  class ne5a2ed5fa5 bajo
  class nceefbeb6ce bajo
  class nbea7d2baa2 bajo
  class ndce4385538 bajo
  class n8129f4b27b bajo
```

Fuentes: [Mermaid](mermaid/firmware_include.mmd) · [PlantUML](plantuml/firmware_include.puml). Las flechas continuas son llamadas síncronas; las discontinuas representan asincronía, eventos o colas. El color del nodo indica riesgo estático.

## Inventario

| Función | Archivo | CC | Propietario | Riesgo/estado | Entra desde | Sale hacia | Estado compartido |
|---|---|---:|---|---|---|---|---|
| `DiagnosticoCalibracion.yawFaseDeltaDeg` | [`include/Cinematica.h`](../../include/Cinematica.h#L9) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.yawFaseInicioDeg` | [`include/Cinematica.h`](../../include/Cinematica.h#L9) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.pivotValidado` | [`include/Cinematica.h`](../../include/Cinematica.h#L10) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.torqueDetectado` | [`include/Cinematica.h`](../../include/Cinematica.h#L10) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.pwmLado8` | [`include/Cinematica.h`](../../include/Cinematica.h#L11) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.movimientoFaseMs` | [`include/Cinematica.h`](../../include/Cinematica.h#L12) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.activa` | [`include/Cinematica.h`](../../include/Cinematica.h#L13) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.pasoRampa` | [`include/Cinematica.h`](../../include/Cinematica.h#L14) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.pasosRampaTotal` | [`include/Cinematica.h`](../../include/Cinematica.h#L15) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.pwmObjetivo` | [`include/Cinematica.h`](../../include/Cinematica.h#L16) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.candidatoDireccion` | [`include/Cinematica.h`](../../include/Cinematica.h#L17) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.deltaEncoders` | [`include/Cinematica.h`](../../include/Cinematica.h#L18) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.promedioLados` | [`include/Cinematica.h`](../../include/Cinematica.h#L19) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.ladosValidos` | [`include/Cinematica.h`](../../include/Cinematica.h#L20) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.encoderResponde` | [`include/Cinematica.h`](../../include/Cinematica.h#L21) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.encoderAislado` | [`include/Cinematica.h`](../../include/Cinematica.h#L22) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.stallAcumuladoMs` | [`include/Cinematica.h`](../../include/Cinematica.h#L23) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.giroDetectadoSinEncoders` | [`include/Cinematica.h`](../../include/Cinematica.h#L24) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.silencioEncodersMs` | [`include/Cinematica.h`](../../include/Cinematica.h#L25) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.limiteSilencioEncodersMs` | [`include/Cinematica.h`](../../include/Cinematica.h#L26) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.gyroConfirmado` | [`include/Cinematica.h`](../../include/Cinematica.h#L27) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.pcntCorroborado` | [`include/Cinematica.h`](../../include/Cinematica.h#L28) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.encodersQueResponden` | [`include/Cinematica.h`](../../include/Cinematica.h#L29) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.respuestaFaseA` | [`include/Cinematica.h`](../../include/Cinematica.h#L30) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.respuestaFaseB` | [`include/Cinematica.h`](../../include/Cinematica.h#L31) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.yawInicioDeg` | [`include/Cinematica.h`](../../include/Cinematica.h#L32) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.yawActualDeg` | [`include/Cinematica.h`](../../include/Cinematica.h#L33) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.errorRetornoDeg` | [`include/Cinematica.h`](../../include/Cinematica.h#L34) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.retornoWatchdogArmado` | [`include/Cinematica.h`](../../include/Cinematica.h#L35) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.retornoSinProgresoMs` | [`include/Cinematica.h`](../../include/Cinematica.h#L36) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.ventanaMaxPwmMs` | [`include/Cinematica.h`](../../include/Cinematica.h#L37) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.pwmBase8` | [`include/Cinematica.h`](../../include/Cinematica.h#L38) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.pwmLogicoIzquierdo` | [`include/Cinematica.h`](../../include/Cinematica.h#L39) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.pwmLogicoDerecho` | [`include/Cinematica.h`](../../include/Cinematica.h#L40) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.pwmPuenteIzquierdo` | [`include/Cinematica.h`](../../include/Cinematica.h#L41) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.pwmPuenteDerecho` | [`include/Cinematica.h`](../../include/Cinematica.h#L42) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoCalibracion.espera` | [`include/Cinematica.h`](../../include/Cinematica.h#L43) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ManualDriveFrame.throttle` | [`include/Comandos.h`](../../include/Comandos.h#L5) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ManualDriveFrame.steering` | [`include/Comandos.h`](../../include/Comandos.h#L6) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ManualDriveFrame.stream` | [`include/Comandos.h`](../../include/Comandos.h#L7) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `events` | — | — |
| `ManualDriveFrame.frame` | [`include/Comandos.h`](../../include/Comandos.h#L8) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ManualDriveFrame.recibidoMs` | [`include/Comandos.h`](../../include/Comandos.h#L9) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.tipo` | [`include/Comandos.h`](../../include/Comandos.h#L49) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.seq` | [`include/Comandos.h`](../../include/Comandos.h#L50) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.heading` | [`include/Comandos.h`](../../include/Comandos.h#L51) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.distanciaCm` | [`include/Comandos.h`](../../include/Comandos.h#L52) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.factor` | [`include/Comandos.h`](../../include/Comandos.h#L53) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.trimIzq` | [`include/Comandos.h`](../../include/Comandos.h#L55) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.trimDer` | [`include/Comandos.h`](../../include/Comandos.h#L56) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.deadbandIzq8` | [`include/Comandos.h`](../../include/Comandos.h#L57) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.deadbandDer8` | [`include/Comandos.h`](../../include/Comandos.h#L58) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.targetXCm` | [`include/Comandos.h`](../../include/Comandos.h#L61) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.targetYCm` | [`include/Comandos.h`](../../include/Comandos.h#L62) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.tieneObjetivoAbsoluto` | [`include/Comandos.h`](../../include/Comandos.h#L63) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.modoPaso` | [`include/Comandos.h`](../../include/Comandos.h#L64) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.pwmPositivo8` | [`include/Comandos.h`](../../include/Comandos.h#L66) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `Registro` | — | — |
| `ComandoRed.pwmNegativo8` | [`include/Comandos.h`](../../include/Comandos.h#L67) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `Registro` | — | — |
| `ComandoRed.polaridadPositiva` | [`include/Comandos.h`](../../include/Comandos.h#L68) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `Registro` | — | — |
| `ComandoRed.polaridadNegativa` | [`include/Comandos.h`](../../include/Comandos.h#L69) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `Registro` | — | — |
| `ComandoRed.icrXCm` | [`include/Comandos.h`](../../include/Comandos.h#L70) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.icrYCm` | [`include/Comandos.h`](../../include/Comandos.h#L71) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ComandoRed.gyroScale` | [`include/Comandos.h`](../../include/Comandos.h#L72) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlAngular::DecisionAngular.modo` | [`include/ControlAngular.h`](../../include/ControlAngular.h#L22) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlAngular::DecisionAngular.modLadoIzq` | [`include/ControlAngular.h`](../../include/ControlAngular.h#L23) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlAngular::DecisionAngular.modLadoDer` | [`include/ControlAngular.h`](../../include/ControlAngular.h#L24) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlAngular::DecisionAngular.solicitarPivote` | [`include/ControlAngular.h`](../../include/ControlAngular.h#L25) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlAngular.limitarFactor` | [`include/ControlAngular.h`](../../include/ControlAngular.h#L28) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `evaluarLazoAngular` | — | — |
| `ControlAngular.evaluarLazoAngular` | [`include/ControlAngular.h`](../../include/ControlAngular.h#L34) | 10 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | `limitarFactor` | — |
| `ControlCalibracion::EvidenciaPivot.derecha` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L11) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::EvidenciaPivot.izquierda` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L11) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::EvidenciaPivot.bilateral` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L12) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::EvidenciaPivot.equilibrado` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L12) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::EvidenciaPivot.torque` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L12) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion.evaluarPivot` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L14) | 9 | ESP32 / tiempo real | Bajo; interno; síncrona | `vigilarPivot` | — | — |
| `ControlCalibracion::VigilanciaPivot.movimiento` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L25) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::VigilanciaPivot.inicioMs` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L26) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::VigilanciaPivot.ventanaMs` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L26) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::VigilanciaPivot.ventanasAsimetricas` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L27) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::VigilanciaPivot.anterior` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L28) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion.vigilarPivot` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L30) | 17 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `evaluarPivot` | — |
| `ControlCalibracion::EvaluacionEncoders.promedioIzquierdo` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L57) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::EvaluacionEncoders.promedioDerecho` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L58) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::EvaluacionEncoders.ladoIzquierdoValido` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L59) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::EvaluacionEncoders.ladoDerechoValido` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L60) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::EvaluacionEncoders.responde` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L61) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::EvaluacionEncoders.sinRespuestaAislada` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L62) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::VigilanciaSilencioEncoders.activa` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L66) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::VigilanciaSilencioEncoders.inicioMs` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L67) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::VigilanciaSilencioEncoders.transcurridoMs` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L68) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::EvidenciaMovimiento.gyroConfirmado` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L72) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::EvidenciaMovimiento.pcntCorroborado` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L73) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::EvidenciaMovimiento.encodersQueResponden` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L74) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::EvidenciaMovimiento.confirmada` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L76) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::ComandoPivot.izquierda` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L80) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion::ComandoPivot.derecha` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L81) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion.comandoPivot` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L84) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion.evaluarMovimiento` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L91) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `calTorque` | — | — |
| `ControlCalibracion.actualizarSilencioConGiro` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L104) | 5 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion.reiniciarVigilanciaRetorno` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L125) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion.retornoSinProgreso` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L133) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCalibracion.evaluarEncoders` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L147) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `calTorque` | `promedioConfiableLado` | — |
| `ControlCalibracion.totalPasosRampa` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L179) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `pasoRampaActual`, `reiniciarDiagnosticoCalibracion` | — | — |
| `ControlCalibracion.pasoRampaActual` | [`include/ControlCalibracion.h`](../../include/ControlCalibracion.h#L184) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `actualizarDiagnosticoCalibracion`, `controlarGiro` | `totalPasosRampa` | — |
| `ControlCompensacion::Muestra.trimIzq` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L16) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCompensacion::Muestra.trimDer` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L17) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCompensacion::Muestra.deadbandIzq8` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L18) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCompensacion::Muestra.deadbandDer8` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L19) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCompensacion.aplicarPisoDeadband` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L25) | 8 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCompensacion.limitarTrim` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L33) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `agregar`, `establecer` | — | — |
| `ControlCompensacion.limitarDeadband8` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L37) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `agregar`, `establecer` | — | — |
| `ControlCompensacion::Perfil.reiniciar` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L44) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarGiro`, `frenarMotores`, `iniciarBaseGiro` | — | — |
| `ControlCompensacion::Perfil.agregar` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L48) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `leerArchivo`, `solicitarGuardarTorque` | `limitarDeadband8`, `limitarTrim` | — |
| `ControlCompensacion::Perfil.establecer` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L60) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `procesarComandos` | `limitarDeadband8`, `limitarTrim` | — |
| `ControlCompensacion::Perfil.getLadoIzq` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L69) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | — | — |
| `ControlCompensacion::Perfil.getLadoDer` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L70) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | — | — |
| `ControlCompensacion::Perfil.getProm` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L71) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCompensacion::Perfil.getDeadbandIzq8` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L72) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `aplicarVelocidades` | — | — |
| `ControlCompensacion::Perfil.getDeadbandDer8` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L73) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `aplicarVelocidades` | — | — |
| `ControlCompensacion::Perfil.getCantidad` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L74) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCompensacion::Perfil.trimIzq_` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L77) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCompensacion::Perfil.trimDer_` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L78) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCompensacion::Perfil.deadbandIzq8_` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L79) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCompensacion::Perfil.deadbandDer8_` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L80) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlCompensacion::Perfil.cantidad_` | [`include/ControlCompensacion.h`](../../include/ControlCompensacion.h#L81) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlConexion.accionAlPerderWebSocket` | [`include/ControlConexion.h`](../../include/ControlConexion.h#L10) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlInicializacionPCNT::Canal.inicializado` | [`include/ControlInicializacionPCNT.h`](../../include/ControlInicializacionPCNT.h#L18) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlInicializacionPCNT::Canal.etapaFallida` | [`include/ControlInicializacionPCNT.h`](../../include/ControlInicializacionPCNT.h#L19) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlInicializacionPCNT::Canal.codigoError` | [`include/ControlInicializacionPCNT.h`](../../include/ControlInicializacionPCNT.h#L20) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlInicializacionPCNT.registrar` | [`include/ControlInicializacionPCNT.h`](../../include/ControlInicializacionPCNT.h#L23) | 3 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlInicializacionPCNT.todosListos` | [`include/ControlInicializacionPCNT.h`](../../include/ControlInicializacionPCNT.h#L33) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `pcntInicializados` | — | — |
| `ControlInicializacionPCNT.textoEtapa` | [`include/ControlInicializacionPCNT.h`](../../include/ControlInicializacionPCNT.h#L39) | 8 | ESP32 / tiempo real | Bajo; interno; síncrona | `setup_Sensores` | — | — |
| `ControlManual::SalidaPWM.izquierdo` | [`include/ControlManual.h`](../../include/ControlManual.h#L9) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlManual::SalidaPWM.derecho` | [`include/ControlManual.h`](../../include/ControlManual.h#L10) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlManual.mezclar` | [`include/ControlManual.h`](../../include/ControlManual.h#L13) | 7 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlManual.leaseVigente` | [`include/ControlManual.h`](../../include/ControlManual.h#L26) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlManual.esperaPrimerFrameVigente` | [`include/ControlManual.h`](../../include/ControlManual.h#L30) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlManual.acercar` | [`include/ControlManual.h`](../../include/ControlManual.h#L35) | 6 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta::ErroresTrayectoria.longitudinalCm` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L12) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta::ErroresTrayectoria.lateralCm` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L13) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta::ErroresTrayectoria.euclidianoCm` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L14) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta::EstadoPI.integralGradoS` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L18) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta::SalidaPI.p` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L22) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta::SalidaPI.i` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L23) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta::SalidaPI.d` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L24) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta::SalidaPI.total` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L25) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta::SalidaPI.integralGradoS` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L26) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta.limitar` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L36) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `actualizarPI`, `balancearGiroDiferencial`, `correccionLateralRumboDeg`, `distanciaFrenoPrevista` | — | — |
| `ControlRuta.normalizar360` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L40) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `calCuenta`, `calTorque`, `controlarAvance`, `controlarCalibracion`, `errorAngularDeg`, `iniciarGiroAbsoluto`, `iniciarPaso`, `iniciarRecuperacionEndpoint`, `rumboCuerpoParaTrayecto` | — | — |
| `ControlRuta.errorAngularDeg` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L45) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `reversaAutomatica` | `normalizar360` | — |
| `ControlRuta.reversaAutomatica` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L52) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `iniciarPaso`, `iniciarRecuperacionEndpoint` | `errorAngularDeg` | — |
| `ControlRuta.rumboCuerpoParaTrayecto` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L57) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance`, `iniciarPaso`, `iniciarRecuperacionEndpoint` | `normalizar360` | — |
| `ControlRuta.correccionLateralParaDireccion` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L61) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | — | — |
| `ControlRuta.frenarLadoIzquierdoParaRumbo` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L70) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | — | — |
| `ControlRuta.distanciaPorTick` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L74) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAsentamientoFinal`, `controlarAvance`, `controlarGiro` | — | — |
| `ControlRuta.distanciaFrenoPrevista` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L80) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | `limitar` | — |
| `ControlRuta::CorreccionICR.dxCm` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L95) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta::CorreccionICR.dyCm` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L96) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta.corregirTraslacionParasita` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L99) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `corregirTraslacionParasitaGlobal` | — | — |
| `ControlRuta.corregirTraslacionParasitaGlobal` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L110) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `aplicarCorreccionICR` | `corregirTraslacionParasita` | — |
| `ControlRuta.calcularErroresTrayectoriaAnclado` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L127) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `actualizarErroresTrayectoria`, `calcularErroresTrayectoria` | — | — |
| `ControlRuta.calcularErroresTrayectoria` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L148) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `calcularErroresTrayectoriaAnclado` | — |
| `ControlRuta.correccionLateralRumboDeg` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L161) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | `limitar` | — |
| `ControlRuta.actualizarPI` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L168) | 6 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAvance` | `limitar` | estado |
| `ControlRuta.endpointAceptable` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L192) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `objetivoAbsolutoAlcanzado` | — | — |
| `ControlRuta.agotoIntentosEndpoint` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L200) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `decidirEndpoint` | — | — |
| `ControlRuta.decidirEndpoint` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L204) | 4 | ESP32 / tiempo real | Bajo; interno; síncrona | `decidirEndpointSeguro` | `agotoIntentosEndpoint` | — |
| `ControlRuta.decidirEndpointSeguro` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L216) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `iniciarRecuperacionEndpoint` | `decidirEndpoint` | — |
| `ControlRuta::SalidaGiroBilateral.pwmL` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L229) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta::SalidaGiroBilateral.pwmR` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L230) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta::SalidaGiroBilateral.compensacionPwm` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L231) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta::SalidaGiroBilateral.desplazamientoCentroCm` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L232) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlRuta.balancearGiroDiferencial` | [`include/ControlRuta.h`](../../include/ControlRuta.h#L238) | 6 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarGiro` | `limitar` | — |
| `ControlSeguridad.imuApta` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L11) | 2 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlSeguridad.estopSolicitado` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L15) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `procesarComandos` | — | — |
| `ControlSeguridad.encoderEsOutlier` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L19) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `clasificarEncoders`, `detectarOutliers` | — | — |
| `ControlSeguridad.deltaEncoderPlausible` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L24) | 3 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlSeguridad.saturarDeltaEncoder` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L28) | 4 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlSeguridad::ClasificacionEncoders.confiable` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L36) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlSeguridad::ClasificacionEncoders.mediana` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L37) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlSeguridad::ClasificacionEncoders.modoDegradado` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L38) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlSeguridad::ClasificacionEncoders.ladoIzquierdoValido` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L39) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlSeguridad::ClasificacionEncoders.ladoDerechoValido` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L40) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlSeguridad.medianaCuatro` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L46) | 4 | ESP32 / tiempo real | Bajo; interno; síncrona | `actualizarSaludEncoders`, `clasificarEncoders`, `mediana4` | — | — |
| `ControlSeguridad.clasificarEncoders` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L60) | 14 | ESP32 / tiempo real | Alto; interno; síncrona | `completarPausaReeval` | `encoderEsOutlier`, `medianaCuatro` | — |
| `ControlSeguridad.promedioConfiableLado` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L106) | 10 | ESP32 / tiempo real | Bajo; interno; síncrona | `actualizarSaludEncoders`, `controlarGiro`, `evaluarEncoders`, `promedioLado`, `promediosConfiableAcotados` | — | — |
| `ControlSeguridad.acotarLadoFuenteUnica` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L130) | 4 | ESP32 / tiempo real | Bajo; interno; síncrona | `promediosConfiableAcotados` | — | — |
| `ControlSeguridad::PromediosLado.izquierdo` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L138) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlSeguridad::PromediosLado.derecho` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L139) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlSeguridad.promediosConfiableAcotados` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L144) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `actualizarOdometria` | `acotarLadoFuenteUnica`, `promedioConfiableLado` | — |
| `ControlSeguridad.mediaEncodersSaludables` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L166) | 4 | ESP32 / tiempo real | Bajo; interno; síncrona | `estimarTicksAvance` | — | — |
| `ControlSeguridad.ladoEnStall` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L179) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `auditarSalud` | — | — |
| `ControlSeguridad.fuentesPorLadoValidas` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L184) | 4 | ESP32 / tiempo real | Bajo; interno; síncrona | `hayPorLado` | — | — |
| `ControlSeguridad.encoderSinRespuestaAislada` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L192) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `actualizarSaludEncoders` | — | — |
| `ControlSeguridad.nivelAntiFriccion8Bit` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L197) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `pwmAntiFriccion` | — | — |
| `ControlSeguridad.movimientoAntiFriccionConfirmado` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L204) | 2 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarAntiFriccion` | — | — |
| `ControlSeguridad.stopDebePreservarFallo` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L209) | 2 | ESP32 / tiempo real | Medio; interno; síncrona | `cancelarMovimiento` | — | parada/cierre |
| `ControlSeguridad::EstadoVigilanciaDivergenciaGiro.menorErrorAbs` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L214) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlSeguridad::EstadoVigilanciaDivergenciaGiro.inicioDivergenciaMs` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L215) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlSeguridad::EstadoVigilanciaDivergenciaGiro.reiniciar` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L217) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarGiro`, `frenarMotores`, `iniciarBaseGiro` | — | — |
| `ControlSeguridad.evaluarDivergenciaGiro` | [`include/ControlSeguridad.h`](../../include/ControlSeguridad.h#L223) | 6 | ESP32 / tiempo real | Bajo; interno; síncrona | `controlarGiro` | — | estado |
| `ControlTorque::Registro.secuencia` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L14) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `Registro` | — | — |
| `ControlTorque::Registro.pwmPositivo8` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L15) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `Registro` | — | — |
| `ControlTorque::Registro.pwmNegativo8` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L16) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `Registro` | — | — |
| `ControlTorque::Registro.polaridadPositiva` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L17) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `Registro` | — | — |
| `ControlTorque::Registro.polaridadNegativa` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L18) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `Registro` | — | — |
| `ControlTorque::Registro.pwmPositivoDerecho8` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L19) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `Registro` | — | — |
| `ControlTorque::Registro.pwmNegativoDerecho8` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L20) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `Registro` | — | — |
| `ControlTorque::Registro.Registro` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L22) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | `polaridadNegativa`, `polaridadPositiva`, `pwmNegativo8`, `pwmNegativoDerecho8`, `pwmPositivo8`, `pwmPositivoDerecho8`, `secuencia` | — |
| `ControlTorque::Historial.registros` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L32) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlTorque::Historial.cantidad` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L33) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlTorque::Historial.siguienteSecuencia` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L34) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `ControlTorque.registroValido` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L37) | 14 | ESP32 / tiempo real | Bajo; interno; síncrona | `agregar`, `leerArchivo`, `solicitarGuardarTorque` | — | — |
| `ControlTorque.agregar` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L48) | 5 | ESP32 / tiempo real | Bajo; interno; síncrona | `leerArchivo`, `solicitarGuardarTorque` | `registroValido` | — |
| `ControlTorque.promedioDireccion` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L61) | 6 | ESP32 / tiempo real | Bajo; interno; síncrona | `recalcularDiagnostico` | — | — |
| `ControlTorque.baseDesdePromedio` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L73) | 3 | ESP32 / tiempo real | Bajo; interno; síncrona | `baseParaPolaridad`, `recalcularDiagnostico` | — | — |
| `ControlTorque.baseParaPolaridad` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L80) | 7 | ESP32 / tiempo real | Bajo; interno; síncrona | `baseTorqueParaPolaridad8` | `baseDesdePromedio` | — |
| `ControlTorque.iguales` | [`include/ControlTorque.h`](../../include/ControlTorque.h#L96) | 10 | ESP32 / tiempo real | Bajo; interno; síncrona | `guardarAtomico` | — | — |
| `EventoRed.tipo` | [`include/Eventos.h`](../../include/Eventos.h#L14) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `EventoRed.seq` | [`include/Eventos.h`](../../include/Eventos.h#L15) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `EventoRed.run_id` | [`include/Eventos.h`](../../include/Eventos.h#L16) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `EventoRed.detalle` | [`include/Eventos.h`](../../include/Eventos.h#L17) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `EventoRed.progreso` | [`include/Eventos.h`](../../include/Eventos.h#L18) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoMemoriaTorque.montada` | [`include/MemoriaTorque.h`](../../include/MemoriaTorque.h#L6) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoMemoriaTorque.cargada` | [`include/MemoriaTorque.h`](../../include/MemoriaTorque.h#L7) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoMemoriaTorque.persistenciaPendiente` | [`include/MemoriaTorque.h`](../../include/MemoriaTorque.h#L8) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoMemoriaTorque.cantidad` | [`include/MemoriaTorque.h`](../../include/MemoriaTorque.h#L9) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoMemoriaTorque.promedioPositivo8` | [`include/MemoriaTorque.h`](../../include/MemoriaTorque.h#L10) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoMemoriaTorque.promedioNegativo8` | [`include/MemoriaTorque.h`](../../include/MemoriaTorque.h#L11) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoMemoriaTorque.promedioPositivoDerecho8` | [`include/MemoriaTorque.h`](../../include/MemoriaTorque.h#L12) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoMemoriaTorque.promedioNegativoDerecho8` | [`include/MemoriaTorque.h`](../../include/MemoriaTorque.h#L13) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoMemoriaTorque.basePositiva8` | [`include/MemoriaTorque.h`](../../include/MemoriaTorque.h#L14) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoMemoriaTorque.baseNegativa8` | [`include/MemoriaTorque.h`](../../include/MemoriaTorque.h#L15) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoMemoriaTorque.basePositivaDerecha8` | [`include/MemoriaTorque.h`](../../include/MemoriaTorque.h#L16) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoMemoriaTorque.baseNegativaDerecha8` | [`include/MemoriaTorque.h`](../../include/MemoriaTorque.h#L17) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `DiagnosticoMemoriaTorque.estado` | [`include/MemoriaTorque.h`](../../include/MemoriaTorque.h#L18) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | estado |
| `PoseEstimator.getX` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L21) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `actualizarErroresTrayectoria`, `iniciarAvance`, `iniciarPaso` | — | — |
| `PoseEstimator.getY` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L22) | 1 | ESP32 / tiempo real | Bajo; interno; síncrona | `actualizarErroresTrayectoria`, `iniciarAvance`, `iniciarPaso` | — | — |
| `PoseEstimator.getThetaRad` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L23) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.getThetaDeg` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L24) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.getArcoCentroGiroCm` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L28) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.getTraslacionGiroXCm` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L29) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.getTraslacionGiroYCm` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L30) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.x_global` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L33) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.y_global` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L34) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.theta_rad` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L35) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.last_pulsos_FL` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L37) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.last_pulsos_FR` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L38) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.last_pulsos_BL` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L39) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.last_pulsos_BR` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L40) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.cm_por_pulso` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L42) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.ultimo_signo_l` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L43) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.ultimo_signo_r` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L44) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.arco_centro_giro_cm` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L45) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.traslacion_giro_x_cm` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L46) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.traslacion_giro_y_cm` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L47) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `PoseEstimator.DISTANCIA_EJES_CM` | [`include/PoseEstimator.h`](../../include/PoseEstimator.h#L49) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Seguridad.inicio_movimiento_ms` | [`include/Seguridad.h`](../../include/Seguridad.h#L25) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Seguridad.pulsos_movimiento_iniciales` | [`include/Seguridad.h`](../../include/Seguridad.h#L26) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Seguridad.inicio_ventana_encoder_ms` | [`include/Seguridad.h`](../../include/Seguridad.h#L27) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Seguridad.pulsos_ventana_encoder` | [`include/Seguridad.h`](../../include/Seguridad.h#L28) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Seguridad.pulsos_lado_anteriores` | [`include/Seguridad.h`](../../include/Seguridad.h#L29) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Seguridad.ultimo_progreso_lado_ms` | [`include/Seguridad.h`](../../include/Seguridad.h#L30) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.pulsosFL` | [`include/Sensores.h`](../../include/Sensores.h#L6) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.pulsosFR` | [`include/Sensores.h`](../../include/Sensores.h#L7) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.pulsosBL` | [`include/Sensores.h`](../../include/Sensores.h#L8) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.pulsosBR` | [`include/Sensores.h`](../../include/Sensores.h#L9) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.delta_pulsos_filtrado_FL` | [`include/Sensores.h`](../../include/Sensores.h#L10) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.delta_pulsos_filtrado_FR` | [`include/Sensores.h`](../../include/Sensores.h#L11) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.delta_pulsos_filtrado_BL` | [`include/Sensores.h`](../../include/Sensores.h#L12) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.delta_pulsos_filtrado_BR` | [`include/Sensores.h`](../../include/Sensores.h#L13) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.velocidad_filtrada_L_cm_s` | [`include/Sensores.h`](../../include/Sensores.h#L14) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.velocidad_filtrada_R_cm_s` | [`include/Sensores.h`](../../include/Sensores.h#L15) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.imu_deltaZ_rad` | [`include/Sensores.h`](../../include/Sensores.h#L16) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.gyro_z_filtrado_rad_s` | [`include/Sensores.h`](../../include/Sensores.h#L17) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.gyro_z_offset_rad_s` | [`include/Sensores.h`](../../include/Sensores.h#L18) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.yaw_integrado_deg` | [`include/Sensores.h`](../../include/Sensores.h#L19) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.timestamp_ms` | [`include/Sensores.h`](../../include/Sensores.h#L20) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.timestamp_us` | [`include/Sensores.h`](../../include/Sensores.h#L21) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.mpu_present` | [`include/Sensores.h`](../../include/Sensores.h#L22) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.mpu_stale` | [`include/Sensores.h`](../../include/Sensores.h#L23) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.mpu_calibrated` | [`include/Sensores.h`](../../include/Sensores.h#L24) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.fuente5vOk` | [`include/Sensores.h`](../../include/Sensores.h#L25) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `SensorSnapshot.sequence` | [`include/Sensores.h`](../../include/Sensores.h#L26) | 1 | ESP32 / tiempo real | Bajo; sin llamada interna detectada; síncrona | — | — | — |
