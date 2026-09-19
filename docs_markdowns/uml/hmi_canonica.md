# UML funcional: `hmi/canonica`

Funciones detectadas: **136**. Tipos detectados: **0**.

## Grafo de llamadas

```mermaid
flowchart LR
  subgraph f0["index.html"]
    nc920c20639["initChart()"]
    n4a66003236["actualizarBarra()"]
    n24a15ab599["init()"]
    n3984eeca7a["api(path, options={})"]
    n41139c6e59["closeApplication(force=false)"]
    n990f36719a["connectBackend()"]
    n83e1f6a5de["consumeBackend(message)"]
    n8856afd435["applyBackendTelemetry(raw)"]
    n2523b3da51["send(obj)"]
    na16d4b72eb["manual(path, payload={})"]
    n2603ab56e9["sendMove(x, y)"]
    n864016acc7["init()"]
    ncdfe0b2c0c["normalizeRobotState(value)"]
    n7d3ca3c276["hasAllowed(command)"]
    ne92e783fe2["mpuStatus(raw)"]
    n40ce9c5f59["canCalibrate()"]
    nf0e5493a09["canRearm()"]
    ndf2e6d76df["calibrationBlockReason()"]
    nec94ee18ab["updateAvailability()"]
    na1ce955727["showCause(reason,visible)"]
    n361356a4fb["setState(label,ready,tone='')"]
    n1823b5c386["valueAt(container,key,index,fallback=undefined)"]
    nc685433f8a["renderDiagnostics(raw)"]
    n4f39883c9d["applyTelemetry(raw)"]
    n2a0ce8d425["onRobotEvent(event)"]
    nf4f2edf3b0["init()"]
    nf4f2714d2e["loadSurfaces(selectId = null)"]
    nd3d33dd5de["onSurfaceSelect(id)"]
    nfdecd17167["applySelected()"]
    n5876df6dcf["saveCurrentLiveSurface()"]
    n1cd5896aad["handleCalibrationCompleted()"]
    n4de58be028["saveCurrent()"]
    n0d33e5a437["captureFromTelemetry()"]
    na50e4ee06c["deleteSelected()"]
    naba53ca132["init()"]
    n22f5ba83e3["change(action)"]
    n4f2356b488["applySnapshot(snapshot)"]
    nb376cb9cd7["applyConnection(connection)"]
    nf04d539831["applySession(session)"]
    n88349ff6d1["applyIdentity(telemetry)"]
    n4cf2b8c26c["esc(value)"]
    n2561c0f60d["loadHistory()"]
    n405209be20["init()"]
    n9da1292af1["updateCount()"]
    n8d5d55c2c5["add(level, message)"]
    n82fb9bcb0b["clear()"]
    n270fccf849["setFilter(l, a)"]
    nb56e3ee7a0["downloadTXT()"]
    n94797771e7["init()"]
    n9dec1603e3["_ticks(gId, n, rI, rO)"]
    nf6fe4c7477["loop(now)"]
    nb339029d3f["init()"]
    n6168507dea["update(t)"]
    n401052095a["render()"]
    n009152b5be["_process(t)"]
    nc73c6a1f4c["_uiTelem()"]
    na43b41b606["_uiPos()"]
    n5926c7ce85["_ui()"]
    n99cd090c2a["update(s)"]
    nf055780b3f["constructor(key)"]
    n5f9ec861e3["configureForm()"]
    n0a0ae594e2["createLogicalStep()"]
    nb2b401af5f["buildVisualization()"]
    nf1ed7a87ce["updatePreview()"]
    ne5f48b24e3["missionPayload(manager)"]
    nc55f64d092["expandExecutionWaypoints(manager, step)"]
    ncc81181bcd["constructor()"]
    n0b10740a12["configureForm(manager)"]
    nb2d31879d2["createLogicalStep(manager, dx, dy)"]
    n5949ff7abc["buildVisualization(manager)"]
    n37ceb0fe01["constructor(key, apiMode, direct)"]
    nc407314f0e["configureForm(manager)"]
    n48d8d9108e["createLogicalStep(manager, length, theta)"]
    n91f67f7733["buildVisualization(manager)"]
    n5554fcb8e6["missionPayload(manager)"]
    n1ec8e9647d["updatePreview(manager)"]
    n231abe9ad5["constructor()"]
    n2bd7768d42["constructor()"]
    n03b4554ec6["init()"]
    n27e22de182["activeMode()"]
    nbc88534577["logicalEndpoint()"]
    nef8c1560ef["updateModeLock()"]
    nc7b2d56643["_chartOptions()"]
    nb41c327362["_initCharts()"]
    nf365a41764["label(text,x,y,color)"]
    n1a267233bf["arrow(from,to,color,text,side=1)"]
    ndab4919006["afterDatasetsDraw(chart)"]
    n48f5c03768["resizeCharts()"]
    nd7b60dcf0e["clearTelemetryTrail()"]
    n65c9da4c9e["applyCapabilities(capabilities)"]
    n3b93d41978["applyConfig(config)"]
    ne922c112c5["applyVectorialTelemetry(raw)"]
    nbb969c4429["vectorialReadiness()"]
    n9f8603867e["renderVectorialSafety()"]
    nfc48ee9c84["updateLabels()"]
    nf30ad44fd3["completarEjeRectangular(campoEditado)"]
    n9809f0accf["updatePreview()"]
    n61922f18a3["addStep()"]
    n8d2d36a778["removeLastStep()"]
    n1c432b7d7f["appendLimited(x0,y0,x1,y1,label,logicalStepId,component)"]
    n5f0a389548["updateListUI()"]
    naf4ef91332["drawPlan()"]
    n6c594e9d16["autoScale()"]
    n71cffbb962["updateTelemetryMap(posX, posY, orientacion)"]
    nfd2260262b["renderMetrics()"]
    n8106d678ad["preview()"]
    nd0ff89b74b["toggleCinema(cardId)"]
    n9e26492ebb["_syncCinemaButton(cardId, isCinema)"]
    n3dcd1bbbb9["lockTabs()"]
    n99e2233f50["unlockTabs()"]
    n4188fe30c8["start()"]
    n013c917aad["onRobotEvent(event)"]
    n911d7be02d["onMission(mission)"]
    nb6c6b50937["onFsmIdle()"]
    n03c6ba6f65["failRoute(reason)"]
    n4fc8d483c3["stopAndReset()"]
    ne37b6fd98a["clearRobotMemory()"]
    na37ae42142["ockhamReturn()"]
    ncebc828126["confirmOckhamReturn()"]
    nfe554c3fd4["cancelOckhamReturn()"]
    n4e04079c5a["init()"]
    n3be661733c["init()"]
    nfba79c889b["setCapability(enabled)"]
    n6c36da3dd8["_pos(e)"]
    n5347835722["_sendManual(pwmL, pwmR, dirL, dirR, force=false)"]
    n12c7f631ab["start(e)"]
    nfecebf0c4f["move(e)"]
    n753836fe5a["end()"]
    ndf34350afe["renderRecording(recording)"]
    n023e86354e["draw(list,color,dash,w)"]
    nc18886817a["returnRecording()"]
    nfd76531c65["deleteRecording()"]
    nee211105b1["_resetVis()"]
    nb77a7b658f["_bars(pL,pR,dL,dR)"]
    n712549f675["_dirName(a,m)"]
    n13420fc034["init()"]
  end
  n009152b5be --> n6168507dea
  n009152b5be --> n71cffbb962
  n009152b5be --> n8d5d55c2c5
  n009152b5be --> na43b41b606
  n009152b5be --> nb6c6b50937
  n009152b5be --> nc73c6a1f4c
  n03b4554ec6 --> n3984eeca7a
  n03b4554ec6 --> n3b93d41978
  n03b4554ec6 --> n4188fe30c8
  n03b4554ec6 --> n4fc8d483c3
  n03b4554ec6 --> n61922f18a3
  n03b4554ec6 --> n8106d678ad
  n03b4554ec6 --> n8d2d36a778
  n03b4554ec6 --> n8d5d55c2c5
  n03b4554ec6 --> n9f8603867e
  n03b4554ec6 --> na37ae42142
  n03b4554ec6 --> nb41c327362
  n03b4554ec6 --> ncebc828126
  n03b4554ec6 --> ne37b6fd98a
  n03b4554ec6 --> nf1ed7a87ce
  n03b4554ec6 --> nf30ad44fd3
  n03b4554ec6 --> nfc48ee9c84
  n03b4554ec6 --> nfe554c3fd4
  n03c6ba6f65 --> n5f0a389548
  n03c6ba6f65 --> n8d5d55c2c5
  n03c6ba6f65 --> n99e2233f50
  n03c6ba6f65 --> n9f8603867e
  n0d33e5a437 --> n8d5d55c2c5
  n12c7f631ab --> n5347835722
  n12c7f631ab --> n8d5d55c2c5
  n12c7f631ab --> na16d4b72eb
  n12c7f631ab --> nee211105b1
  n12c7f631ab --> nfecebf0c4f
  n13420fc034 --> n03c6ba6f65
  n13420fc034 --> n2523b3da51
  n13420fc034 --> n2561c0f60d
  n13420fc034 --> n270fccf849
  n13420fc034 --> n41139c6e59
  n13420fc034 --> n6168507dea
  n13420fc034 --> n753836fe5a
  n13420fc034 --> n82fb9bcb0b
  n13420fc034 --> n8d5d55c2c5
  n13420fc034 --> n99e2233f50
  n13420fc034 --> nb56e3ee7a0
  n13420fc034 --> nc920c20639
  n13420fc034 --> nd0ff89b74b
  n1a267233bf --> nf365a41764
  n1cd5896aad --> n3984eeca7a
  n1cd5896aad --> n8d5d55c2c5
  n1cd5896aad --> nf4f2714d2e
  n22f5ba83e3 --> n3984eeca7a
  n22f5ba83e3 --> n4f2356b488
  n22f5ba83e3 --> n8d5d55c2c5
  n24a15ab599 --> n8d5d55c2c5
  n24a15ab599 --> n990f36719a
  n2523b3da51 --> n3984eeca7a
  n2523b3da51 --> n8d5d55c2c5
  n2561c0f60d --> n3984eeca7a
  n2561c0f60d --> n4cf2b8c26c
  n2561c0f60d --> n8d5d55c2c5
  n2603ab56e9 --> n3984eeca7a
  n2603ab56e9 --> n8d5d55c2c5
  n2a0ce8d425 --> n1cd5896aad
  n2a0ce8d425 --> n361356a4fb
  n2a0ce8d425 --> n8d5d55c2c5
  n2a0ce8d425 --> na1ce955727
  n2a0ce8d425 --> nec94ee18ab
  n361356a4fb --> nec94ee18ab
  n3b93d41978 --> n9f8603867e
  n3be661733c --> n4188fe30c8
  n3be661733c --> n753836fe5a
  n3be661733c --> nc18886817a
  n3be661733c --> ndf34350afe
  n3be661733c --> nfba79c889b
  n3be661733c --> nfd76531c65
  n3be661733c --> nfecebf0c4f
  n3dcd1bbbb9 --> n8d5d55c2c5
  n40ce9c5f59 --> n7d3ca3c276
  n41139c6e59 --> n3984eeca7a
  n41139c6e59 --> n8d5d55c2c5
  n4188fe30c8 -.-> n27e22de182
  n4188fe30c8 -.-> n3984eeca7a
  n4188fe30c8 -.-> n3dcd1bbbb9
  n4188fe30c8 -.-> n5f0a389548
  n4188fe30c8 -.-> n8d5d55c2c5
  n4188fe30c8 -.-> n99e2233f50
  n4188fe30c8 -.-> naf4ef91332
  n4188fe30c8 -.-> nbb969c4429
  n4188fe30c8 -.-> nd7b60dcf0e
  n4188fe30c8 -.-> ne5f48b24e3
  n48d8d9108e --> nbc88534577
  n4a66003236 --> n6168507dea
  n4de58be028 --> n3984eeca7a
  n4de58be028 --> n8d5d55c2c5
  n4de58be028 --> nf4f2714d2e
  n4f2356b488 --> n65c9da4c9e
  n4f2356b488 --> n88349ff6d1
  n4f2356b488 --> nb376cb9cd7
  n4f2356b488 --> nf04d539831
  n4f39883c9d --> n361356a4fb
  n4f39883c9d --> n8d5d55c2c5
  n4f39883c9d --> na1ce955727
  n4f39883c9d --> nc685433f8a
  n4f39883c9d --> ncdfe0b2c0c
  n4f39883c9d --> ne92e783fe2
  n4fc8d483c3 -.-> n3984eeca7a
  n4fc8d483c3 -.-> n5f0a389548
  n4fc8d483c3 -.-> n8d5d55c2c5
  n4fc8d483c3 -.-> n99e2233f50
  n4fc8d483c3 -.-> n9f8603867e
  n4fc8d483c3 -.-> nd7b60dcf0e
  n4fc8d483c3 -.-> nef8c1560ef
  n4fc8d483c3 -.-> nf1ed7a87ce
  n4fc8d483c3 -.-> nfc48ee9c84
  n5347835722 --> n753836fe5a
  n5347835722 --> n8d5d55c2c5
  n5347835722 --> na16d4b72eb
  n5876df6dcf --> n3984eeca7a
  n5876df6dcf --> n8d5d55c2c5
  n5876df6dcf --> nf4f2714d2e
  n5926c7ce85 --> n401052095a
  n6168507dea --> n401052095a
  n61922f18a3 -.-> n0a0ae594e2
  n61922f18a3 -.-> n27e22de182
  n61922f18a3 -.-> n5f0a389548
  n61922f18a3 -.-> n6c594e9d16
  n61922f18a3 -.-> n8d5d55c2c5
  n61922f18a3 -.-> nc55f64d092
  n61922f18a3 -.-> nd7b60dcf0e
  n61922f18a3 -.-> nef8c1560ef
  n61922f18a3 -.-> nf1ed7a87ce
  n61922f18a3 -.-> nfc48ee9c84
  n65c9da4c9e --> n3b93d41978
  n6c594e9d16 -.-> n27e22de182
  n6c594e9d16 -.-> n6168507dea
  n71cffbb962 --> n6168507dea
  n71cffbb962 --> nfd2260262b
  n753836fe5a --> n8d5d55c2c5
  n753836fe5a --> na16d4b72eb
  n753836fe5a --> nb77a7b658f
  n753836fe5a --> ndf34350afe
  n753836fe5a --> nee211105b1
  n8106d678ad --> nd0ff89b74b
  n82fb9bcb0b --> n9da1292af1
  n83e1f6a5de --> n2561c0f60d
  n83e1f6a5de --> n2a0ce8d425
  n83e1f6a5de --> n3b93d41978
  n83e1f6a5de --> n4f2356b488
  n83e1f6a5de --> n8856afd435
  n83e1f6a5de --> n8d5d55c2c5
  n83e1f6a5de --> n911d7be02d
  n83e1f6a5de --> nb376cb9cd7
  n83e1f6a5de --> nf04d539831
  n864016acc7 -.-> n03c6ba6f65
  n864016acc7 -.-> n2523b3da51
  n864016acc7 -.-> n361356a4fb
  n864016acc7 -.-> n3984eeca7a
  n864016acc7 -.-> n40ce9c5f59
  n864016acc7 -.-> n5f0a389548
  n864016acc7 -.-> n8d5d55c2c5
  n864016acc7 -.-> naf4ef91332
  n864016acc7 -.-> ndf2e6d76df
  n864016acc7 -.-> nec94ee18ab
  n864016acc7 -.-> nf0e5493a09
  n8856afd435 --> n009152b5be
  n8856afd435 --> n4f39883c9d
  n8856afd435 --> n6168507dea
  n8856afd435 --> n88349ff6d1
  n8856afd435 --> n8d5d55c2c5
  n8856afd435 --> ne922c112c5
  n8856afd435 --> nfba79c889b
  n8d2d36a778 -.-> n5f0a389548
  n8d2d36a778 -.-> n6c594e9d16
  n8d2d36a778 -.-> n8d5d55c2c5
  n8d2d36a778 -.-> nef8c1560ef
  n8d2d36a778 -.-> nf1ed7a87ce
  n8d2d36a778 -.-> nfc48ee9c84
  n8d5d55c2c5 --> n9da1292af1
  n911d7be02d --> n03c6ba6f65
  n911d7be02d --> n5f0a389548
  n911d7be02d --> n8d5d55c2c5
  n911d7be02d --> n99e2233f50
  n911d7be02d --> n9f8603867e
  n911d7be02d --> naf4ef91332
  n94797771e7 --> n9dec1603e3
  n94797771e7 --> nf6fe4c7477
  n9809f0accf --> n27e22de182
  n990f36719a --> n3984eeca7a
  n990f36719a --> n83e1f6a5de
  n990f36719a --> n8d5d55c2c5
  n99cd090c2a --> n8d5d55c2c5
  n9e26492ebb --> n8d5d55c2c5
  n9f8603867e --> nbb969c4429
  na16d4b72eb --> n3984eeca7a
  na37ae42142 -.-> n3984eeca7a
  na37ae42142 -.-> n5f0a389548
  na37ae42142 -.-> n6c594e9d16
  na37ae42142 -.-> n8d5d55c2c5
  na37ae42142 -.-> naf4ef91332
  na37ae42142 -.-> nef8c1560ef
  na50e4ee06c --> n3984eeca7a
  na50e4ee06c --> n8d5d55c2c5
  na50e4ee06c --> nf4f2714d2e
  naba53ca132 --> n22f5ba83e3
  naba53ca132 --> n2561c0f60d
  naf4ef91332 -.-> n27e22de182
  naf4ef91332 -.-> n6168507dea
  naf4ef91332 -.-> nb2b401af5f
  nb2d31879d2 --> nbc88534577
  nb339029d3f --> n401052095a
  nb376cb9cd7 -.-> n03c6ba6f65
  nb376cb9cd7 -.-> n5926c7ce85
  nb376cb9cd7 -.-> n5f0a389548
  nb376cb9cd7 -.-> n99e2233f50
  nb376cb9cd7 -.-> naf4ef91332
  nb376cb9cd7 -.-> nec94ee18ab
  nb41c327362 --> n1a267233bf
  nb41c327362 --> nc7b2d56643
  nb41c327362 --> ndab4919006
  nb41c327362 --> nf365a41764
  nc18886817a -.-> n3984eeca7a
  nc18886817a -.-> n6c594e9d16
  nc18886817a -.-> n8d5d55c2c5
  nc18886817a -.-> n911d7be02d
  nc407314f0e --> n9f8603867e
  nc407314f0e --> nf1ed7a87ce
  nc55f64d092 --> n1c432b7d7f
  nc685433f8a --> n1823b5c386
  nc73c6a1f4c --> n6168507dea
  nc920c20639 --> n2603ab56e9
  nc920c20639 --> n4a66003236
  nc920c20639 --> n6168507dea
  ncebc828126 --> n3984eeca7a
  ncebc828126 --> n3dcd1bbbb9
  ncebc828126 --> n5f0a389548
  ncebc828126 --> n8d5d55c2c5
  ncebc828126 --> naf4ef91332
  ncebc828126 --> nd7b60dcf0e
  nd0ff89b74b --> n48f5c03768
  nd0ff89b74b --> n9e26492ebb
  nd7b60dcf0e --> n6168507dea
  nd7b60dcf0e --> nfd2260262b
  ndf2e6d76df --> n7d3ca3c276
  ndf2e6d76df --> ne92e783fe2
  ndf34350afe --> n023e86354e
  ne37b6fd98a --> n3984eeca7a
  ne37b6fd98a --> n5f0a389548
  ne37b6fd98a --> n8d5d55c2c5
  ne37b6fd98a --> n99e2233f50
  ne37b6fd98a --> naf4ef91332
  ne922c112c5 --> n9f8603867e
  nec94ee18ab --> n40ce9c5f59
  nec94ee18ab --> ndf2e6d76df
  nec94ee18ab --> nf0e5493a09
  nf04d539831 -.-> n03c6ba6f65
  nf04d539831 -.-> n5f0a389548
  nf04d539831 -.-> naf4ef91332
  nf0e5493a09 --> n7d3ca3c276
  nf4f2714d2e --> n3984eeca7a
  nf4f2714d2e --> n8d5d55c2c5
  nf4f2714d2e --> nd3d33dd5de
  nf4f2edf3b0 --> n0d33e5a437
  nf4f2edf3b0 --> n4de58be028
  nf4f2edf3b0 --> n5876df6dcf
  nf4f2edf3b0 --> na50e4ee06c
  nf4f2edf3b0 --> nd3d33dd5de
  nf4f2edf3b0 --> nf4f2714d2e
  nf4f2edf3b0 --> nfdecd17167
  nf6fe4c7477 --> n8d5d55c2c5
  nfba79c889b --> n753836fe5a
  nfc48ee9c84 --> n27e22de182
  nfc48ee9c84 --> n5f9ec861e3
  nfc48ee9c84 --> naf4ef91332
  nfd2260262b -.-> nbc88534577
  nfd76531c65 --> n3984eeca7a
  nfd76531c65 --> n8d5d55c2c5
  nfd76531c65 --> ndf34350afe
  nfdecd17167 --> n361356a4fb
  nfdecd17167 --> n3984eeca7a
  nfdecd17167 --> n8d5d55c2c5
  nfdecd17167 --> nec94ee18ab
  nfe554c3fd4 -.-> n5f0a389548
  nfe554c3fd4 -.-> n8d5d55c2c5
  nfe554c3fd4 -.-> naf4ef91332
  nfecebf0c4f --> n5347835722
  nfecebf0c4f --> n6c36da3dd8
  nfecebf0c4f --> n712549f675
  nfecebf0c4f --> n753836fe5a
  nfecebf0c4f --> nb77a7b658f
  classDef alto fill:#5b1f2a,stroke:#ff7a7a,color:#fff
  classDef medio fill:#4a3717,stroke:#ffca67,color:#fff
  classDef bajo fill:#123b3a,stroke:#39e6aa,color:#fff
  class nc920c20639 medio
  class n4a66003236 bajo
  class n24a15ab599 bajo
  class n3984eeca7a bajo
  class n41139c6e59 medio
  class n990f36719a bajo
  class n83e1f6a5de medio
  class n8856afd435 alto
  class n2523b3da51 medio
  class na16d4b72eb bajo
  class n2603ab56e9 bajo
  class n864016acc7 medio
  class ncdfe0b2c0c bajo
  class n7d3ca3c276 bajo
  class ne92e783fe2 bajo
  class n40ce9c5f59 bajo
  class nf0e5493a09 medio
  class ndf2e6d76df medio
  class nec94ee18ab medio
  class na1ce955727 bajo
  class n361356a4fb medio
  class n1823b5c386 bajo
  class nc685433f8a alto
  class n4f39883c9d alto
  class n2a0ce8d425 bajo
  class nf4f2edf3b0 alto
  class nf4f2714d2e alto
  class nd3d33dd5de bajo
  class nfdecd17167 alto
  class n5876df6dcf alto
  class n1cd5896aad alto
  class n4de58be028 alto
  class n0d33e5a437 alto
  class na50e4ee06c medio
  class naba53ca132 bajo
  class n22f5ba83e3 bajo
  class n4f2356b488 bajo
  class nb376cb9cd7 medio
  class nf04d539831 medio
  class n88349ff6d1 bajo
  class n4cf2b8c26c bajo
  class n2561c0f60d medio
  class n405209be20 bajo
  class n9da1292af1 bajo
  class n8d5d55c2c5 bajo
  class n82fb9bcb0b bajo
  class n270fccf849 bajo
  class nb56e3ee7a0 bajo
  class n94797771e7 bajo
  class n9dec1603e3 bajo
  class nf6fe4c7477 alto
  class nb339029d3f bajo
  class n6168507dea bajo
  class n401052095a medio
  class n009152b5be bajo
  class nc73c6a1f4c bajo
  class na43b41b606 bajo
  class n5926c7ce85 bajo
  class n99cd090c2a medio
  class nf055780b3f bajo
  class n5f9ec861e3 bajo
  class n0a0ae594e2 bajo
  class nb2b401af5f bajo
  class nf1ed7a87ce bajo
  class ne5f48b24e3 bajo
  class nc55f64d092 bajo
  class ncc81181bcd bajo
  class n0b10740a12 bajo
  class nb2d31879d2 bajo
  class n5949ff7abc bajo
  class n37ceb0fe01 bajo
  class nc407314f0e bajo
  class n48d8d9108e bajo
  class n91f67f7733 bajo
  class n5554fcb8e6 bajo
  class n1ec8e9647d bajo
  class n231abe9ad5 bajo
  class n2bd7768d42 bajo
  class n03b4554ec6 medio
  class n27e22de182 bajo
  class nbc88534577 bajo
  class nef8c1560ef bajo
  class nc7b2d56643 bajo
  class nb41c327362 bajo
  class nf365a41764 bajo
  class n1a267233bf bajo
  class ndab4919006 bajo
  class n48f5c03768 bajo
  class nd7b60dcf0e bajo
  class n65c9da4c9e bajo
  class n3b93d41978 bajo
  class ne922c112c5 bajo
  class nbb969c4429 bajo
  class n9f8603867e bajo
  class nfc48ee9c84 bajo
  class nf30ad44fd3 bajo
  class n9809f0accf bajo
  class n61922f18a3 bajo
  class n8d2d36a778 bajo
  class n1c432b7d7f bajo
  class n5f0a389548 bajo
  class naf4ef91332 bajo
  class n6c594e9d16 bajo
  class n71cffbb962 bajo
  class nfd2260262b bajo
  class n8106d678ad bajo
  class nd0ff89b74b bajo
  class n9e26492ebb medio
  class n3dcd1bbbb9 bajo
  class n99e2233f50 bajo
  class n4188fe30c8 medio
  class n013c917aad bajo
  class n911d7be02d medio
  class nb6c6b50937 bajo
  class n03c6ba6f65 bajo
  class n4fc8d483c3 medio
  class ne37b6fd98a medio
  class na37ae42142 medio
  class ncebc828126 medio
  class nfe554c3fd4 bajo
  class n4e04079c5a bajo
  class n3be661733c bajo
  class nfba79c889b bajo
  class n6c36da3dd8 bajo
  class n5347835722 medio
  class n12c7f631ab medio
  class nfecebf0c4f medio
  class n753836fe5a medio
  class ndf34350afe bajo
  class n023e86354e bajo
  class nc18886817a medio
  class nfd76531c65 bajo
  class nee211105b1 bajo
  class nb77a7b658f bajo
  class n712549f675 bajo
  class n13420fc034 alto
```

