# UML funcional: `herramientas/scripts`

Funciones detectadas: **72**. Tipos detectados: **0**.

## Grafo de llamadas

```mermaid
flowchart LR
  subgraph f0["SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html"]
    n4cbf77c493["fm(n,d=1)"]
    n593102974c["nLetter()"]
    n14f1ef6e7b["toast(m,t='info')"]
    n622aa6745d["addPt()"]
    n9c3c116062["rmPt(l)"]
    n01061f78b6["clearAll()"]
    n198e0e1e49["rPts()"]
    n315958bad0["genRoute()"]
    n4671b319ba["rChips()"]
    nc767be9aa0["calcSteps()"]
    n1fe21f1388["buildCSegs()"]
    ndf4f4132cd["rSteps()"]
    n09bc0b5b13["draw()"]
    nb71483d30e["arrow(ax,ay,dx,dy,col)"]
    nf82920201d["toggleCine()"]
    n3cf472952d["startC()"]
    n1aaec6a3f2["stopC()"]
    nee051b40aa["cFrame(t)"]
    nbfbc2048fe["hlStep(i)"]
    nc0f3636e27["upSt()"]
    n93bfeb5085["resetV()"]
    n505bfe6358["initResize()"]
    n29119f8e5f["mainW()"]
    n1a802d9f8b["pL()"]
    ne07e262822["pR()"]
    ne7fd0cdcfd["onDown(type,e)"]
    ndcc2caa7d0["onMove(cx2)"]
    n6703467174["onUp()"]
    naa0319038c["initEv()"]
    n756c4c6df7["rsz()"]
    n4df1dd2897["init()"]
  end
  subgraph f1["consultar_db.py"]
    n49043c79a3["connect_db(db_path: str)"]
    n5e0a00e68d["show_summary(conn: sqlite3.Connection)"]
    nbf82b6eba0["list_sessions(conn: sqlite3.Connection, limit: int)"]
    n9c7418a419["list_commands(conn: sqlite3.Connection, limit: int)"]
    n2434d50d95["list_events(conn: sqlite3.Connection, limit: int)"]
    n34bb24da56["list_telemetry(conn: sqlite3.Connection, limit: int)"]
    nce7e9f10a5["search_all(conn: sqlite3.Connection, term: str)"]
    n48e4577564["main()"]
  end
  subgraph f2["generar_catalogo.py"]
    nafd356db32["active_files()"]
    nbe76a713c4["subsystem(path: Path)"]
    n7c3b78fe6d["language(path: Path)"]
    n1cb0d8a18e["run_ctags(files: list[Path])"]
    n8581868fe2["embedded_html_functions(path: Path)"]
    n6c940fadb8["find_end(start: int)"]
    n80d4f33be9["scripted_functions(files: list[Path])"]
    n771ad39ac6["visibility(item: dict[str, Any], source_line: str)"]
    nf0f6e15fe6["normalize_tags(tags: list[dict[str, Any]])"]
    nb4286ff169["_function_body(item: dict[str, Any])"]
    nb9f51e6533["_owner(folder: str)"]
    n82d755956d["_platform(folder: str)"]
    n9e90b74861["_complexity(body: str)"]
    ndf31e443c8["_interaction(body: str)"]
    n982ba39366["_shared_state(body: str)"]
    nb32f8aebc6["infer_calls(functions: list[dict[str, Any]], routes: list[dict[str, Any]])"]
    n03f09ef830["dependency_cycles(functions: list[dict[str, Any]])"]
    n20291b9270["visit(node: str)"]
    n256df7dfce["extract_routes(files: list[Path])"]
    na83aea5101["node_id(item: dict[str, Any])"]
    ncfaa75265c["mermaid_for_folder(folder: str, functions: list[dict[str, Any]])"]
    n33bac7ac43["plantuml_for_folder(folder: str, functions: list[dict[str, Any]])"]
    ne8f50e770a["write_markdown(functions: list[dict[str, Any]], types: list[dict[str, Any]])"]
    n709c592fef["main()"]
  end
  subgraph f3["generar_resumen_sqlite.py"]
    n9eab49d8ad["rows(connection: sqlite3.Connection, query: str, parameters: tuple = ())"]
    ncb8c29961f["elapsed_seconds(value: str, origin: datetime)"]
    n1c9814c081["main()"]
  end
  subgraph f4["renderizar_uml.py"]
    n5483b456cb["download_if_needed()"]
    nf237d76ace["render(format_name: str)"]
    n489ce3258b["main()"]
  end
  subgraph f5["validar_enlaces.py"]
    n3d5d392d32["markdown_files()"]
    n3d26275b4a["normalize_target(raw: str)"]
    n5d2522213b["main()"]
  end
  n01061f78b6 --> n09bc0b5b13
  n01061f78b6 --> n14f1ef6e7b
  n01061f78b6 --> n198e0e1e49
  n01061f78b6 --> n1aaec6a3f2
  n01061f78b6 --> n4671b319ba
  n01061f78b6 --> nc0f3636e27
  n01061f78b6 --> ndf4f4132cd
  n03f09ef830 --> n20291b9270
  n09bc0b5b13 --> nb71483d30e
  n198e0e1e49 --> n09bc0b5b13
  n198e0e1e49 --> n9c3c116062
  n1a802d9f8b --> ne7fd0cdcfd
  n1aaec6a3f2 --> n09bc0b5b13
  n1c9814c081 --> n9eab49d8ad
  n1c9814c081 --> ncb8c29961f
  n29119f8e5f --> ne7fd0cdcfd
  n315958bad0 --> n09bc0b5b13
  n315958bad0 --> n14f1ef6e7b
  n315958bad0 --> n4671b319ba
  n315958bad0 --> nc0f3636e27
  n315958bad0 --> nc767be9aa0
  n315958bad0 --> ndf4f4132cd
  n33bac7ac43 --> na83aea5101
  n489ce3258b --> n5483b456cb
  n489ce3258b --> nf237d76ace
  n48e4577564 --> n2434d50d95
  n48e4577564 --> n34bb24da56
  n48e4577564 --> n49043c79a3
  n48e4577564 --> n5e0a00e68d
  n48e4577564 --> n9c7418a419
  n48e4577564 --> nbf82b6eba0
  n48e4577564 --> nce7e9f10a5
  n4cbf77c493 --> n593102974c
  n4df1dd2897 --> n09bc0b5b13
  n4df1dd2897 --> n198e0e1e49
  n4df1dd2897 --> n315958bad0
  n4df1dd2897 --> n505bfe6358
  n4df1dd2897 --> n622aa6745d
  n4df1dd2897 --> n756c4c6df7
  n4df1dd2897 --> naa0319038c
  n4df1dd2897 --> nc0f3636e27
  n505bfe6358 --> n29119f8e5f
  n505bfe6358 --> n6703467174
  n505bfe6358 --> n756c4c6df7
  n505bfe6358 --> ndcc2caa7d0
  n505bfe6358 --> ne7fd0cdcfd
  n5d2522213b --> n3d26275b4a
  n5d2522213b --> n3d5d392d32
  n622aa6745d --> n09bc0b5b13
  n622aa6745d --> n14f1ef6e7b
  n622aa6745d --> n198e0e1e49
  n622aa6745d --> n593102974c
  n622aa6745d --> nc0f3636e27
  n709c592fef --> n03f09ef830
  n709c592fef --> n1cb0d8a18e
  n709c592fef --> n256df7dfce
  n709c592fef --> n80d4f33be9
  n709c592fef --> n8581868fe2
  n709c592fef --> nafd356db32
  n709c592fef --> nb32f8aebc6
  n709c592fef --> ne8f50e770a
  n709c592fef --> nf0f6e15fe6
  n756c4c6df7 --> n09bc0b5b13
  n8581868fe2 --> n6c940fadb8
  n93bfeb5085 --> n09bc0b5b13
  n9c3c116062 --> n09bc0b5b13
  n9c3c116062 --> n198e0e1e49
  n9c3c116062 --> n4671b319ba
  n9c3c116062 --> nc0f3636e27
  n9c3c116062 --> nc767be9aa0
  n9c3c116062 --> ndf4f4132cd
  naa0319038c --> n09bc0b5b13
  nb32f8aebc6 --> n82d755956d
  nb32f8aebc6 --> n982ba39366
  nb32f8aebc6 --> n9e90b74861
  nb32f8aebc6 --> nb4286ff169
  nb32f8aebc6 --> nb9f51e6533
  nb32f8aebc6 --> ndf31e443c8
  nc767be9aa0 --> n1fe21f1388
  ncfaa75265c --> na83aea5101
  ndcc2caa7d0 --> n29119f8e5f
  ndcc2caa7d0 --> n756c4c6df7
  ndf4f4132cd --> n09bc0b5b13
  ne07e262822 --> ne7fd0cdcfd
  ne8f50e770a --> n33bac7ac43
  ne8f50e770a --> ncfaa75265c
  nee051b40aa --> n09bc0b5b13
  nee051b40aa --> n14f1ef6e7b
  nee051b40aa --> n1aaec6a3f2
  nee051b40aa --> nbfbc2048fe
  nf0f6e15fe6 --> n771ad39ac6
  nf0f6e15fe6 --> n7c3b78fe6d
  nf0f6e15fe6 --> nbe76a713c4
  nf82920201d --> n1aaec6a3f2
  nf82920201d --> n3cf472952d
  classDef alto fill:#5b1f2a,stroke:#ff7a7a,color:#fff
  classDef medio fill:#4a3717,stroke:#ffca67,color:#fff
  classDef bajo fill:#123b3a,stroke:#39e6aa,color:#fff
  class n4cbf77c493 bajo
  class n593102974c bajo
  class n14f1ef6e7b bajo
  class n622aa6745d bajo
  class n9c3c116062 bajo
  class n01061f78b6 bajo
  class n198e0e1e49 bajo
  class n315958bad0 bajo
  class n4671b319ba bajo
  class nc767be9aa0 bajo
  class n1fe21f1388 bajo
  class ndf4f4132cd bajo
  class n09bc0b5b13 bajo
  class nb71483d30e bajo
  class nf82920201d bajo
  class n3cf472952d medio
  class n1aaec6a3f2 bajo
  class nee051b40aa bajo
  class nbfbc2048fe bajo
  class nc0f3636e27 bajo
  class n93bfeb5085 bajo
  class n505bfe6358 bajo
  class n29119f8e5f bajo
  class n1a802d9f8b bajo
  class ne07e262822 bajo
  class ne7fd0cdcfd bajo
  class ndcc2caa7d0 bajo
  class n6703467174 bajo
  class naa0319038c bajo
  class n756c4c6df7 bajo
  class n4df1dd2897 bajo
  class n49043c79a3 bajo
  class n5e0a00e68d bajo
  class nbf82b6eba0 bajo
  class n9c7418a419 bajo
  class n2434d50d95 bajo
  class n34bb24da56 alto
  class nce7e9f10a5 bajo
  class n48e4577564 bajo
  class nafd356db32 bajo
  class nbe76a713c4 bajo
  class n7c3b78fe6d bajo
  class n1cb0d8a18e bajo
  class n8581868fe2 bajo
  class n6c940fadb8 bajo
  class n80d4f33be9 bajo
  class n771ad39ac6 bajo
  class nf0f6e15fe6 bajo
  class nb4286ff169 bajo
  class nb9f51e6533 bajo
  class n82d755956d bajo
  class n9e90b74861 bajo
  class ndf31e443c8 medio
  class n982ba39366 medio
  class nb32f8aebc6 alto
  class n03f09ef830 bajo
  class n20291b9270 bajo
  class n256df7dfce bajo
  class na83aea5101 bajo
  class ncfaa75265c bajo
  class n33bac7ac43 bajo
  class ne8f50e770a alto
  class n709c592fef bajo
  class n9eab49d8ad bajo
  class ncb8c29961f bajo
  class n1c9814c081 medio
  class n5483b456cb bajo
  class nf237d76ace bajo
  class n489ce3258b bajo
  class n3d5d392d32 bajo
  class n3d26275b4a bajo
  class n5d2522213b bajo
```

