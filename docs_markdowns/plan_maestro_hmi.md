# Plan Maestro de Mejora del HMI

**Proyecto:** carroTest3-For-Esp32-s3-wroom · Robot móvil 4WD diferencial ESP32-S3
**Alcance:** HMI modular (`desktop_app/robot_app/hmi/`) servido por Flask/Waitress y empaquetado en Android (Chaquopy + WebView).
**Fecha:** 2026-10-27 · **Estado:** Propuesta (sin implementar)

---

## 0. Resumen ejecutivo

| # | Fase | Objetivo | Prioridad |
|---|------|----------|-----------|
| A | Corrección de bugs del gestor de ventanas | Que cerrar, minimizar y la vista única funcionen; que ninguna ventana tape el E-STOP | **Crítica** |
| B | Modo Ventana Única y menú "···" | Recuperar la vista de una ventana a la vez; controles de ventana desde el menú | Alta |
| 0 | Sistema de diseño (tokens, paleta, tipografía) | Rehacer el tema claro y eliminar ~400 colores fijos | Alta |
| 1 | Perfil Operador / Técnico + modal de 3 pestañas | Simplificar la interfaz para usuarios no técnicos | Alta |
| 2 | Guía del operador y Acerca de | Ayuda integrada, créditos, logos y licencia | Media |
| 3 | Programador de rutas con flujo ①–⑤ | Orden lógico: nombrar o no, modo, pasos, ejecutar | Media |
| 4 | Gráficas responsivas | Que Chart.js se vea bien en todos los modos | Media |
| 5 | "Enfocar" en mosaico | Ver una ventana completa dentro del mosaico | Media |
| 6 | Android: enlaces externos | Abrir GitHub en el navegador del sistema | Baja |
| — | Idioma inglés | Pospuesto (ver §11) | Futuro |

La Fase A va primero porque incluye un posible riesgo de seguridad (ventanas por encima del botón E-STOP). La Fase 0 va antes que las de interfaz para que todo lo nuevo se construya sobre los tokens de diseño.

---

## 1. Fase A — Bugs del gestor de ventanas

Diagnóstico hecho sobre el código actual de `window_manager.js`, `window_interact.js`, `window_layout.js`, `tab_manager.js` y `layout.css`.

### A1. Los botones Minimizar / Maximizar / Cerrar no responden en modo Flotante — **confirmado en código**
- **Causa:** `WindowInteract.startDrag()` (`js/window_interact.js`, inicio de la función) llama a `win.headerEl.setPointerCapture(e.pointerId)` en **cualquier** `pointerdown` sobre la cabecera, incluido el que cae sobre los botones `.win-btn`. Con la captura activa, el `click` se redirige a la cabecera y los botones nunca lo reciben.
- **Afecta a:** todas las ventanas en modo Flotante, en especial los forks ("Fork · Programador de Rutas") y los módulos del menú "···", que **fuerzan** el modo Flotante (`createForkWindow` y `createCustomWindow` llaman a `setMode('floating')`).
- **Corrección:** al inicio de `startDrag`, salir si `e.target.closest('.window-controls')`.
- **Prueba:** test de Vitest que simula `pointerdown` + `click` sobre `.win-btn-close` en modo flotante y verifica que la ventana se elimina.

### A2. El modo de vista única muestra todas las ventanas abiertas — **confirmado en código**
Tres causas combinadas:
1. **CSS:** `#hmi-workspace.workspace-floating .tab-content { display: flex !important; }` (`css/layout.css`, bloque "Modo Flotante") anula cualquier `style.display = 'none'`. La única forma de ocultar una ventana es la clase `.is-minimized`.
2. **`TabManager.switch()`** solo oculta las demás ventanas en modo `tabs`. En `floating` muestra la nueva sin minimizar las anteriores, así que se acumulan.
3. **`TabManager._syncWindowManagerTab()`** llama a `WindowManager._applyWindowRect(win)`, **que no existe** (la función real es `WindowLayout.applyWindowRect`). Por eso la ventana no recibe posición ni tamaño y se apila arriba a la izquierda, con aspecto de "pestañas abiertas".
- **Corrección:** ver Fase B; el modo Ventana Única se implementa como un modo explícito.

