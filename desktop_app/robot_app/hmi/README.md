# HMI activo

Esta es la interfaz compleja servida por Flask desde la PC. Fue promovida desde
el mockup `IUInWeb/IndexBeta.html` para eliminar la dependencia operativa con
`archive/legacy/`.

- `index.html` contiene la interfaz actual.
- `vendor/` conserva bibliotecas JavaScript locales para operación offline.
- El navegador solo usa HTTP/SSE con Flask; Python mantiene el WebSocket único
  hacia el ESP32.