Fuentes: [Mermaid](mermaid/herramientas_scripts.mmd) · [PlantUML](plantuml/herramientas_scripts.puml). Las flechas continuas son llamadas síncronas; las discontinuas representan asincronía, eventos o colas. El color del nodo indica riesgo estático.

## Inventario

| Función | Archivo | CC | Propietario | Riesgo/estado | Entra desde | Sale hacia | Estado compartido |
|---|---|---:|---|---|---|---|---|
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.fm` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L187) | 4 | Mantenimiento / herramientas | Bajo; sin llamada interna detectada; síncrona | — | `nLetter` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.nLetter` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L189) | 2 | Mantenimiento / herramientas | Bajo; interno; síncrona | `addPt`, `fm` | — | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.toast` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L192) | 2 | Mantenimiento / herramientas | Bajo; interno; síncrona | `addPt`, `cFrame`, `clearAll`, `genRoute` | — | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.addPt` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L201) | 5 | Mantenimiento / herramientas | Bajo; interno; síncrona | `init` | `draw`, `nLetter`, `rPts`, `toast`, `upSt` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.rmPt` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L212) | 1 | Mantenimiento / herramientas | Bajo; interno; síncrona | `rPts` | `calcSteps`, `draw`, `rChips`, `rPts`, `rSteps`, `upSt` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.clearAll` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L217) | 2 | Mantenimiento / herramientas | Bajo; sin llamada interna detectada; síncrona | — | `draw`, `rChips`, `rPts`, `rSteps`, `stopC`, `toast`, `upSt` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.rPts` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L223) | 2 | Mantenimiento / herramientas | Bajo; interno; síncrona | `addPt`, `clearAll`, `init`, `rmPt` | `draw`, `rmPt` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.genRoute` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L237) | 5 | Mantenimiento / herramientas | Bajo; interno; síncrona | `init` | `calcSteps`, `draw`, `rChips`, `rSteps`, `toast`, `upSt` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.rChips` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L249) | 3 | Mantenimiento / herramientas | Bajo; interno; síncrona | `clearAll`, `genRoute`, `rmPt` | — | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.calcSteps` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L259) | 3 | Mantenimiento / herramientas | Bajo; interno; síncrona | `genRoute`, `rmPt` | `buildCSegs` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.buildCSegs` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L271) | 3 | Mantenimiento / herramientas | Bajo; interno; síncrona | `calcSteps` | — | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.rSteps` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L280) | 8 | Mantenimiento / herramientas | Bajo; interno; síncrona | `clearAll`, `genRoute`, `rmPt` | `draw` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.draw` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L315) | 46 | Mantenimiento / herramientas | Bajo; interno; síncrona | `addPt`, `cFrame`, `clearAll`, `genRoute`, `init`, `initEv`, `rPts`, `rSteps`, `resetV`, `rmPt`, `rsz`, `stopC` | `arrow` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.arrow` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L432) | 2 | Mantenimiento / herramientas | Bajo; interno; síncrona | `draw` | — | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.toggleCine` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L443) | 2 | Mantenimiento / herramientas | Bajo; sin llamada interna detectada; síncrona | — | `startC`, `stopC` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.startC` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L445) | 2 | Mantenimiento / herramientas | Medio; interno; síncrona | `toggleCine` | `add` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.stopC` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L455) | 2 | Mantenimiento / herramientas | Bajo; interno; síncrona | `cFrame`, `clearAll`, `toggleCine` | `draw` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.cFrame` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L465) | 6 | Mantenimiento / herramientas | Bajo; sin llamada interna detectada; síncrona | — | `draw`, `hlStep`, `stopC`, `toast` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.hlStep` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L479) | 2 | Mantenimiento / herramientas | Bajo; interno; síncrona | `cFrame` | `add` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.upSt` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L486) | 1 | Mantenimiento / herramientas | Bajo; interno; síncrona | `addPt`, `clearAll`, `genRoute`, `init`, `rmPt` | — | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.resetV` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L493) | 1 | Mantenimiento / herramientas | Bajo; sin llamada interna detectada; síncrona | — | `draw` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.initResize` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L496) | 13 | Mantenimiento / herramientas | Bajo; interno; síncrona | `init` | `add`, `mainW`, `onDown`, `onMove`, `onUp`, `rsz` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.mainW` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L497) | 3 | Mantenimiento / herramientas | Bajo; interno; síncrona | `initResize`, `onMove` | `add`, `onDown` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.pL` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L498) | 3 | Mantenimiento / herramientas | Bajo; sin llamada interna detectada; síncrona | — | `add`, `onDown` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.pR` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L499) | 3 | Mantenimiento / herramientas | Bajo; sin llamada interna detectada; síncrona | — | `add`, `onDown` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.onDown` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L503) | 3 | Mantenimiento / herramientas | Bajo; interno; síncrona | `initResize`, `mainW`, `pL`, `pR` | `add` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.onMove` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L518) | 3 | Mantenimiento / herramientas | Bajo; interno; síncrona | `initResize` | `mainW`, `rsz` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.onUp` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L539) | 3 | Mantenimiento / herramientas | Bajo; interno; síncrona | `initResize` | — | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.initEv` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L562) | 11 | Mantenimiento / herramientas | Bajo; interno; síncrona | `init` | `draw` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.rsz` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L589) | 2 | Mantenimiento / herramientas | Bajo; interno; síncrona | `init`, `initResize`, `onMove` | `draw` | — |
| `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.init` | [`SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html`](../../SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html#L597) | 4 | Mantenimiento / herramientas | Bajo; sin llamada interna detectada; síncrona | — | `addPt`, `draw`, `genRoute`, `initEv`, `initResize`, `loop`, `rPts`, `rsz`, `upSt` | — |
| `connect_db` | [`consultar_db.py`](../../consultar_db.py#L21) | 2 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | `connect` | — |
| `show_summary` | [`consultar_db.py`](../../consultar_db.py#L32) | 6 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | — | estado |
| `list_sessions` | [`consultar_db.py`](../../consultar_db.py#L54) | 3 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | — | — |
| `list_commands` | [`consultar_db.py`](../../consultar_db.py#L64) | 4 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | — | estado |
| `list_events` | [`consultar_db.py`](../../consultar_db.py#L75) | 2 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | — | — |
| `list_telemetry` | [`consultar_db.py`](../../consultar_db.py#L83) | 3 | Mantenimiento / herramientas | Alto; interno; síncrona | `main` | — | estado, telemetría |
| `search_all` | [`consultar_db.py`](../../consultar_db.py#L96) | 12 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | — | — |
| `main` | [`consultar_db.py`](../../consultar_db.py#L118) | 11 | Mantenimiento / herramientas | Bajo; entrada/framework; síncrona | — | `close`, `connect_db`, `list_commands`, `list_events`, `list_sessions`, `list_telemetry`, `parse_args`, `search_all`, `show_summary` | telemetría |
| `active_files` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L36) | 6 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | — | — |
| `subsystem` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L64) | 8 | Mantenimiento / herramientas | Bajo; interno; síncrona | `normalize_tags` | — | — |
| `language` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L83) | 1 | Mantenimiento / herramientas | Bajo; interno; síncrona | `normalize_tags` | — | — |
| `run_ctags` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L91) | 7 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | `run` | — |
| `embedded_html_functions` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L109) | 22 | Mantenimiento / herramientas | Bajo; interno; asíncrona | `main` | `find_end` | — |
| `embedded_html_functions.find_end` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L119) | 5 | Mantenimiento / herramientas | Bajo; interno; síncrona | `embedded_html_functions` | — | — |
| `scripted_functions` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L156) | 8 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | — | — |
| `visibility` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L187) | 2 | Mantenimiento / herramientas | Bajo; interno; síncrona | `normalize_tags` | — | — |
| `normalize_tags` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L194) | 8 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | `language`, `subsystem`, `visibility` | — |
| `_function_body` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L244) | 1 | Mantenimiento / herramientas | Bajo; interno; síncrona | `infer_calls` | — | — |
| `_owner` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L250) | 5 | Mantenimiento / herramientas | Bajo; interno; síncrona | `infer_calls` | — | — |
| `_platform` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L262) | 5 | Mantenimiento / herramientas | Bajo; interno; síncrona | `infer_calls` | — | — |
| `_complexity` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L274) | 12 | Mantenimiento / herramientas | Bajo; interno; síncrona | `infer_calls` | — | — |
| `_interaction` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L280) | 7 | Mantenimiento / herramientas | Medio; interno; cola/evento | `infer_calls` | — | cola de comandos, cola de eventos |
| `_shared_state` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L290) | 9 | Mantenimiento / herramientas | Medio; interno; síncrona | `infer_calls` | — | estado, misión activa, comando activo, telemetría, sesión, parada/cierre |
| `infer_calls` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L304) | 18 | Mantenimiento / herramientas | Alto; interno; asíncrona | `main` | `_complexity`, `_function_body`, `_interaction`, `_owner`, `_platform`, `_shared_state` | sesión, parada/cierre |
| `dependency_cycles` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L340) | 12 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | `add`, `visit` | — |
| `dependency_cycles.visit` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L350) | 9 | Mantenimiento / herramientas | Bajo; interno; síncrona | `dependency_cycles` | `add` | — |
| `extract_routes` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L381) | 9 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | — | — |
| `node_id` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L402) | 1 | Mantenimiento / herramientas | Bajo; interno; síncrona | `mermaid_for_folder`, `plantuml_for_folder` | — | — |
| `mermaid_for_folder` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L407) | 15 | Mantenimiento / herramientas | Bajo; interno; síncrona | `write_markdown` | `add`, `node_id` | — |
| `plantuml_for_folder` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L444) | 8 | Mantenimiento / herramientas | Bajo; interno; síncrona | `write_markdown` | `node_id` | — |
| `write_markdown` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L471) | 10 | Mantenimiento / herramientas | Alto; interno; síncrona | `main` | `mermaid_for_folder`, `plantuml_for_folder` | estado |
| `main` | [`scripts/documentacion/generar_catalogo.py`](../../scripts/documentacion/generar_catalogo.py#L522) | 10 | Mantenimiento / herramientas | Bajo; entrada/framework; síncrona | — | `active_files`, `dependency_cycles`, `embedded_html_functions`, `extract_routes`, `infer_calls`, `normalize_tags`, `run_ctags`, `scripted_functions`, `write_markdown` | — |
| `rows` | [`scripts/documentacion/generar_resumen_sqlite.py`](../../scripts/documentacion/generar_resumen_sqlite.py#L18) | 1 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | — | — |
| `elapsed_seconds` | [`scripts/documentacion/generar_resumen_sqlite.py`](../../scripts/documentacion/generar_resumen_sqlite.py#L22) | 1 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | — | — |
| `main` | [`scripts/documentacion/generar_resumen_sqlite.py`](../../scripts/documentacion/generar_resumen_sqlite.py#L27) | 11 | Mantenimiento / herramientas | Medio; entrada/framework; síncrona | — | `close`, `connect`, `elapsed_seconds`, `parse_args`, `rows` | telemetría, sesión |
| `download_if_needed` | [`scripts/documentacion/renderizar_uml.py`](../../scripts/documentacion/renderizar_uml.py#L25) | 3 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | — | — |
| `render` | [`scripts/documentacion/renderizar_uml.py`](../../scripts/documentacion/renderizar_uml.py#L42) | 3 | Mantenimiento / herramientas | Bajo; interno; síncrona | `_ui`, `init`, `main`, `update` | `run` | — |
| `main` | [`scripts/documentacion/renderizar_uml.py`](../../scripts/documentacion/renderizar_uml.py#L54) | 3 | Mantenimiento / herramientas | Bajo; entrada/framework; síncrona | — | `download_if_needed`, `render` | — |
| `markdown_files` | [`scripts/documentacion/validar_enlaces.py`](../../scripts/documentacion/validar_enlaces.py#L15) | 4 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | — | — |
| `normalize_target` | [`scripts/documentacion/validar_enlaces.py`](../../scripts/documentacion/validar_enlaces.py#L26) | 3 | Mantenimiento / herramientas | Bajo; interno; síncrona | `main` | — | — |
| `main` | [`scripts/documentacion/validar_enlaces.py`](../../scripts/documentacion/validar_enlaces.py#L36) | 10 | Mantenimiento / herramientas | Bajo; entrada/framework; síncrona | — | `markdown_files`, `normalize_target` | — |
