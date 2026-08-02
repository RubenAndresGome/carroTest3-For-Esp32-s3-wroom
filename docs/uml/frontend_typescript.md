# UML funcional: `frontend/typescript`

Funciones detectadas: **3**. Tipos detectados: **4**.

## Grafo de llamadas

```mermaid
flowchart LR
  subgraph f0["api.ts"]
    n965d6db927["constructor()"]
    n71584cfab9["status()"]
  end
  subgraph f1["protocol.ts"]
    n53ecf45228["isRobotStatus()"]
  end
  classDef alto fill:#5b1f2a,stroke:#ff7a7a,color:#fff
  classDef medio fill:#4a3717,stroke:#ffca67,color:#fff
  classDef bajo fill:#123b3a,stroke:#39e6aa,color:#fff
  class n965d6db927 bajo
  class n71584cfab9 bajo
  class n53ecf45228 bajo
```

Fuentes: [Mermaid](mermaid/frontend_typescript.mmd) · [PlantUML](plantuml/frontend_typescript.puml). Las flechas continuas son llamadas síncronas; las discontinuas representan asincronía, eventos o colas. El color del nodo indica riesgo estático.

## Inventario

| Función | Archivo | CC | Propietario | Riesgo/estado | Entra desde | Sale hacia | Estado compartido |
|---|---|---:|---|---|---|---|---|
| `RobotApi.constructor` | [`desktop_app/frontend/src/api.ts`](../../desktop_app/frontend/src/api.ts#L4) | 1 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `RobotApi.status` | [`desktop_app/frontend/src/api.ts`](../../desktop_app/frontend/src/api.ts#L6) | 2 | HMI / operador | Bajo; entrada/framework; asíncrona | `connection`, `events`, `stream` | — | — |
| `isRobotStatus` | [`desktop_app/frontend/src/protocol.ts`](../../desktop_app/frontend/src/protocol.ts#L120) | 1 | HMI / operador | Bajo; sin llamada interna detectada; síncrona | — | — | — |
