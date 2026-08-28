# HMI activo

Esta es la interfaz compleja servida por Flask desde la PC. Fue promovida desde
el mockup `IUInWeb/IndexBeta.html` para eliminar la dependencia operativa con
`archive/legacy/`.

- `index.html` contiene la interfaz actual.
- `vendor/` conserva bibliotecas JavaScript locales para operación offline.
- El navegador solo usa HTTP/SSE con Flask; Python mantiene el WebSocket único
  hacia el ESP32.

## Contrato responsive

### Sistema visual técnico glass

El HMI conserva una sola fuente HTML/CSS offline para escritorio y Android. La
estética glass se implementa con tokens CSS (`--glass-surface`,
`--glass-border`, `--shadow-elevated`) y SVG locales; no requiere Tailwind, CDN
ni una segunda variante del DOM. El desenfoque es moderado para no sacrificar
contraste ni rendimiento en WebView.

El FSM se presenta como un stepper desplazable con estado activo y
`aria-current`. El visor de logs separa filtros y acciones, muestra cada nivel
como badge, conserva una tipografía monoespaciada y marca la limpieza de base de
datos como acción destructiva con confirmación obligatoria.

La validación estática vive en
`desktop_app/frontend/tests/hmi-responsive-contract.test.ts`. Comprueba el
viewport accesible, áreas seguras, tokens mínimos de tacto y tipografía,
navegación completa, breakpoints y patrones que podrían ocultar overflow.

La matriz mínima para validación visual y en dispositivo es:

| Familia | Viewports CSS | Orientación |
| --- | --- | --- |
| Teléfono compacto | 320×568, 360×640 | vertical |
| Teléfono actual | 390×844, 412×915, 480×800 | vertical |
| Teléfono apaisado | 640×360, 844×390 | horizontal |
| Tablet | 600×960, 768×1024, 800×1280 | ambas |
| Tablet grande/escritorio | 1024×768, 1280×720, 1440×900 | horizontal |

En todas las combinaciones debe conservarse la navegación completa, no debe
haber overflow horizontal del documento y los controles críticos deben seguir
visibles. Para ejecutar sólo este contrato:

```powershell
Push-Location desktop_app\frontend
npm run test:hmi-responsive
Pop-Location
```

La comprobación anterior protege el contrato en el código fuente. Para medir
el layout real, los bloqueos operativos y accesibilidad WCAG con Chromium:

```powershell
Push-Location desktop_app\frontend
npm run test:hmi-browser:install  # sólo la primera vez
npm run test:hmi-browser
Pop-Location
```

Playwright inicia Flask de forma aislada, recorre las cinco vistas (Touch se
mantiene informativa y bloqueada), ejecuta la matriz completa y exige cero
overflow global, targets de 44/48 px y cero infracciones axe serias o críticas.

La aceptación en Android requiere además una pasada física en al menos un
teléfono y una tablet: rotación en ambas orientaciones, recortes por notch o
barras del sistema, teclado virtual, gesto de desplazamiento y lectura básica
con TalkBack. Esta comprobación no se sustituye con la emulación de Chromium.
