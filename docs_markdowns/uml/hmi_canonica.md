# UML funcional: `hmi/canonica`

Funciones detectadas: **74**. Tipos detectados: **0**.

## Grafo de llamadas

```mermaid
flowchart LR
  subgraph f0["index.html"]
    nee1a6a0b78["initChart()"]
    n499fcf91a7["actualizarBarra()"]
    naa761ddf13["init()"]
    n5d8de27216["api(path, options={})"]
    n1eb30b32f8["closeApplication(force=false)"]
    ncbda059a58["connectBackend()"]
    n24e3573506["consumeBackend(message)"]
    n7dc0415ad7["applyBackendTelemetry(raw)"]
    nd786ec0a0d["send(obj)"]
    n01e34424b2["sendMove(x, y)"]
    nff3745dc1b["init()"]
    nfeaa5344ac["setState(label,ready)"]
    n310ca3d0a6["applyTelemetry(raw)"]
    ne528bc1fbb["onRobotEvent(event)"]
    n83015f61e2["init()"]
    nee2f53d786["change(action)"]
    nafecb1f816["applySnapshot(snapshot)"]
    nd11d8a2d86["applyConnection(connection)"]
    n620364c7a2["applySession(session)"]
    n50e280aa82["applyIdentity(telemetry)"]
    nb443efa380["esc(value)"]
    n03da7d2bb2["loadHistory()"]
    n09b87c3718["init()"]
    n495824e449["add(level, message)"]
    n358fc686e6["clear()"]
    n51536db22f["setFilter(l, a)"]
    n9a0d149461["downloadTXT()"]
    n005287f472["init()"]
    nf165946cf2["_ticks(gId, n, rI, rO)"]
    n9c497b2954["loop(now)"]
    n874d80cedd["init()"]
    nf873e7be72["update(t)"]
    n0eb63532ba["render()"]
    n28404d21e3["_process(t)"]
    n46a8c63678["_uiTelem()"]
    nefe53ae67d["_uiPos()"]
    nd3ec81537a["_ui()"]
    nc2ea51e326["update(s)"]
    na03b402ce9["init()"]
    ndb9606cca3["_chartOptions()"]
    na593ff6d29["_initCharts()"]
    n35690a4881["resizeCharts()"]
    n57582c23f7["clearTelemetryTrail()"]
    nc857e445b1["updateLabels()"]
    n9f3d4efa0a["updatePreview()"]
    n62d29c02ec["addStep()"]
    n2376ea587c["removeLastStep()"]
    ne0a576a097["appendLimited(x0,y0,x1,y1,label)"]
    n4aed01ac25["updateListUI()"]
    n8b1e31f0fc["drawPlan()"]
    nb2ee4fda87["autoScale()"]
    nf97041cdcd["updateTelemetryMap(posX, posY, orientacion)"]
    n5c29d8c5ab["toggleCinema(cardId)"]
    n12c45c1d06["lockTabs()"]
    n455d72224e["unlockTabs()"]
    n5dda903873["start()"]
    na09d5ae582["onRobotEvent(event)"]
    nf49f37b09f["onMission(mission)"]
    n5146fd5912["onFsmIdle()"]
    n57eaee319e["failRoute(reason)"]
    naf210a7a1c["stopAndReset()"]
    nc580fabe2b["clearRobotMemory()"]
    n755859acce["ockhamReturn()"]
    nac06183909["init()"]
    n3a8d0d7387["init()"]
    n67a1ad1741["_pos(e)"]
    n1415fcf322["_sendManual(pwmL, pwmR, dirL, dirR, force=false)"]
    n7044b57d0e["start(e)"]
    n8b1ad5be84["move(e)"]
    n2c9fdf6d34["end()"]
    n2ed59fa1cc["_resetVis()"]
    nc6792f166d["_bars(pL,pR,dL,dR)"]
    n63d08ce8c2["_dirName(a,m)"]
    n13665709c2["init()"]
  end
  n005287f472 --> n9c497b2954
  n005287f472 --> nf165946cf2
  n01e34424b2 --> n495824e449
  n01e34424b2 --> n5d8de27216
  n03da7d2bb2 --> n495824e449
  n03da7d2bb2 --> n5d8de27216
  n03da7d2bb2 --> nb443efa380
  n12c45c1d06 --> n495824e449
  n13665709c2 --> n03da7d2bb2
  n13665709c2 --> n1eb30b32f8
  n13665709c2 --> n2c9fdf6d34
  n13665709c2 --> n358fc686e6
  n13665709c2 --> n495824e449
  n13665709c2 --> n51536db22f
  n13665709c2 --> n5c29d8c5ab
  n13665709c2 --> n9a0d149461
  n13665709c2 --> nd786ec0a0d
  n13665709c2 --> nee1a6a0b78
  n13665709c2 --> nf873e7be72
  n1415fcf322 --> nd786ec0a0d
  n1eb30b32f8 --> n495824e449
  n1eb30b32f8 --> n5d8de27216
  n2376ea587c -.-> n495824e449
  n2376ea587c -.-> n4aed01ac25
  n2376ea587c -.-> n8b1e31f0fc
  n2376ea587c -.-> nb2ee4fda87
  n24e3573506 --> n03da7d2bb2
  n24e3573506 --> n495824e449
  n24e3573506 --> n620364c7a2
  n24e3573506 --> n7dc0415ad7
  n24e3573506 --> nafecb1f816
  n24e3573506 --> nd11d8a2d86
  n24e3573506 --> ne528bc1fbb
  n24e3573506 --> nf49f37b09f
  n28404d21e3 --> n46a8c63678
  n28404d21e3 --> n495824e449
  n28404d21e3 --> n5146fd5912
  n28404d21e3 --> nefe53ae67d
  n28404d21e3 --> nf873e7be72
  n28404d21e3 --> nf97041cdcd
  n2c9fdf6d34 --> n1415fcf322
  n2c9fdf6d34 --> n2ed59fa1cc
  n2c9fdf6d34 --> nc6792f166d
  n310ca3d0a6 --> n495824e449
  n310ca3d0a6 --> nfeaa5344ac
  n3a8d0d7387 --> n2c9fdf6d34
  n3a8d0d7387 --> n5dda903873
  n3a8d0d7387 --> n8b1ad5be84
  n46a8c63678 --> nf873e7be72
  n499fcf91a7 --> nf873e7be72
  n57582c23f7 --> nf873e7be72
  n57eaee319e --> n455d72224e
  n57eaee319e --> n495824e449
  n57eaee319e --> n4aed01ac25
  n5c29d8c5ab --> n35690a4881
  n5dda903873 -.-> n12c45c1d06
  n5dda903873 -.-> n495824e449
  n5dda903873 -.-> n4aed01ac25
  n5dda903873 -.-> n5d8de27216
  n5dda903873 -.-> n8b1e31f0fc
  n62d29c02ec -.-> n495824e449
  n62d29c02ec -.-> n4aed01ac25
  n62d29c02ec -.-> n57582c23f7
  n62d29c02ec -.-> n8b1e31f0fc
  n62d29c02ec -.-> nb2ee4fda87
  n62d29c02ec -.-> ne0a576a097
  n7044b57d0e --> n1415fcf322
  n7044b57d0e --> n495824e449
  n7044b57d0e --> n8b1ad5be84
  n755859acce -.-> n12c45c1d06
  n755859acce -.-> n495824e449
  n755859acce -.-> n4aed01ac25
  n755859acce -.-> n57582c23f7
  n755859acce -.-> n5d8de27216
  n755859acce -.-> n8b1e31f0fc
  n755859acce -.-> nb2ee4fda87
  n7dc0415ad7 --> n28404d21e3
  n7dc0415ad7 --> n310ca3d0a6
  n7dc0415ad7 --> n495824e449
  n7dc0415ad7 --> n50e280aa82
  n7dc0415ad7 --> nf873e7be72
  n83015f61e2 --> n03da7d2bb2
  n83015f61e2 --> nee2f53d786
  n874d80cedd --> n0eb63532ba
  n8b1ad5be84 --> n1415fcf322
  n8b1ad5be84 --> n2c9fdf6d34
  n8b1ad5be84 --> n63d08ce8c2
  n8b1ad5be84 --> n67a1ad1741
  n8b1ad5be84 --> nc6792f166d
  n8b1e31f0fc -.-> nf873e7be72
  n9c497b2954 --> n495824e449
  na03b402ce9 --> n2376ea587c
  na03b402ce9 --> n5dda903873
  na03b402ce9 --> n62d29c02ec
  na03b402ce9 --> n755859acce
  na03b402ce9 --> n9f3d4efa0a
  na03b402ce9 --> na593ff6d29
  na03b402ce9 --> naf210a7a1c
  na03b402ce9 --> nc580fabe2b
  na03b402ce9 --> nc857e445b1
  na593ff6d29 --> ndb9606cca3
  naa761ddf13 --> n495824e449
  naa761ddf13 --> ncbda059a58
  naf210a7a1c -.-> n455d72224e
  naf210a7a1c -.-> n495824e449
  naf210a7a1c -.-> n4aed01ac25
  naf210a7a1c -.-> n57582c23f7
  naf210a7a1c -.-> n5d8de27216
  naf210a7a1c -.-> n8b1e31f0fc
  nafecb1f816 --> n50e280aa82
  nafecb1f816 --> n620364c7a2
  nafecb1f816 --> nd11d8a2d86
  nb2ee4fda87 -.-> nf873e7be72
  nc2ea51e326 --> n495824e449
  nc580fabe2b --> n455d72224e
  nc580fabe2b --> n495824e449
  nc580fabe2b --> n4aed01ac25
  nc580fabe2b --> n5d8de27216
  nc580fabe2b --> n8b1e31f0fc
  nc857e445b1 --> n9f3d4efa0a
  ncbda059a58 --> n24e3573506
  ncbda059a58 --> n495824e449
  ncbda059a58 --> n5d8de27216
  nd11d8a2d86 --> nd3ec81537a
  nd3ec81537a --> n0eb63532ba
  nd786ec0a0d --> n495824e449
  nd786ec0a0d --> n5d8de27216
  ne528bc1fbb --> n495824e449
  ne528bc1fbb --> nfeaa5344ac
  nee1a6a0b78 --> n01e34424b2
  nee1a6a0b78 --> n499fcf91a7
  nee1a6a0b78 --> nf873e7be72
  nee2f53d786 --> n495824e449
  nee2f53d786 --> n5d8de27216
  nee2f53d786 --> nafecb1f816
  nf49f37b09f --> n455d72224e
  nf49f37b09f --> n495824e449
  nf49f37b09f --> n4aed01ac25
  nf49f37b09f --> n57eaee319e
  nf49f37b09f --> n8b1e31f0fc
  nf873e7be72 --> n0eb63532ba
  nf97041cdcd --> nf873e7be72
  nff3745dc1b --> n495824e449
  nff3745dc1b --> nd786ec0a0d
  nff3745dc1b --> nfeaa5344ac
  classDef alto fill:#5b1f2a,stroke:#ff7a7a,color:#fff
  classDef medio fill:#4a3717,stroke:#ffca67,color:#fff
  classDef bajo fill:#123b3a,stroke:#39e6aa,color:#fff
  class nee1a6a0b78 medio
  class n499fcf91a7 bajo
  class naa761ddf13 bajo
  class n5d8de27216 bajo
  class n1eb30b32f8 medio
  class ncbda059a58 bajo
  class n24e3573506 medio
  class n7dc0415ad7 alto
  class nd786ec0a0d medio
  class n01e34424b2 bajo
  class nff3745dc1b bajo
  class nfeaa5344ac bajo
  class n310ca3d0a6 alto
  class ne528bc1fbb bajo
  class n83015f61e2 bajo
  class nee2f53d786 bajo
  class nafecb1f816 bajo
  class nd11d8a2d86 medio
  class n620364c7a2 medio
  class n50e280aa82 bajo
  class nb443efa380 bajo
  class n03da7d2bb2 medio
  class n09b87c3718 bajo
  class n495824e449 bajo
  class n358fc686e6 bajo
  class n51536db22f bajo
  class n9a0d149461 bajo
  class n005287f472 bajo
  class nf165946cf2 bajo
  class n9c497b2954 alto
  class n874d80cedd bajo
  class nf873e7be72 bajo
  class n0eb63532ba medio
  class n28404d21e3 bajo
  class n46a8c63678 bajo
  class nefe53ae67d bajo
  class nd3ec81537a bajo
  class nc2ea51e326 medio
  class na03b402ce9 medio
  class ndb9606cca3 bajo
  class na593ff6d29 bajo
  class n35690a4881 bajo
  class n57582c23f7 bajo
  class nc857e445b1 bajo
  class n9f3d4efa0a bajo
  class n62d29c02ec bajo
  class n2376ea587c bajo
  class ne0a576a097 bajo
  class n4aed01ac25 bajo
  class n8b1e31f0fc bajo
  class nb2ee4fda87 bajo
  class nf97041cdcd bajo
  class n5c29d8c5ab bajo
  class n12c45c1d06 bajo
  class n455d72224e bajo
  class n5dda903873 medio
  class na09d5ae582 bajo
  class nf49f37b09f medio
  class n5146fd5912 bajo
  class n57eaee319e bajo
  class naf210a7a1c medio
  class nc580fabe2b medio
  class n755859acce medio
  class nac06183909 bajo
  class n3a8d0d7387 bajo
  class n67a1ad1741 bajo
  class n1415fcf322 medio
  class n7044b57d0e medio
  class n8b1ad5be84 medio
  class n2c9fdf6d34 medio
  class n2ed59fa1cc bajo
  class nc6792f166d bajo
  class n63d08ce8c2 bajo
  class n13665709c2 alto
```