### A3. Las ventanas pueden quedar por encima de la cabecera y del E-STOP — **probable, verificar en navegador**
- **Causa:** `bringToFront()` incrementa `maxZIndex` sin límite en **cada** `pointerdown` dentro de una ventana. `#hmi-workspace.workspace-floating` tiene `position: relative` pero no crea contexto de apilamiento, y la cabecera sticky usa `z-index: 100`. Tras unos 90 toques, una ventana puede pintarse encima de la cabecera al hacer scroll.
- **Riesgo:** el **E-STOP** podría quedar tapado. Es un problema de seguridad operativa.
- **Corrección:**
  - `#hmi-workspace { isolation: isolate; }` para encerrar los z-index de las ventanas.
  - Normalizar los z-index (reasignar 11…10+n según el orden) cuando `maxZIndex` supere un umbral.
  - Prueba de contrato: el E-STOP queda siempre por encima de cualquier ventana.

### A4. El menú "···" queda detrás de una ventana maximizada — **probable, verificar**
- `#glass-fork-menu` (`z-index: 1000`) está **dentro** de la cabecera, cuyo contexto de apilamiento es `z-index: 100`. Una ventana maximizada (`position: fixed; z-index: 999`) lo cubre, y el menú parece no abrirse.
- **Corrección:** sacar el menú de la cabecera (montarlo en `body`) o usar `<dialog>` nativo, que se muestra en la *top layer*.

### A5. Al tocar la pestaña activa, su contenido desaparece — **confirmado en código**
- `TabManager._handleWindowTaskbarClick()`: en modo `tabs`, tocar la pestaña ya activa llama a `minimizeWindow()` y el área de trabajo queda vacía. Para el operador parece un fallo.
- **Corrección:** en modo `tabs`, tocar la pestaña activa no hace nada. Minimizar solo desde el control de la ventana o desde el menú "···".

### A6. `setMode()` mezcla `mode` y `effectiveMode` — **confirmado en código**
- En pantallas de menos de 768px, `effectiveMode = 'tabs'`, pero la clase CSS del workspace y el `if/else` que aplica el diseño usan `mode`. Resultado: estado `tabs` con clase `workspace-floating`.
- **Corrección:** usar `effectiveMode` en todo el cuerpo de la función.

### A7. Girar la tablet o reducir la ventana borra la preferencia de modo — **confirmado en código**
- El listener `resize` llama a `setMode('tabs')`, que **persiste** el modo en `localStorage` y en el backend (`SettingsManager.syncBackend()`). Al volver a pantalla ancha, la preferencia del usuario ya se perdió.
- **Corrección:** distinguir el modo preferido (persistido) del modo efectivo (calculado). El `resize` cambia solo el efectivo.

### A8. Texto repetido en el menú "···" — **confirmado en código**
- `WindowLayout.renderForkMenu()` imprime `${item.icon} ${item.name}`, y en `fork_registry.js` los `icon` son **texto** (`'Boost'`, `'Fork'`, `'Telemetría'`). De ahí "**Boost** HUD Cinético & **Boost**" y "**Telemetría** Consola de **Telemetría** Extendida".
- El botón del encabezado `#ws-btn-add-fork` muestra el icono de tres puntos **y además** `<span class="ws-label">···</span>`.
- **Corrección:**
  - `icon` pasa a ser un SVG (como en `_registerPanels`), o se elimina del texto.
  - Quitar el `<span class="ws-label">···</span>`; el `aria-label` ya describe el botón para lectores de pantalla.
  - Pasar los estilos en línea del menú a clases CSS (Fase 0).

### Criterios de aceptación de la Fase A
- [ ] En todos los modos, cerrar, minimizar y maximizar responden al primer toque.
- [ ] Ninguna ventana cubre la cabecera ni el E-STOP, ni tras 500 interacciones.
- [ ] El menú "···" se ve por encima de una ventana maximizada.
- [ ] Tocar la pestaña activa no vacía la pantalla.
- [ ] Girar la tablet no cambia la preferencia de disposición guardada.

---

## 2. Fase B — Modo Ventana Única y menú "···"

### B1. Modos de disposición propuestos

