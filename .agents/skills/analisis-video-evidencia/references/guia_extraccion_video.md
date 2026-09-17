# Guía de Extracción e Inspección de Video Forense

Esta guía complementa a `SKILL.md` detallando las herramientas, formatos y procedimientos técnicos para procesar grabaciones de video de ensayos físicos del robot ESP32-S3.

---

## 1. Convenciones del Repositorio para Archivos de Video

El repositorio cuenta con una estructura canónica para archivar y procesar evidencia audiovisual:

- **Videos crudos / normalizados:** `evidencia/videos/` (gestionados vía Git LFS o locales).
- **Fotogramas y hojas de contacto:** `evidencia/fotogramas/<slug_video>/`
- **Informes de incidentes correlacionados:** `evidencia/incidentes/`

### Formatos recomendados y normalización
- **Contenedores:** `.mp4`, `.webm`, `.mkv`.
- **Códecs óptimos:** H.264 (AVC) o H.265 (HEVC) para video; AAC para audio (si incluye sonido de motores).
- **Tasa de fotogramas:** 15 a 30 fps es suficiente para capturar aceleraciones, patinaje y paradas del chasis.

---

## 2. Protocolo Obligatorio: Solicitar Ubicación y Nombre del Video

El agente **DEBE SIEMPRE** requerir explícitamente:
1. **Ubicación**: Ruta de la carpeta donde reside el video (ej. `evidencia/videos/` o una ruta absoluta en disco como `C:\Pruebas\Robot\`).
2. **Nombre del archivo**: Nombre exacto con extensión (ej. `ejes_invertidos_2026-09-08.mp4`).

### Ejemplo de interacción estándar:
- **Usuario:** *"El robot sigue fallando al intentar girar a 90 grados, revisa el video."*
- **Agente:**
  > *"Con gusto reviso la evidencia visual. Para acceder al archivo, por favor indícame:*
  > 1. *La **ubicación** o carpeta donde está guardado (ej. `evidencia/videos/`).*
  > 2. *El **nombre exacto del archivo de video** (ej. `ensayo_giro_90.mp4`)."*

### Comprobación de ruta en Windows PowerShell:
```powershell
Test-Path "evidencia\videos\<nombre_archivo>.mp4"
```
Si el archivo no se encuentra, sugerir listar los existentes en `evidencia/videos/` sin asumir uno a ciegas.

---

## 3. Inspección Multimodal con `view_file`

La herramienta nativa `view_file` admite archivos binarios de video directamente:

```json
{
  "AbsolutePath": "c:\\Users\\IK\\Documents\\Codex\\carroTest3-For-Esp32-s3-wroom\\evidencia\\videos\\mi_video.mp4",
  "toolAction": "Viewing video file",
  "toolSummary": "Multimodal inspection of physical robot trial"
}
```

Al utilizar `view_file` en un video, el modelo analiza directamente la secuencia visual. Enfoca la atención en:
- **00:00 - Reposo inicial:** Observar la orientación del frente del chasis (+Y), si los cables cuelgan o ejercen tensión.
- **Transición de arranque:** Identificar si las 4 ruedas arrancan en el mismo instante o si una rueda sufre retardo por holgura de engranaje (*backlash*) o zona muerta de PWM.
- **Régimen dinámico:** Observar la velocidad angular de giro y si existe deslizamiento relativo respecto a la textura del suelo.
- **Transición terminal:** Observar si la detención es por rampa controlada (suave) o corte brusco (watchdog / E-STOP).

---

## 4. Inspección Forense Avanzada con `scripts/inspeccionar_video.py`

Cuando se requiera extraer fotogramas exactos o generar una hoja de contactos (contact sheet) para anexar a un reporte técnico:

### Sintaxis básica:
```powershell
& 'desktop_app\.test-venv\Scripts\python.exe' .agents\skills\analisis-video-evidencia\scripts\inspeccionar_video.py --video "evidencia\videos\<video>.mp4" --info
```

### Generación de hoja de contacto (24 fotogramas uniformes):
```powershell
& 'desktop_app\.test-venv\Scripts\python.exe' .agents\skills\analisis-video-evidencia\scripts\inspeccionar_video.py --video "evidencia\videos\<video>.mp4" --contact-sheet --frames 24
```
Salida generada:
- Hoja de contacto: `evidencia/fotogramas/contacto_<slug>.jpg`
- Muestras individuales: `evidencia/fotogramas/<slug>/fotograma_01.jpg` a `fotograma_24.jpg`

### Extracción de fotograma en un segundo exacto:
```powershell
& 'desktop_app\.test-venv\Scripts\python.exe' .agents\skills\analisis-video-evidencia\scripts\inspeccionar_video.py --video "evidencia\videos\<video>.mp4" --extract-at 00:04.500 --output "evidencia/fotogramas/anomalia_04s.jpg"
```

---

## 5. Correlación de Marcas de Tiempo

1. En el video, anota el segundo en que inicia el movimiento físico ($t_{inicio}$) y el segundo en que ocurre la anomalía ($t_{evento}$).
2. En la base de datos SQLite (`consultar_db.py --telemetry`), busca la muestra con el comando activo en esa misma ventana de tiempo relativo ($\Delta t = t_{evento} - t_{inicio}$).
3. Cruza la variación de ticks de encoder y el `yaw_deg` de la IMU para determinar si el fallo fue físico (mecánico/cable) o lógico (software/firmware).