Fuentes: [Mermaid](mermaid/hmi_canonica.mmd) · [PlantUML](plantuml/hmi_canonica.puml). Las flechas continuas son llamadas síncronas; las discontinuas representan asincronía, eventos o colas. El color del nodo indica riesgo estático.

## Inventario

| Función | Archivo | CC | Propietario | Riesgo/estado | Entra desde | Sale hacia | Estado compartido |
|---|---|---:|---|---|---|---|---|
| `HMI.initChart` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1881) | 5 | HMI / operador | Medio; interno; asíncrona | `init` | `actualizarBarra`, `sendMove`, `update` | — |
| `HMI.actualizarBarra` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1977) | 3 | HMI / operador | Bajo; interno; síncrona | `initChart` | `update` | — |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2013) | 2 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | `add`, `connectBackend` | — |
| `HMI.api` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2020) | 9 | HMI / operador | Bajo; interno; asíncrona | `applySelected`, `change`, `clearRobotMemory`, `closeApplication`, `confirmOckhamReturn`, `connectBackend`, `deleteRecording`, `deleteSelected`, `handleCalibrationCompleted`, `init`, `loadHistory`, `loadSurfaces`, `manual`, `ockhamReturn`, `returnRecording`, `saveCurrent`, `saveCurrentLiveSurface`, `send`, `sendMove`, `start`, `stopAndReset` | — | — |
| `HMI.closeApplication` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2044) | 10 | HMI / operador | Medio; interno; asíncrona | `init` | `add`, `api`, `close`, `closeApp` | — |
| `HMI.connectBackend` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2069) | 3 | HMI / operador | Bajo; interno; asíncrona | `init` | `add`, `api`, `consumeBackend` | — |
| `HMI.consumeBackend` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2080) | 12 | HMI / operador | Medio; interno; síncrona | `connectBackend` | `add`, `applyBackendTelemetry`, `applyConfig`, `applyConnection`, `applySession`, `applySnapshot`, `loadHistory`, `onMission`, `onRobotEvent` | telemetría, sesión |
| `HMI.applyBackendTelemetry` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2099) | 166 | HMI / operador | Alto; interno; síncrona | `consumeBackend` | `_process`, `add`, `applyIdentity`, `applyTelemetry`, `applyVectorialTelemetry`, `setCapability`, `update` | estado, telemetría, parada/cierre |
| `HMI.send` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2179) | 8 | HMI / operador | Medio; interno; asíncrona | `_drain_one`, `_run`, `init` | `add`, `api` | parada/cierre |
| `HMI.manual` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2194) | 2 | HMI / operador | Bajo; interno; síncrona | `_sendManual`, `end`, `start` | `api` | — |
| `HMI.sendMove` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2197) | 4 | HMI / operador | Bajo; interno; síncrona | `initChart` | `add`, `api` | — |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2213) | 19 | HMI / operador | Medio; sin llamada interna detectada; cola/evento | — | `add`, `api`, `calibrationBlockReason`, `canCalibrate`, `canRearm`, `drawPlan`, `failRoute`, `send`, `setState`, `updateAvailability`, `updateListUI` | parada/cierre |
| `HMI.normalizeRobotState` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2275) | 3 | HMI / operador | Bajo; interno; síncrona | `applyTelemetry` | — | — |
| `HMI.hasAllowed` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2279) | 2 | HMI / operador | Bajo; interno; síncrona | `calibrationBlockReason`, `canCalibrate`, `canRearm` | — | — |
| `HMI.mpuStatus` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2282) | 18 | HMI / operador | Bajo; interno; síncrona | `applyTelemetry`, `calibrationBlockReason` | — | — |
| `HMI.canCalibrate` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2294) | 6 | HMI / operador | Bajo; interno; síncrona | `init`, `updateAvailability` | `hasAllowed` | — |
| `HMI.canRearm` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2298) | 4 | HMI / operador | Medio; interno; síncrona | `init`, `updateAvailability` | `hasAllowed` | parada/cierre |
| `HMI.calibrationBlockReason` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2302) | 13 | HMI / operador | Medio; interno; síncrona | `init`, `updateAvailability` | `hasAllowed`, `mpuStatus` | estado, parada/cierre |
| `HMI.updateAvailability` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2315) | 4 | HMI / operador | Medio; interno; síncrona | `applyConnection`, `applySelected`, `init`, `onRobotEvent`, `setState` | `calibrationBlockReason`, `canCalibrate`, `canRearm` | parada/cierre |
| `HMI.showCause` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2325) | 3 | HMI / operador | Bajo; interno; síncrona | `applyTelemetry`, `onRobotEvent` | — | — |
| `HMI.setState` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2330) | 1 | HMI / operador | Medio; interno; síncrona | `applySelected`, `applyTelemetry`, `init`, `onRobotEvent` | `updateAvailability` | parada/cierre |
| `HMI.valueAt` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2340) | 10 | HMI / operador | Bajo; interno; síncrona | `renderDiagnostics` | — | — |
| `HMI.renderDiagnostics` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2345) | 115 | HMI / operador | Alto; interno; síncrona | `applyTelemetry` | `valueAt` | — |
| `HMI.applyTelemetry` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2416) | 88 | HMI / operador | Alto; interno; síncrona | `applyBackendTelemetry` | `add`, `mpuStatus`, `normalizeRobotState`, `renderDiagnostics`, `setState`, `showCause` | estado, parada/cierre |
| `HMI.onRobotEvent` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2520) | 31 | HMI / operador | Bajo; interno; síncrona | `consumeBackend` | `add`, `handleCalibrationCompleted`, `setState`, `showCause`, `updateAvailability` | estado |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2576) | 20 | HMI / operador | Alto; sin llamada interna detectada; síncrona | — | `applySelected`, `captureFromTelemetry`, `deleteSelected`, `loadSurfaces`, `onSurfaceSelect`, `saveCurrent`, `saveCurrentLiveSurface` | — |
| `HMI.loadSurfaces` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2643) | 16 | HMI / operador | Alto; interno; asíncrona | `deleteSelected`, `handleCalibrationCompleted`, `init`, `saveCurrent`, `saveCurrentLiveSurface` | `add`, `api`, `onSurfaceSelect` | — |
| `HMI.onSurfaceSelect` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2677) | 26 | HMI / operador | Bajo; interno; síncrona | `init`, `loadSurfaces` | — | — |
| `HMI.applySelected` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2701) | 18 | HMI / operador | Alto; interno; asíncrona | `init` | `add`, `api`, `setState`, `updateAvailability` | — |
| `HMI.saveCurrentLiveSurface` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2740) | 21 | HMI / operador | Alto; interno; asíncrona | `init` | `add`, `api`, `loadSurfaces` | — |
| `HMI.handleCalibrationCompleted` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2778) | 18 | HMI / operador | Alto; interno; asíncrona | `onRobotEvent` | `add`, `api`, `loadSurfaces` | — |
| `HMI.saveCurrent` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2813) | 21 | HMI / operador | Alto; interno; asíncrona | `init` | `add`, `api`, `loadSurfaces` | — |
| `HMI.captureFromTelemetry` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2855) | 18 | HMI / operador | Alto; interno; síncrona | `init` | `add` | — |
| `HMI.deleteSelected` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2875) | 9 | HMI / operador | Medio; interno; asíncrona | `init` | `add`, `api`, `loadSurfaces` | — |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2905) | 1 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | `change`, `loadHistory` | — |
| `HMI.change` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2911) | 4 | HMI / operador | Bajo; interno; asíncrona | `init` | `add`, `api`, `applySnapshot` | — |
| `HMI.applySnapshot` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2921) | 10 | HMI / operador | Bajo; interno; síncrona | `change`, `consumeBackend` | `applyCapabilities`, `applyConnection`, `applyIdentity`, `applySession` | telemetría |
| `HMI.applyConnection` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2929) | 10 | HMI / operador | Medio; interno; cola/evento | `applySnapshot`, `consumeBackend` | `_ui`, `drawPlan`, `failRoute`, `unlockTabs`, `updateAvailability`, `updateListUI` | telemetría |
| `HMI.applySession` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2953) | 6 | HMI / operador | Medio; interno; cola/evento | `applySnapshot`, `consumeBackend` | `drawPlan`, `failRoute`, `updateListUI` | sesión |
| `HMI.applyIdentity` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2966) | 4 | HMI / operador | Bajo; interno; síncrona | `applyBackendTelemetry`, `applySnapshot` | — | telemetría |
| `HMI.esc` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2971) | 3 | HMI / operador | Bajo; interno; síncrona | `loadHistory` | — | — |
| `HMI.loadHistory` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2972) | 7 | HMI / operador | Medio; interno; asíncrona | `consumeBackend`, `init` | `add`, `api`, `esc` | telemetría, sesión |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2995) | 1 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `HMI.updateCount` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2996) | 1 | HMI / operador | Bajo; interno; síncrona | `add`, `clear` | — | — |
| `HMI.add` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3001) | 7 | HMI / operador | Bajo; interno; síncrona | `_on_robot_message`, `_process`, `_sendManual`, `_syncCinemaButton`, `addStep`, `applyBackendTelemetry`, `applySelected`, `applyTelemetry`, `cancel`, `cancelOckhamReturn`, `captureFromTelemetry`, `change`, `clearRobotMemory`, `closeApplication`, `confirmOckhamReturn`, `connectBackend`, `consumeBackend`, `deleteRecording`, `deleteSelected`, `dependency_cycles`, `end`, `failRoute`, `handleCalibrationCompleted`, `hlStep`, `init`, `initResize`, `loadHistory`, `loadSurfaces`, `lockTabs`, `loop`, `mainW`, `mermaid_for_folder`, `ockhamReturn`, `onDown`, `onMission`, `onRobotEvent`, `pL`, `pR`, `removeLastStep`, `returnRecording`, `saveCurrent`, `saveCurrentLiveSurface`, `send`, `sendMove`, `send_command`, `simplify_rdp`, `start`, `startC`, `stopAndReset`, `subscribe`, `update`, `visit` | `updateCount` | — |
| `HMI.clear` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3028) | 3 | HMI / operador | Bajo; interno; síncrona | `_run`, `init`, `start` | `updateCount` | — |
| `HMI.setFilter` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3034) | 2 | HMI / operador | Bajo; interno; síncrona | `init` | — | — |
| `HMI.downloadTXT` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3035) | 2 | HMI / operador | Bajo; interno; síncrona | `init` | — | estado |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3047) | 2 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | `_ticks`, `loop` | — |
| `HMI._ticks` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3060) | 4 | HMI / operador | Bajo; interno; síncrona | `init` | — | — |
| `HMI.loop` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3070) | 19 | HMI / operador | Alto; entrada/framework; síncrona | `init` | `add` | estado, telemetría |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3120) | 1 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | `render` | — |
| `HMI.update` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3121) | 13 | HMI / operador | Bajo; interno; síncrona | `_process`, `_uiTelem`, `actualizarBarra`, `applyBackendTelemetry`, `autoScale`, `clearTelemetryTrail`, `compact_database`, `create_app`, `drawPlan`, `finish`, `init`, `initChart`, `updateTelemetryMap` | `render` | — |
| `HMI.render` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3142) | 10 | HMI / operador | Medio; interno; síncrona | `_ui`, `init`, `main`, `update` | — | telemetría |
| `HMI._process` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3161) | 6 | HMI / operador | Bajo; interno; síncrona | `applyBackendTelemetry` | `_uiPos`, `_uiTelem`, `add`, `onFsmIdle`, `update`, `updateTelemetryMap` | estado, telemetría |
| `HMI._uiTelem` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3178) | 43 | HMI / operador | Bajo; interno; síncrona | `_process` | `update` | estado, telemetría |
| `HMI._uiPos` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3231) | 3 | HMI / operador | Bajo; interno; síncrona | `_process` | — | telemetría |
| `HMI._ui` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3246) | 2 | HMI / operador | Bajo; interno; síncrona | `applyConnection` | `render` | — |
| `HMI.update` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3254) | 6 | HMI / operador | Medio; interno; síncrona | `_process`, `_uiTelem`, `actualizarBarra`, `applyBackendTelemetry`, `autoScale`, `clearTelemetryTrail`, `compact_database`, `create_app`, `drawPlan`, `finish`, `init`, `initChart`, `updateTelemetryMap` | `add` | parada/cierre |
| `HMI.constructor` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3278) | 1 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `HMI.configureForm` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3279) | 1 | HMI / operador | Bajo; interno; síncrona | `updateLabels` | — | — |
| `HMI.createLogicalStep` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3280) | 1 | HMI / operador | Bajo; interno; síncrona | `addStep` | — | — |
| `HMI.buildVisualization` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3281) | 1 | HMI / operador | Bajo; interno; síncrona | `drawPlan` | — | — |
| `HMI.updatePreview` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3282) | 1 | HMI / operador | Bajo; interno; síncrona | `addStep`, `configureForm`, `init`, `removeLastStep`, `stopAndReset` | — | — |
| `HMI.missionPayload` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3283) | 1 | HMI / operador | Bajo; interno; cola/evento | `start` | — | — |
| `HMI.expandExecutionWaypoints` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3286) | 1 | HMI / operador | Bajo; interno; síncrona | `addStep` | `appendLimited` | — |
| `HMI.constructor` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3295) | 1 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `HMI.configureForm` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3296) | 1 | HMI / operador | Bajo; interno; síncrona | `updateLabels` | — | — |
| `HMI.createLogicalStep` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3310) | 7 | HMI / operador | Bajo; interno; síncrona | `addStep` | `logicalEndpoint` | — |
| `HMI.buildVisualization` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3322) | 1 | HMI / operador | Bajo; interno; cola/evento | `drawPlan` | — | — |
| `HMI.constructor` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3332) | 1 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `HMI.configureForm` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3335) | 5 | HMI / operador | Bajo; interno; síncrona | `updateLabels` | `renderVectorialSafety`, `updatePreview` | — |
| `HMI.createLogicalStep` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3356) | 11 | HMI / operador | Bajo; interno; síncrona | `addStep` | `logicalEndpoint` | — |
| `HMI.buildVisualization` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3381) | 2 | HMI / operador | Bajo; interno; síncrona | `drawPlan` | — | — |
| `HMI.missionPayload` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3395) | 1 | HMI / operador | Bajo; interno; síncrona | `start` | — | — |
| `HMI.updatePreview` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3403) | 4 | HMI / operador | Bajo; interno; síncrona | `addStep`, `configureForm`, `init`, `removeLastStep`, `stopAndReset` | — | — |
| `HMI.constructor` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3416) | 1 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `HMI.constructor` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3420) | 1 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3431) | 4 | HMI / operador | Medio; sin llamada interna detectada; asíncrona | — | `_initCharts`, `add`, `addStep`, `api`, `applyConfig`, `cancelOckhamReturn`, `clearRobotMemory`, `completarEjeRectangular`, `confirmOckhamReturn`, `ockhamReturn`, `preview`, `removeLastStep`, `renderVectorialSafety`, `start`, `stopAndReset`, `updateLabels`, `updatePreview` | — |
| `HMI.activeMode` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3487) | 3 | HMI / operador | Bajo; interno; síncrona | `addStep`, `autoScale`, `drawPlan`, `start`, `updateLabels`, `updatePreview` | — | — |
| `HMI.logicalEndpoint` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3490) | 2 | HMI / operador | Bajo; interno; síncrona | `createLogicalStep`, `renderMetrics` | — | — |
| `HMI.updateModeLock` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3493) | 4 | HMI / operador | Bajo; interno; síncrona | `addStep`, `ockhamReturn`, `removeLastStep`, `stopAndReset` | — | — |
| `HMI._chartOptions` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3499) | 1 | HMI / operador | Bajo; interno; síncrona | `_initCharts` | — | — |
| `HMI._initCharts` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3516) | 13 | HMI / operador | Bajo; interno; síncrona | `init` | `_chartOptions`, `afterDatasetsDraw`, `arrow`, `label` | — |
| `HMI.label` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3530) | 1 | HMI / operador | Bajo; interno; síncrona | `_initCharts`, `arrow` | — | — |
| `HMI.arrow` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3536) | 1 | HMI / operador | Bajo; interno; síncrona | `_initCharts`, `draw` | `label` | — |
| `HMI.afterDatasetsDraw` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3579) | 2 | HMI / operador | Bajo; interno; síncrona | `_initCharts` | — | — |
| `HMI.resizeCharts` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3603) | 3 | HMI / operador | Bajo; interno; síncrona | `toggleCinema` | — | — |
| `HMI.clearTelemetryTrail` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3607) | 2 | HMI / operador | Bajo; interno; síncrona | `addStep`, `confirmOckhamReturn`, `start`, `stopAndReset` | `renderMetrics`, `update` | — |
| `HMI.applyCapabilities` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3612) | 4 | HMI / operador | Bajo; interno; síncrona | `applySnapshot` | `applyConfig` | — |
| `HMI.applyConfig` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3616) | 4 | HMI / operador | Bajo; interno; síncrona | `applyCapabilities`, `consumeBackend`, `init` | `renderVectorialSafety` | — |
| `HMI.applyVectorialTelemetry` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3623) | 2 | HMI / operador | Bajo; interno; síncrona | `applyBackendTelemetry` | `renderVectorialSafety` | — |
| `HMI.vectorialReadiness` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3627) | 14 | HMI / operador | Bajo; interno; síncrona | `renderVectorialSafety`, `start` | — | estado |
| `HMI.renderVectorialSafety` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3640) | 5 | HMI / operador | Bajo; interno; síncrona | `applyConfig`, `applyVectorialTelemetry`, `configureForm`, `failRoute`, `init`, `onMission`, `stopAndReset` | `vectorialReadiness` | — |
| `HMI.updateLabels` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3650) | 1 | HMI / operador | Bajo; interno; síncrona | `addStep`, `init`, `removeLastStep`, `stopAndReset` | `activeMode`, `configureForm`, `drawPlan` | — |
| `HMI.completarEjeRectangular` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3654) | 8 | HMI / operador | Bajo; interno; síncrona | `init` | — | — |
| `HMI.updatePreview` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3668) | 1 | HMI / operador | Bajo; interno; síncrona | `addStep`, `configureForm`, `init`, `removeLastStep`, `stopAndReset` | `activeMode` | — |
| `HMI.addStep` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3671) | 7 | HMI / operador | Bajo; interno; cola/evento | `init` | `activeMode`, `add`, `autoScale`, `clearTelemetryTrail`, `createLogicalStep`, `expandExecutionWaypoints`, `updateLabels`, `updateListUI`, `updateModeLock`, `updatePreview` | telemetría |
| `HMI.removeLastStep` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3697) | 4 | HMI / operador | Bajo; interno; cola/evento | `init` | `add`, `autoScale`, `updateLabels`, `updateListUI`, `updateModeLock`, `updatePreview` | telemetría |
| `HMI.appendLimited` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3709) | 3 | HMI / operador | Bajo; interno; cola/evento | `expandExecutionWaypoints` | — | — |
| `HMI.updateListUI` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3716) | 10 | HMI / operador | Bajo; interno; cola/evento | `addStep`, `applyConnection`, `applySession`, `cancelOckhamReturn`, `clearRobotMemory`, `confirmOckhamReturn`, `failRoute`, `init`, `ockhamReturn`, `onMission`, `removeLastStep`, `start`, `stopAndReset` | — | — |
| `HMI.drawPlan` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3731) | 16 | HMI / operador | Bajo; interno; cola/evento | `applyConnection`, `applySession`, `cancelOckhamReturn`, `clearRobotMemory`, `confirmOckhamReturn`, `init`, `ockhamReturn`, `onMission`, `start`, `updateLabels` | `activeMode`, `buildVisualization`, `update` | — |
| `HMI.autoScale` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3760) | 7 | HMI / operador | Bajo; interno; cola/evento | `addStep`, `ockhamReturn`, `removeLastStep`, `returnRecording` | `activeMode`, `update` | — |
| `HMI.updateTelemetryMap` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3776) | 4 | HMI / operador | Bajo; interno; síncrona | `_process` | `renderMetrics`, `update` | — |
| `HMI.renderMetrics` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3790) | 5 | HMI / operador | Bajo; interno; cola/evento | `clearTelemetryTrail`, `updateTelemetryMap` | `logicalEndpoint` | telemetría |
| `HMI.preview` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3800) | 1 | HMI / operador | Bajo; interno; síncrona | `init` | `toggleCinema` | — |
| `HMI.toggleCinema` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3803) | 6 | HMI / operador | Bajo; interno; síncrona | `init`, `preview` | `_syncCinemaButton`, `resizeCharts` | — |
| `HMI._syncCinemaButton` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3819) | 7 | HMI / operador | Medio; interno; síncrona | `toggleCinema` | `add` | — |
| `HMI.lockTabs` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3843) | 2 | HMI / operador | Bajo; interno; síncrona | `confirmOckhamReturn`, `start` | `add` | — |
| `HMI.unlockTabs` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3849) | 1 | HMI / operador | Bajo; interno; síncrona | `applyConnection`, `clearRobotMemory`, `failRoute`, `init`, `onMission`, `start`, `stopAndReset` | — | — |
| `HMI.start` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3853) | 12 | HMI / operador | Medio; interno; cola/evento | `__init__`, `_on_robot_message`, `_schedule_disconnect_grace`, `close_application`, `connect`, `init`, `main`, `manual_start`, `reconnect` | `activeMode`, `add`, `api`, `clearTelemetryTrail`, `drawPlan`, `lockTabs`, `missionPayload`, `unlockTabs`, `updateListUI`, `vectorialReadiness` | — |
| `HMI.onRobotEvent` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3879) | 1 | HMI / operador | Bajo; interno; síncrona | `consumeBackend` | — | — |
| `HMI.onMission` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3880) | 22 | HMI / operador | Medio; interno; síncrona | `consumeBackend`, `returnRecording` | `add`, `drawPlan`, `failRoute`, `renderVectorialSafety`, `unlockTabs`, `updateListUI` | — |
| `HMI.onFsmIdle` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3918) | 1 | HMI / operador | Bajo; interno; síncrona | `_process` | — | — |
| `HMI.failRoute` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3919) | 1 | HMI / operador | Bajo; interno; síncrona | `applyConnection`, `applySession`, `init`, `onMission` | `add`, `renderVectorialSafety`, `unlockTabs`, `updateListUI` | — |
| `HMI.stopAndReset` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3925) | 7 | HMI / operador | Medio; interno; cola/evento | `init` | `add`, `api`, `clearTelemetryTrail`, `renderVectorialSafety`, `unlockTabs`, `updateLabels`, `updateListUI`, `updateModeLock`, `updatePreview` | telemetría |
| `HMI.clearRobotMemory` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3950) | 5 | HMI / operador | Medio; interno; asíncrona | `init` | `add`, `api`, `drawPlan`, `unlockTabs`, `updateListUI` | — |
| `HMI.ockhamReturn` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3962) | 10 | HMI / operador | Medio; interno; cola/evento | `init` | `add`, `api`, `autoScale`, `drawPlan`, `updateListUI`, `updateModeLock` | — |
| `HMI.confirmOckhamReturn` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L3998) | 5 | HMI / operador | Medio; interno; asíncrona | `init` | `add`, `api`, `clearTelemetryTrail`, `drawPlan`, `lockTabs`, `updateListUI` | — |
| `HMI.cancelOckhamReturn` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4011) | 2 | HMI / operador | Bajo; interno; cola/evento | `init` | `add`, `drawPlan`, `updateListUI` | — |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4024) | 7 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4065) | 4 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | `deleteRecording`, `end`, `move`, `renderRecording`, `returnRecording`, `setCapability`, `start` | — |
| `HMI.setCapability` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4086) | 11 | HMI / operador | Bajo; interno; síncrona | `applyBackendTelemetry`, `init` | `end` | — |
| `HMI._pos` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4097) | 3 | HMI / operador | Bajo; interno; síncrona | `move` | — | — |
| `HMI._sendManual` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4102) | 11 | HMI / operador | Medio; interno; síncrona | `move`, `start` | `add`, `end`, `manual` | — |
| `HMI.start` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4115) | 20 | HMI / operador | Medio; interno; síncrona | `__init__`, `_on_robot_message`, `_schedule_disconnect_grace`, `close_application`, `connect`, `init`, `main`, `manual_start`, `reconnect` | `_resetVis`, `_sendManual`, `add`, `manual`, `move` | estado |
| `HMI.move` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4142) | 13 | HMI / operador | Medio; interno; síncrona | `init`, `start` | `_bars`, `_dirName`, `_pos`, `_sendManual`, `end` | — |
| `HMI.end` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4168) | 5 | HMI / operador | Medio; interno; síncrona | `_sendManual`, `init`, `move`, `setCapability` | `_bars`, `_resetVis`, `add`, `manual`, `renderRecording` | — |
| `HMI.renderRecording` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4180) | 33 | HMI / operador | Bajo; interno; síncrona | `deleteRecording`, `end`, `init` | `draw` | — |
| `HMI.draw` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4195) | 3 | HMI / operador | Bajo; interno; síncrona | `addPt`, `cFrame`, `clearAll`, `genRoute`, `init`, `initEv`, `rPts`, `rSteps`, `renderRecording`, `resetV`, `rmPt`, `rsz`, `stopC` | — | — |
| `HMI.returnRecording` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4211) | 6 | HMI / operador | Medio; interno; cola/evento | `init` | `add`, `api`, `autoScale`, `onMission` | telemetría |
| `HMI.deleteRecording` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4226) | 5 | HMI / operador | Bajo; interno; asíncrona | `init` | `add`, `api`, `renderRecording` | — |
| `HMI._resetVis` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4232) | 1 | HMI / operador | Bajo; interno; síncrona | `end`, `start` | — | — |
| `HMI._bars` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4238) | 5 | HMI / operador | Bajo; interno; síncrona | `end`, `move` | — | — |
| `HMI._dirName` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4252) | 18 | HMI / operador | Bajo; interno; síncrona | `move` | — | — |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L4264) | 30 | HMI / operador | Alto; sin llamada interna detectada; asíncrona | — | `add`, `clear`, `closeApplication`, `downloadTXT`, `end`, `failRoute`, `initChart`, `loadHistory`, `send`, `setFilter`, `toggleCinema`, `unlockTabs`, `update` | telemetría, parada/cierre |