| Modo | Comportamiento |
|------|----------------|
| **Pestañas** | Como hoy, sin cabecera de ventana. Ideal para el perfil Operador en tablet. |
| **Mosaico** | Ventanas apiladas a lo ancho, con "Enfocar" (Fase 5). |
| **Ventana Única** *(recupera el comportamiento original de "Flotante")* | **Una sola ventana visible a la vez**, ocupando toda el área de trabajo y con cabecera (minimizar, maximizar, reducir y cerrar). Al pulsar otra pestaña, o abrir un módulo desde "···", la anterior se minimiza automáticamente. |
| **Flotante libre** *(opcional, solo perfil Técnico)* | Varias ventanas arrastrables a la vez. Se llega con "Reducir" desde Ventana Única o al crear un fork. |

**Reglas de Ventana Única:**
1. Invariante: como máximo **una** ventana sin `.is-minimized` (excepto si el usuario la "redujo" a flotante).
2. Una sola función `WindowManager.focusSingle(id)` concentra la lógica: minimiza todas las demás, restaura `id` y aplica su tamaño. `TabManager.switch()`, `HMI_FORK_REGISTRY.createInstance()`, `openWindow()` y `createForkWindow()` la llaman cuando el modo es `single`. Así se elimina la lógica duplicada que hoy causa A2.
3. Por defecto la ventana ocupa el 100% del área de trabajo, sin coordenadas absolutas, así que no le afecta el bug del `rect`.
4. Crear un fork o un módulo **ya no fuerza** el modo Flotante: se abre como la ventana única activa.

### B2. Controles de ventana: Minimizar, Maximizar, Reducir y Cerrar

| Control | Icono | Acción |
|---------|-------|--------|
| Minimizar | `—` | Oculta la ventana; queda en la barra de pestañas y en "···" → *Ventanas abiertas*. |
| Maximizar | `□` | Ocupa toda la pantalla bajo la cabecera, sin tapar el E-STOP. |
| Reducir (restaurar) | `❐` | Vuelve al tamaño anterior; en Ventana Única la pasa a flotante. |
| Cerrar | `✕` | Solo en forks y módulos; las 5 ventanas base no se cierran. |

- Iconos SVG en lugar de caracteres Unicode, con área táctil mínima de **44×44 px** (la de los botones actuales es pequeña para la tablet).
- Maximizar y Reducir comparten botón y alternan según el estado, como ya hace `maximizeWindow()`.
- Doble toque en la cabecera = maximizar/reducir.

### B3. Rediseño del menú "···"

```
┌ Ventanas y módulos ─────────────────────────── ✕ ┐
│ VENTANAS ABIERTAS                                │
│  ⚡ HUD Cinético & Boost    [—] [□] [✕]           │
│  ⑂ Fork · Programador      [—] [❐] [✕]   ● activa │
│                                                  │
│ ABRIR MÓDULO                                     │
│  ⚡ HUD Cinético & Boost (DRV8833)      [Abrir]   │
│     Ráfaga térmica y reparto de par              │
│  ⑂ Bifurcación de Ruta (A/B)           [Abrir]   │
│  ▤ Consola de Telemetría Extendida     [Abrir]   │
└──────────────────────────────────────────────────┘
```

- La sección **Ventanas abiertas** lista las ventanas minimizadas o abiertas con sus controles. Así se pueden **cerrar o minimizar desde el menú**, que es lo que hoy no funciona.
- **Abrir módulo** conserva el catálogo actual, sin texto duplicado.
- Si un módulo es singleton y ya está abierto, "Abrir" cambia a "Ir a".
- Implementación como `<dialog>` (corrige A4, trae foco atrapado y cierre con Esc).
- En perfil Operador se ocultan los módulos de diagnóstico (Consola de Telemetría, Boost HUD).

### Criterios de aceptación de la Fase B
- [ ] En Ventana Única, al pulsar cualquier pestaña o módulo solo queda visible esa ventana.
- [ ] Desde "···" se puede minimizar, maximizar, reducir y cerrar cualquier ventana abierta.
- [ ] Sin texto repetido en el menú ni en su botón.
- [ ] Tests de Vitest para el invariante "una sola visible" y para cada control.

---

## 3. Fase 0 — Sistema de diseño y temas