Fuentes: [Mermaid](mermaid/hmi_canonica.mmd) · [PlantUML](plantuml/hmi_canonica.puml). Las flechas continuas son llamadas síncronas; las discontinuas representan asincronía, eventos o colas. El color del nodo indica riesgo estático.

## Inventario

| Función | Archivo | CC | Propietario | Riesgo/estado | Entra desde | Sale hacia | Estado compartido |
|---|---|---:|---|---|---|---|---|
| `HMI.initChart` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1312) | 5 | HMI / operador | Medio; interno; asíncrona | `init` | `actualizarBarra`, `sendMove`, `update` | — |
| `HMI.actualizarBarra` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1408) | 3 | HMI / operador | Bajo; interno; síncrona | `initChart` | `update` | — |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1444) | 2 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | `add`, `connectBackend` | — |
| `HMI.api` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1451) | 6 | HMI / operador | Bajo; interno; asíncrona | `change`, `clearRobotMemory`, `closeApplication`, `connectBackend`, `loadHistory`, `ockhamReturn`, `send`, `sendMove`, `start`, `stopAndReset` | — | — |
| `HMI.closeApplication` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1463) | 10 | HMI / operador | Medio; interno; asíncrona | `init` | `add`, `api`, `close`, `closeApp` | — |
| `HMI.connectBackend` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1488) | 3 | HMI / operador | Bajo; interno; asíncrona | `init` | `add`, `api`, `consumeBackend` | — |
| `HMI.consumeBackend` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1499) | 11 | HMI / operador | Medio; interno; síncrona | `connectBackend` | `add`, `applyBackendTelemetry`, `applyConnection`, `applySession`, `applySnapshot`, `loadHistory`, `onMission`, `onRobotEvent` | telemetría, sesión |
| `HMI.applyBackendTelemetry` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1517) | 141 | HMI / operador | Alto; interno; síncrona | `consumeBackend` | `_process`, `add`, `applyIdentity`, `applyTelemetry`, `update` | estado, telemetría, parada/cierre |
| `HMI.send` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1584) | 7 | HMI / operador | Medio; interno; asíncrona | `_drain_one`, `_run`, `_sendManual`, `init` | `add`, `api` | parada/cierre |
| `HMI.sendMove` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1598) | 4 | HMI / operador | Bajo; interno; síncrona | `initChart` | `add`, `api` | — |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1612) | 4 | HMI / operador | Bajo; sin llamada interna detectada; asíncrona | — | `add`, `send`, `setState` | — |
| `HMI.setState` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1630) | 1 | HMI / operador | Bajo; interno; síncrona | `applyTelemetry`, `init`, `onRobotEvent` | — | — |
| `HMI.applyTelemetry` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1637) | 54 | HMI / operador | Alto; interno; síncrona | `applyBackendTelemetry` | `add`, `setState` | parada/cierre |
| `HMI.onRobotEvent` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1691) | 19 | HMI / operador | Bajo; interno; síncrona | `consumeBackend` | `add`, `setState` | — |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1717) | 1 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | `change`, `loadHistory` | — |
| `HMI.change` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1723) | 4 | HMI / operador | Bajo; interno; asíncrona | `init` | `add`, `api`, `applySnapshot` | — |
| `HMI.applySnapshot` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1733) | 8 | HMI / operador | Bajo; interno; síncrona | `change`, `consumeBackend` | `applyConnection`, `applyIdentity`, `applySession` | telemetría |
| `HMI.applyConnection` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1740) | 5 | HMI / operador | Medio; interno; asíncrona | `applySnapshot`, `consumeBackend` | `_ui` | telemetría |
| `HMI.applySession` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1750) | 2 | HMI / operador | Medio; interno; síncrona | `applySnapshot`, `consumeBackend` | — | sesión |
| `HMI.applyIdentity` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1751) | 4 | HMI / operador | Bajo; interno; síncrona | `applyBackendTelemetry`, `applySnapshot` | — | telemetría |
| `HMI.esc` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1756) | 3 | HMI / operador | Bajo; interno; síncrona | `loadHistory` | — | — |
| `HMI.loadHistory` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1757) | 4 | HMI / operador | Medio; interno; asíncrona | `consumeBackend`, `init` | `add`, `api`, `esc` | telemetría, sesión |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1773) | 1 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `HMI.add` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1774) | 7 | HMI / operador | Bajo; interno; síncrona | `_process`, `addStep`, `applyBackendTelemetry`, `applyTelemetry`, `cancel`, `change`, `clearRobotMemory`, `closeApplication`, `connectBackend`, `consumeBackend`, `dependency_cycles`, `failRoute`, `hlStep`, `init`, `initResize`, `loadHistory`, `lockTabs`, `loop`, `mainW`, `mermaid_for_folder`, `ockhamReturn`, `onDown`, `onMission`, `onRobotEvent`, `pL`, `pR`, `removeLastStep`, `send`, `sendMove`, `send_command`, `start`, `startC`, `stopAndReset`, `subscribe`, `update`, `visit` | — | — |
| `HMI.clear` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1791) | 3 | HMI / operador | Bajo; interno; síncrona | `_run`, `init`, `liberarMisionAutonoma`, `start` | — | — |
| `HMI.setFilter` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1797) | 2 | HMI / operador | Bajo; interno; síncrona | `init` | — | — |
| `HMI.downloadTXT` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1798) | 2 | HMI / operador | Bajo; interno; síncrona | `init` | — | estado |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1810) | 2 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | `_ticks`, `loop` | — |
| `HMI._ticks` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1823) | 4 | HMI / operador | Bajo; interno; síncrona | `init` | — | — |
| `HMI.loop` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1833) | 19 | HMI / operador | Alto; entrada/framework; síncrona | `init` | `add` | estado, telemetría |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1883) | 1 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | `render` | — |
| `HMI.update` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1884) | 13 | HMI / operador | Bajo; interno; síncrona | `_process`, `_uiTelem`, `actualizarBarra`, `applyBackendTelemetry`, `autoScale`, `clearTelemetryTrail`, `create_app`, `drawPlan`, `init`, `initChart`, `updateTelemetryMap` | `render` | — |
| `HMI.render` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1905) | 10 | HMI / operador | Medio; interno; síncrona | `_ui`, `init`, `main`, `update` | — | telemetría |
| `HMI._process` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1924) | 6 | HMI / operador | Bajo; interno; síncrona | `applyBackendTelemetry` | `_uiPos`, `_uiTelem`, `add`, `onFsmIdle`, `update`, `updateTelemetryMap` | estado, telemetría |
| `HMI._uiTelem` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1941) | 11 | HMI / operador | Bajo; interno; síncrona | `_process` | `update` | estado, telemetría |
| `HMI._uiPos` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1978) | 3 | HMI / operador | Bajo; interno; síncrona | `_process` | — | telemetría |
| `HMI._ui` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L1993) | 2 | HMI / operador | Bajo; interno; síncrona | `applyConnection` | `render` | — |
| `HMI.update` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2001) | 5 | HMI / operador | Medio; interno; síncrona | `_process`, `_uiTelem`, `actualizarBarra`, `applyBackendTelemetry`, `autoScale`, `clearTelemetryTrail`, `create_app`, `drawPlan`, `init`, `initChart`, `updateTelemetryMap` | `add` | parada/cierre |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2016) | 1 | HMI / operador | Medio; sin llamada interna detectada; síncrona | — | `_initCharts`, `addStep`, `clearRobotMemory`, `ockhamReturn`, `removeLastStep`, `start`, `stopAndReset`, `updateLabels`, `updatePreview` | — |
| `HMI._chartOptions` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2038) | 1 | HMI / operador | Bajo; interno; síncrona | `_initCharts` | — | — |
| `HMI._initCharts` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2055) | 5 | HMI / operador | Bajo; interno; síncrona | `init` | `_chartOptions` | — |
| `HMI.resizeCharts` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2080) | 3 | HMI / operador | Bajo; interno; síncrona | `toggleCinema` | — | — |
| `HMI.clearTelemetryTrail` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2084) | 2 | HMI / operador | Bajo; interno; síncrona | `addStep`, `ockhamReturn`, `stopAndReset` | `update` | — |
| `HMI.updateLabels` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2088) | 2 | HMI / operador | Bajo; interno; síncrona | `init` | `updatePreview` | — |
| `HMI.updatePreview` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2102) | 4 | HMI / operador | Bajo; interno; síncrona | `init`, `updateLabels` | — | — |
| `HMI.addStep` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2109) | 8 | HMI / operador | Bajo; interno; cola/evento | `init` | `add`, `appendLimited`, `autoScale`, `clearTelemetryTrail`, `drawPlan`, `updateListUI` | telemetría |
| `HMI.removeLastStep` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2137) | 4 | HMI / operador | Bajo; interno; cola/evento | `init` | `add`, `autoScale`, `drawPlan`, `updateListUI` | telemetría |
| `HMI.appendLimited` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2148) | 2 | HMI / operador | Bajo; interno; cola/evento | `addStep` | — | — |
| `HMI.updateListUI` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2155) | 9 | HMI / operador | Bajo; interno; cola/evento | `addStep`, `clearRobotMemory`, `failRoute`, `ockhamReturn`, `onMission`, `removeLastStep`, `start`, `stopAndReset` | — | — |
| `HMI.drawPlan` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2169) | 2 | HMI / operador | Bajo; interno; cola/evento | `addStep`, `clearRobotMemory`, `ockhamReturn`, `onMission`, `removeLastStep`, `start`, `stopAndReset` | `update` | — |
| `HMI.autoScale` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2174) | 4 | HMI / operador | Bajo; interno; cola/evento | `addStep`, `ockhamReturn`, `removeLastStep` | `update` | — |
| `HMI.updateTelemetryMap` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2189) | 4 | HMI / operador | Bajo; interno; síncrona | `_process` | `update` | — |
| `HMI.toggleCinema` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2202) | 1 | HMI / operador | Bajo; interno; síncrona | `init` | `resizeCharts` | — |
| `HMI.lockTabs` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2207) | 2 | HMI / operador | Bajo; interno; síncrona | `ockhamReturn`, `start` | `add` | — |
| `HMI.unlockTabs` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2213) | 1 | HMI / operador | Bajo; interno; síncrona | `clearRobotMemory`, `failRoute`, `onMission`, `stopAndReset` | — | — |
| `HMI.start` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2217) | 8 | HMI / operador | Medio; interno; cola/evento | `__init__`, `close_application`, `connect`, `init`, `main`, `reconnect` | `add`, `api`, `drawPlan`, `lockTabs`, `updateListUI` | — |
| `HMI.onRobotEvent` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2232) | 1 | HMI / operador | Bajo; interno; síncrona | `consumeBackend` | — | — |
| `HMI.onMission` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2233) | 12 | HMI / operador | Medio; interno; síncrona | `consumeBackend` | `add`, `drawPlan`, `failRoute`, `unlockTabs`, `updateListUI` | — |
| `HMI.onFsmIdle` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2255) | 1 | HMI / operador | Bajo; interno; síncrona | `_process` | — | — |
| `HMI.failRoute` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2256) | 1 | HMI / operador | Bajo; interno; síncrona | `onMission` | `add`, `unlockTabs`, `updateListUI` | — |
| `HMI.stopAndReset` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2261) | 7 | HMI / operador | Medio; interno; cola/evento | `init` | `add`, `api`, `clearTelemetryTrail`, `drawPlan`, `unlockTabs`, `updateListUI` | telemetría |
| `HMI.clearRobotMemory` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2282) | 4 | HMI / operador | Medio; interno; asíncrona | `init` | `add`, `api`, `drawPlan`, `unlockTabs`, `updateListUI` | — |
| `HMI.ockhamReturn` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2292) | 7 | HMI / operador | Medio; interno; cola/evento | `init` | `add`, `api`, `autoScale`, `clearTelemetryTrail`, `drawPlan`, `lockTabs`, `updateListUI` | telemetría |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2314) | 1 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2333) | 3 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | `end`, `move`, `start` | — |
| `HMI._pos` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2347) | 3 | HMI / operador | Bajo; interno; síncrona | `move` | — | — |
| `HMI._sendManual` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2352) | 8 | HMI / operador | Medio; interno; síncrona | `end`, `move`, `start` | `send` | — |
| `HMI.start` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2361) | 12 | HMI / operador | Medio; interno; síncrona | `__init__`, `close_application`, `connect`, `init`, `main`, `reconnect` | `_sendManual`, `add`, `move` | estado |
| `HMI.move` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2372) | 13 | HMI / operador | Medio; interno; síncrona | `init`, `start` | `_bars`, `_dirName`, `_pos`, `_sendManual`, `end` | — |
| `HMI.end` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2398) | 3 | HMI / operador | Medio; interno; síncrona | `guardarCheckpoint`, `inicializarPersistenciaMision`, `init`, `liberarMisionAutonoma`, `move` | `_bars`, `_resetVis`, `_sendManual` | — |
| `HMI._resetVis` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2408) | 1 | HMI / operador | Bajo; interno; síncrona | `end` | — | — |
| `HMI._bars` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2414) | 5 | HMI / operador | Bajo; interno; síncrona | `end`, `move` | — | — |
| `HMI._dirName` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2428) | 18 | HMI / operador | Bajo; interno; síncrona | `move` | — | — |
| `HMI.init` | [`desktop_app/robot_app/hmi/index.html`](../../desktop_app/robot_app/hmi/index.html#L2440) | 31 | HMI / operador | Alto; sin llamada interna detectada; asíncrona | — | `add`, `clear`, `closeApplication`, `downloadTXT`, `end`, `initChart`, `loadHistory`, `send`, `setFilter`, `toggleCinema`, `update` | telemetría, parada/cierre |