### Diagnóstico
- `theme.css` define unas 20 variables por tema, pero hay **~400 colores fijos** fuera de él: `layout.css` 132, `components.css` 98, `responsive.css` 73, `touch.css` 15. Esos colores no cambian con el tema.
- **98 estilos en línea** en las tarjetas (`tab_conexion.html` 40, `tab_rutas.html` 36, `hud_boost.html` 15), además de los generados en JS (`fork_registry.js`, `window_layout.js`).
- Chart.js usa colores fijos en JS.
- El tema "Light Glass" usa superficies translúcidas al 85% sobre fondo claro, con pérdida de contraste y tarjetas oscuras que no cambiaron.
- Etiquetas de 8–11 px, ilegibles en la tablet.

### Propuesta
1. **Tokens en tres capas** (en `theme.css`):
   - *Paleta base*: escala de neutros de 10 pasos con matiz frío, escala del verde de marca y escalas de estado.
   - *Semánticos*: `--surface-1/2/3`, `--text-strong/normal/muted`, `--border`, `--accent`, `--ok`, `--warn`, `--danger`, `--info`, `--fsm-active`, `--fsm-done`, `--fsm-idle`, `--chart-grid`, `--chart-series-1…6`.
   - *Componentes*: solo consumen tokens semánticos.
2. **Color:**
   - Acento **verde** (confirmado), coherente con el engrane del logo del ITESS.
   - El verde de acento se usa en acciones; el estado "ok" usa el mismo tono **siempre con un icono ✓**.
   - Ámbar = atención, rojo = fallo o E-STOP, azul = información. Significan lo mismo en todos los temas.
   - Contraste **WCAG AA**: 4.5:1 en texto y 3:1 en bordes y controles.
3. **Tipografía:**
   - Una sans para la interfaz y una mono **solo para datos**, con `font-variant-numeric: tabular-nums`.
   - Escala 12 / 14 / 16 / 20 / 24, con un **mínimo de 14 px** en la tablet.
   - Fuentes **locales** en `hmi/fonts/` (sin CDN, por la CSP y el uso sin conexión).
4. **Temas:**
   - **Oscuro** (por defecto).
   - **Claro rehecho**: superficies sólidas, profundidad con sombras y texto casi negro.
   - **Alto contraste**: útil a plena luz del sol.
   - Ocultar "Light Glass" hasta que el claro esté listo.
5. **Limpieza:** colores fijos → tokens; `style=""` → clases (también refuerza la CSP).
6. **Gráficas:** leer colores con `getComputedStyle(document.documentElement)` y repintar al cambiar de tema.

### Criterios de aceptación de la Fase 0
- [ ] Test de Vitest: ningún color hex o `rgb()` fuera de `theme.css`.
- [ ] Test de contraste: cada par texto/superficie de cada tema cumple AA.
- [ ] Cero `style=""` en `cards/*.html`.
- [ ] Capturas de los 3 temas × 4 modos de disposición, revisadas.

---

## 4. Fase 1 — Perfil Operador / Técnico y modal de configuración

### Perfil de interfaz
- Nuevo ajuste `profile: 'operator' | 'technician'` en `SettingsManager`. Se aplica como `body[data-profile]`.
- Regla única: `body[data-profile="operator"] [data-debug-only] { display: none !important; }`.
- Pasar a Técnico pide una confirmación simple.

| Perfil Operador oculta | Se mantiene siempre |
|---|---|
| Pestaña Testing S3, pines, PWM | Conexión |
| Logs del pie y telemetría cruda | **E-STOP** y estado del robot |
| Bifurcaciones A/B, métricas de segmentos, modo vectorial experimental | Rutas (flujo ①–⑤), mapa |
| "Borrar misión Python", opciones de SQLite | "Regresar al origen" |
| Módulos de diagnóstico del menú "···" | Barra de estados (nodos) |

En perfil Operador, la disposición por defecto es **Pestañas**.

### Barra de estados (8 nodos): se conserva
- **Operador:** `Listo → Girando → Esperando → Avanzando → Terminado`, con iconos. **FALLO** y **E-STOP** pasan a ser insignias aparte que se encienden en rojo. Debajo del nodo activo: "Paso 2/5" y una barra de progreso del tramo.
- **Técnico:** se queda como está (INIT…ESTOP) con el texto del checkpoint, para que coincida con los logs y SQLite.

### Modal de configuración con 3 pestañas
`[Ajustes] [Guía del operador] [Acerca de]`
- **Ajustes:** Perfil; Disposición (Pestañas / Mosaico / Ventana Única / Flotante); Tema; Registro SQLite (solo Técnico).
  - **Se elimina** "Modo de Espacio de Trabajo Inicial", que repite "Disposición": la disposición elegida se guarda como la inicial.
  - El selector de idioma se oculta o se marca "próximamente".

---

## 5. Fase 2 — Guía del operador y Acerca de

### Guía del operador (`cards/ayuda_operador.html`)
Secciones en `<details>`, sin JavaScript:
1. Antes de empezar: batería, superficie, espacio libre, ubicación del E-STOP.
2. Conectar el robot.
3. Calibrar: 5 s de reposo, giro de +25° y retorno.
4. Crear y ejecutar una ruta (flujo ①–⑤).
5. Qué significa cada estado:

   | Nodo | Significado |
   |---|---|
   | Listo | Conectado y esperando órdenes |
   | Girando | Orientándose hacia el siguiente punto |
   | Esperando | Pausa breve para estabilizarse |
   | Avanzando | Recorriendo el tramo |
   | Terminado | Llegó al último punto |
   | ⚠ Fallo | No pudo completar el paso; revisar obstáculos |
   | ⛔ E-STOP | Paro de emergencia activo |

6. Regresar al origen.
7. Qué hacer si…: no avanza, gira de más, se pierde la conexión, se activa el E-STOP.
8. Glosario (solo Técnico): FSM, PCNT, ICR, PWM, yaw, Ockham.

Además, botones **?** contextuales que abren la sección correspondiente (`#guia-estados`, `#guia-calibracion`, `#guia-rutas`).

### Acerca de (`cards/acerca_de.html`)

```
Robot móvil 4WD diferencial · ESP32-S3
HMI vX.Y · Backend vX.Y · Firmware vX.Y (o "sin conexión")

Desarrollador del prototipo 4WD y la aplicación:  Rubén Andrés Gómez Molina
Asesor interno:   Alejandro Tapia Muñoz
Asesora externa:  Dra. Rosa María López Salinas

[ franja blanca:  logo TecNM · logo ITESS · logo CENIDET ]

GitHub: https://github.com/RubenAndresGome/carroTest3-For-Esp32-s3-wroom   [QR]
Licencia: Apache License 2.0
Créditos: Chart.js, Flask, Waitress, Chaquopy
```

**Logos** (origen: `C:\Users\IK\Downloads\LOGOS_ADCADEMIA`):

| Archivo | Observación | Destino |
|---|---|---|
| `LOGO-VERTICAL-TECNM.png` (188 KB) | Buena calidad, azul marino | `hmi/img/tecnm.webp` (~400 px de alto) |
| `logo_itess.png` (1.39 MB) | Fondo gris incluido; se usa tal cual por ahora | `hmi/img/itess.webp` (~600 px de ancho, ≈40 KB) |
| `CENIDET_logo (1).svg` (355 KB) | No es vectorial: es un PNG embebido en un SVG | Extraer el PNG → `hmi/img/cenidet.webp` |

- **Franja blanca** detrás de los logos: los manuales de identidad no permiten recolorearlos ni invertirlos, y el azul marino no se ve sobre el tema oscuro.
- Total de imágenes por debajo de ~150 KB, para no inflar la APK.
- Nueva ruta `/img/<archivo>` en `web.py`, con lista blanca de extensiones (`.webp`, `.png`, `.svg`).
- `alt` descriptivo en cada logo.
- **Nota legal visible:** "Los logotipos de TecNM, ITESS y CENIDET son propiedad de sus respectivas instituciones y no están cubiertos por la licencia Apache 2.0."
- El repositorio ya tiene `LICENSE` (Apache 2.0). Opcional: agregar un `NOTICE` con autoría y créditos institucionales.
- **QR** de GitHub como SVG estático, generado una vez y versionado (sin librería en tiempo de ejecución).

---

## 6. Fase 3 — Programador de rutas con flujo ①–⑤

```
① ¿Guardar esta ruta?   ( ) Ruta rápida, sin guardar   (•) Ruta con nombre
② Nombre: [__________]          ← solo si eligió "con nombre"
③ Modo:  [Rectangular] [Ángulo y distancia] [Vectorial*]
④ Pasos: X [__] ±   Y [__] ±   [+ Agregar] [Deshacer]
   └ tabla de pasos debajo, con borrar por fila
⑤ [Previsualizar]  [▶ Ejecutar]  [Guardar]            [■ Detener]
```

- **Separar Guardar de Ejecutar.** Hoy `#btn-route-start` hace las dos cosas. Con "Ruta rápida" el botón Guardar se oculta.
- Menú **"Avanzado ▾"** (`data-debug-only`) con bifurcaciones A/B, "Borrar misión Python", métricas de muestras y segmentos, y modo vectorial experimental.
- El modo se elige con botones segmentados en lugar del `<select>`.
- **Detener** siempre visible, separado de las demás acciones.
- Terminología para el operador: "Ockham" → **"Regresar al origen"**; "Angular por descomposición" → **"Ángulo y distancia"**.

---

## 7. Fase 4 — Gráficas responsivas

- **Container queries:** `container-type: inline-size` en cada ventana. Así `.route-maps-grid` decide entre 1 y 2 columnas según el **ancho de la ventana** y no el del viewport (hoy `@media (max-width:1024px)` en `responsive.css`).
- **`ResizeObserver`** en cada `.route-map-container`, en lugar de los `setTimeout(resizeCharts, 50)` repartidos por `window_manager.js`, `tab_manager.js` y `route_charts.js`.
- En ventanas estrechas, selector **[Plan] [Real] [Ambas]** en lugar de dos gráficas diminutas.
- Altura mínima de ~280 px por gráfica.
- Colores tomados de los tokens `--chart-*` (Fase 0).

---

## 8. Fase 5 — "Enfocar" en modo mosaico

- Hoy cada ventana del mosaico mide **520 px fijos** (`window_layout.js`, `applyTiling`) y el maximizado usa `position: fixed`, que tapa la barra de pestañas.
- **Enfocar** (doble toque en la cabecera o botón ⤢): la ventana ocupa el 100% de la altura del área de trabajo, sin `fixed`, y las demás quedan como franjas de una línea.
- Altura según el contenido: `min(contenido, alto del área de trabajo)`, respetando la altura que el usuario haya ajustado.

---

## 9. Fase 6 — Android: enlaces externos

- En el `WebViewClient` de `android_app`, implementar `shouldOverrideUrlLoading`: las URLs fuera de `localhost` se abren con un `Intent.ACTION_VIEW` en el navegador del sistema.
- Sin esto, el operador queda atrapado en GitHub dentro del HMI, sin forma de volver.

---

## 10. Pruebas y verificación por fase

| Fase | Verificación |
|---|---|
| Todas | `npm --prefix desktop_app/frontend test` · `compilar_hmi.py --check` · pruebas de backend Python |
| A, B | Nuevos tests de Vitest: controles de ventana, invariante de Ventana Única, z-index del E-STOP, menú "···" |
| 0 | Test sin colores fijos fuera de `theme.css`; test de contraste AA |
| 1 | Test de que los `[data-debug-only]` se ocultan en perfil Operador |
| 2 | Tests ARIA del modal con pestañas; ruta `/img/` con lista blanca (test de backend) |
| 4 | Revisión manual: 3 temas × 4 modos × tablet y escritorio |
| 6 | Prueba en tablet: el enlace abre el navegador del sistema |

Cada fase va en su propio commit (Conventional Commits, en español).

---

## 11. Futuro — Idioma inglés (pospuesto)

- **No requiere base de datos:** bastan archivos estáticos `hmi/i18n/es.json` y `hmi/i18n/en.json`, con claves usadas mediante `data-i18n="clave"` en las tarjetas.
- Punto de partida: `SettingsManager.applyTranslations()` (`js/settings_manager.js`).
- Requisito previo: terminar la Fase 0 y la Fase 3, para no traducir textos que van a cambiar.

---

## 12. Datos confirmados

- **Créditos:** Rubén Andrés Gómez Molina (desarrollador del prototipo 4WD y la aplicación); Alejandro Tapia Muñoz (asesor interno, sin grado); Dra. Rosa María López Salinas (asesora externa).
- **Instituciones:** TecNM · ITESS · CENIDET.
- **Repositorio:** https://github.com/RubenAndresGome/carroTest3-For-Esp32-s3-wroom
- **Licencia:** Apache License 2.0 (ya presente en `LICENSE`).
- **Acento de color:** verde.
- **Pendientes menores (no bloquean):** versión mejorada del logo del ITESS; confirmar el grado del asesor interno si se desea mostrar.
