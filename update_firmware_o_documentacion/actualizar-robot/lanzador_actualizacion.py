#!/usr/bin/env python3
"""Interfaz grafica para actualizar la HMI Android o el firmware ESP32-S3."""

from __future__ import annotations

import json
import os
import queue
import re
import shutil
import subprocess
import sys
import threading
import time
from pathlib import Path

import customtkinter as ctk
from PIL import Image, ImageDraw
from tkinter import messagebox

def _resolver_rutas() -> tuple[Path, Path]:
    """Resuelve rutas de trabajo tanto en código fuente como en ejecutable empaquetado."""
    anclas: list[Path] = [Path.cwd()]
    if getattr(sys, "frozen", False) and hasattr(sys, "_MEIPASS"):
        anclas.extend([Path(sys.executable).resolve().parent, Path(sys._MEIPASS)])
    else:
        anclas.append(Path(__file__).resolve().parent)

    vistos: set[Path] = set()
    for ancla in anclas:
        for raiz in (ancla.resolve(), *ancla.resolve().parents):
            if raiz in vistos:
                continue
            vistos.add(raiz)
            if (raiz / "platformio.ini").is_file() and (raiz / "android_app" / "gradlew.bat").is_file():
                lanzador = raiz / "update_firmware_o_documentacion" / "actualizar-robot"
                scripts = lanzador / "scripts"
                if not scripts.is_dir():
                    raise FileNotFoundError(f"No se encontró la carpeta de scripts: {scripts}")
                return lanzador, scripts
    raise FileNotFoundError(
        "No se encontró la raíz del repositorio Robot S3. Coloca el ejecutable dentro de "
        "update_firmware_o_documentacion\\actualizar-robot\\dist o ejecútalo desde el repositorio."
    )


def _comando_python(script: Path, argumentos: list[str]) -> list[str]:
    """El EXE no puede usarse como intérprete; localizar Python de forma explícita."""
    if not getattr(sys, "frozen", False):
        return [sys.executable, "-u", str(script), *argumentos]

    candidatos = [
        os.environ.get("ROBOT_UPDATER_PYTHON", ""),
        str(Path.home() / ".platformio" / "penv" / "Scripts" / "python.exe"),
        shutil.which("python") or "",
    ]
    for candidato in candidatos:
        if candidato and Path(candidato).is_file():
            return [candidato, "-u", str(script), *argumentos]
    if lanzador_py := shutil.which("py"):
        return [lanzador_py, "-3", "-u", str(script), *argumentos]
    raise RuntimeError(
        "No se encontró Python para ejecutar los scripts. Instala Python 3 o define ROBOT_UPDATER_PYTHON."
    )


def _detectar_puertos_windows() -> list[tuple[str, str]]:
    """Devuelve puertos COM y la descripción que Windows muestra al usuario."""
    powershell = Path(os.environ.get("SystemRoot", r"C:\Windows")) / "System32" / "WindowsPowerShell" / "v1.0" / "powershell.exe"
    if not powershell.is_file():
        raise RuntimeError("No se encontró PowerShell para consultar los puertos COM de Windows.")
    consulta = (
        "$ErrorActionPreference='Stop';"
        "Get-CimInstance Win32_PnPEntity | "
        "Where-Object { $_.Name -match '\\(COM\\d+\\)' } | "
        "ForEach-Object { [PSCustomObject]@{"
        "descripcion=$_.Name;estado=$_.Status} } | ConvertTo-Json -Compress"
    )
    resultado = subprocess.run(
        [str(powershell), "-NoProfile", "-Command", consulta],
        capture_output=True,
        text=True,
        errors="replace",
        timeout=10,
        check=False,
    )
    if resultado.returncode:
        raise RuntimeError(resultado.stderr.strip() or "Windows no pudo enumerar los puertos COM.")
    bruto = resultado.stdout.strip()
    if not bruto:
        return []
    datos = json.loads(bruto)
    if isinstance(datos, dict):
        datos = [datos]
    puertos: dict[str, str] = {}
    for dato in datos:
        descripcion = str(dato.get("descripcion", "")).strip()
        coincidencia = re.search(r"\((COM\d+)\)", descripcion, re.IGNORECASE)
        puerto = coincidencia.group(1).upper() if coincidencia else ""
        if re.fullmatch(r"COM\d+", puerto) and descripcion:
            puertos[puerto] = descripcion
    return sorted(puertos.items(), key=lambda item: int(item[0][3:]))


try:
    BASE_DIR, SCRIPTS_DIR = _resolver_rutas()
    ERROR_RUTAS = ""
except FileNotFoundError as error:
    BASE_DIR, SCRIPTS_DIR, ERROR_RUTAS = None, None, str(error)

COLORES_LOG = {
    "INFO": "#a0d0ff",
    "WARNING": "#f0d060",
    "ERROR": "#ff6060",
    "SUCCESS": "#60d060",
    "timestamp": "#6070a0",
}


class FirmwareUpdaterApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("blue")

        self.title("Actualizador de Firmware Profesional")
        self.geometry("820x690")
        self.minsize(720, 580)

        self.script_var = ctk.StringVar(value="actualizar_hmi_tablet.py")
        self.vmot_confirmado = ctk.BooleanVar(value=False)
        self.puerto_seleccionado = ctk.StringVar(value="Pulsa Actualizar lista")
        self.puertos: dict[str, str] = {}
        self.puertos_consultados = False

        # --- Fondo con gradiente ---
        self.bg_image = self._crear_gradiente((820, 690), "#0a0a1a", "#1a2a6c")
        self.bg_label = ctk.CTkLabel(self, image=self.bg_image, text="")
        self.bg_label.place(x=0, y=0, relwidth=1, relheight=1)

        self.main_frame = ctk.CTkFrame(self, fg_color="transparent")
        self.main_frame.pack(fill="both", expand=True, padx=30, pady=24)

        # --- Titulo ---
        titulo = ctk.CTkFrame(self.main_frame, fg_color="transparent")
        titulo.pack(fill="x", pady=(0, 10))
        ctk.CTkLabel(
            titulo,
            text="Robot S3 / Actualizador",
            font=ctk.CTkFont(size=26, weight="bold"),
            text_color="#ffffff",
        ).pack(side="left")
        ctk.CTkLabel(
            self.main_frame,
            text="Actualizacion segura y profesional para dispositivos embebidos",
            font=ctk.CTkFont(size=13),
            text_color="#b0c0e0",
        ).pack(anchor="w", pady=(0, 14))

        # --- Seleccion de modulo ---
        ctk.CTkLabel(
            self.main_frame,
            text="Selecciona el modulo a actualizar:",
            font=ctk.CTkFont(size=15, weight="bold"),
            text_color="#e0e8f0",
        ).pack(anchor="w")

        radios = ctk.CTkFrame(self.main_frame, fg_color="transparent")
        radios.pack(anchor="w", pady=(5, 4))
        self.radio_tablet = ctk.CTkRadioButton(
            radios,
            text="Tablet HMI (via ADB)",
            variable=self.script_var,
            value="actualizar_hmi_tablet.py",
            font=ctk.CTkFont(size=14),
            command=self._actualizar_interfaz,
        )
        self.radio_tablet.pack(side="left", padx=(0, 22))
        self.radio_esp32 = ctk.CTkRadioButton(
            radios,
            text="Carrito ESP32 (Boot + USB)",
            variable=self.script_var,
            value="actualizar_firmware_esp32.py",
            font=ctk.CTkFont(size=14),
            command=self._actualizar_interfaz,
        )
        self.radio_esp32.pack(side="left")

        # --- Instrucciones dinamicas ---
        self.lbl_instrucciones = ctk.CTkLabel(
            self.main_frame,
            text="",
            font=ctk.CTkFont(size=13, slant="italic"),
            text_color="#f0d060",
            wraplength=680,
            justify="left",
        )
        self.lbl_instrucciones.pack(anchor="w", pady=(8, 8))

        # --- Parametros del ESP32 (mostrar/ocultar) ---
        self.param_frame = ctk.CTkFrame(
            self.main_frame, fg_color="#101024", border_width=1, border_color="#2a4a7a"
        )
        self.lbl_puerto = ctk.CTkLabel(
            self.param_frame,
            text="Puerto serie (COMx):",
            font=ctk.CTkFont(size=13),
            text_color="#c0d0e0",
        )
        self.combo_puerto = ctk.CTkComboBox(
            self.param_frame,
            width=330,
            values=[self.puerto_seleccionado.get()],
            variable=self.puerto_seleccionado,
            state="readonly",
        )
        self.btn_actualizar_puertos = ctk.CTkButton(
            self.param_frame,
            text="Actualizar lista",
            width=125,
            command=self.actualizar_puertos,
            fg_color="#3a5a8a",
            hover_color="#4a6a9a",
        )
        self.chk_vmot = ctk.CTkCheckBox(
            self.param_frame,
            text="Confirmo que VMOT esta Fisicamente APAGADO",
            variable=self.vmot_confirmado,
            font=ctk.CTkFont(size=12),
            text_color="#f0d060",
            hover_color="#4a4a6a",
        )

        # --- Boton de ejecucion ---
        self.btn_run = ctk.CTkButton(
            self.main_frame,
            text=">  Ejecutar Actualizacion",
            font=ctk.CTkFont(size=16, weight="bold"),
            height=44,
            command=self.ejecutar,
            fg_color="#2a6a9a",
            hover_color="#3a8aba",
        )
        self.btn_run.pack(fill="x", pady=(6, 10))

        # --- Estado ---
        self.lbl_estado = ctk.CTkLabel(
            self.main_frame,
            text="Listo",
            font=ctk.CTkFont(size=12),
            text_color="#80b0d0",
        )
        self.lbl_estado.pack(anchor="w", pady=(0, 5))

        # --- Logs ---
        log_frame = ctk.CTkFrame(
            self.main_frame, fg_color="#0a0a1a", border_width=1, border_color="#2a4a7a"
        )
        log_frame.pack(fill="both", expand=True)

        barra = ctk.CTkFrame(log_frame, fg_color="transparent", height=30)
        barra.pack(fill="x", padx=5, pady=(5, 0))
        ctk.CTkLabel(
            barra,
            text="Registro de eventos",
            font=ctk.CTkFont(size=13, weight="bold"),
            text_color="#a0b8d0",
        ).pack(side="left")
        ctk.CTkButton(
            barra,
            text="Limpiar",
            width=70,
            height=24,
            command=self._limpiar_log,
            fg_color="#3a3a5a",
            hover_color="#5a5a8a",
            font=ctk.CTkFont(size=11),
        ).pack(side="right")

        self.txt_log = ctk.CTkTextbox(
            log_frame,
            wrap="word",
            font=ctk.CTkFont(family="Consolas", size=11),
            fg_color="#0d0d20",
            text_color="#c0d0e0",
            border_width=0,
        )
        self.txt_log.pack(fill="both", expand=True, padx=5, pady=5)

        # --- Estado interno ---
        self.process = None
        self.running = False
        self.eventos: queue.Queue[tuple[str, object]] = queue.Queue()

        if ERROR_RUTAS:
            self.log(ERROR_RUTAS, "ERROR")
            self._estado("Repositorio no encontrado", "#ff6060")
            self.btn_run.configure(state="disabled")
        else:
            self.log(f"Sistema iniciado. Repositorio: {BASE_DIR}")
        self.after(75, self._procesar_eventos)
        self._actualizar_interfaz()

    # ---------------- UI helpers ----------------
    def _crear_gradiente(self, size, color1, color2):
        img = Image.new("RGB", size, color1)
        draw = ImageDraw.Draw(img)
        width, height = size
        c1 = tuple(int(color1[i : i + 2], 16) for i in (1, 3, 5))
        c2 = tuple(int(color2[i : i + 2], 16) for i in (1, 3, 5))
        for y in range(height):
            ratio = y / height
            rgb = tuple(int(a * (1 - ratio) + b * ratio) for a, b in zip(c1, c2))
            draw.line([(0, y), (width, y)], fill=rgb)
        return ctk.CTkImage(light_image=img, dark_image=img, size=size)

    def _es_esp32(self) -> bool:
        return self.script_var.get() == "actualizar_firmware_esp32.py"

    def _actualizar_interfaz(self):
        esp32 = self._es_esp32()
        if esp32:
            self.instrucciones_esp32()
        else:
            self.instrucciones_hmi()
        if esp32:
            if not self.lbl_puerto.winfo_manager():
                self.lbl_puerto.pack(side="left", padx=(6, 6))
                self.combo_puerto.pack(side="left", padx=(0, 8))
                self.btn_actualizar_puertos.pack(side="left", padx=(0, 12))
                self.chk_vmot.pack(side="left", padx=(0, 4))
            self.param_frame.pack(fill="x", pady=(0, 10))
            if not self.puertos_consultados:
                self.actualizar_puertos()
        else:
            self.param_frame.pack_forget()

    def instrucciones_hmi(self):
        self._instrucciones(
            "Conecta una sola tablet con depuracion USB autorizada y cierra el controlador "
            "Windows (solo el dueno del WebSocket debe estar activo). La APK se compila desde la "
            "fuente canonica y se instala sin borrar los datos."
        )

    def instrucciones_esp32(self):
        self._instrucciones(
            "El carrito debe estar con VMOT fisicamente apagado (alimentado solo por USB) durante "
            "la carga. Identifica antes el puerto COM real. Si no entra solo en bootloader, "
            "mantien BOOT, pulsa RST una vez y libera."
        )

    def _instrucciones(self, texto: str):
        self.lbl_instrucciones.configure(text=texto)

    def log(self, msg: str, nivel: str = "INFO"):
        color = COLORES_LOG.get(nivel, "#c0d0e0")
        ts = time.strftime("%H:%M:%S")
        try:
            self.txt_log.insert("end", f"[{ts}] ", "timestamp")
            self.txt_log.insert("end", f"{msg}\n", nivel)
            self.txt_log.tag_config("timestamp", foreground=COLORES_LOG["timestamp"])
            self.txt_log.tag_config(nivel, foreground=color)
            self.txt_log.see("end")
            self.update_idletasks()
        except Exception:
            pass

    def _limpiar_log(self):
        self.txt_log.delete("0.0", "end")

    def _estado(self, texto: str, color: str = "#80b0d0"):
        self.lbl_estado.configure(text=texto, text_color=color)

    def actualizar_puertos(self):
        if self.running or self.btn_actualizar_puertos.cget("state") == "disabled":
            return
        self.btn_actualizar_puertos.configure(state="disabled", text="Buscando...")
        self.log("Consultando puertos COM de Windows...", "INFO")
        threading.Thread(target=self._hilo_puertos, daemon=True).start()

    def _hilo_puertos(self):
        try:
            self.eventos.put(("puertos", _detectar_puertos_windows()))
        except Exception as error:
            self.eventos.put(("puertos_error", str(error)))

    # ---------------- Ejecucion ----------------
    def ejecutar(self):
        if self.running:
            messagebox.showwarning("En ejecucion", "Ya hay una actualizacion en curso.")
            return

        script = self.script_var.get()
        if SCRIPTS_DIR is None:
            messagebox.showerror("Repositorio no encontrado", ERROR_RUTAS)
            return
        ruta = SCRIPTS_DIR / script
        if not ruta.is_file():
            self.log(f"Script no encontrado: {ruta}", "ERROR")
            messagebox.showerror("Error", f"No se encontro el script:\n{ruta}")
            return

        args = [ruta]
        if self._es_esp32():
            if not self.vmot_confirmado.get():
                messagebox.showerror(
                    "Confirmacion obligatoria",
                    "Debes confirmar que VMOT esta fisicamente apagado antes de cargar.",
                )
                return
            puerto = self.puertos.get(self.puerto_seleccionado.get(), "")
            if not puerto:
                messagebox.showerror("Error", "Selecciona un puerto COM de la lista y confirma que corresponde al ESP32-S3.")
                return
            args += ["--port", puerto, "--confirmar-vmot-apagado"]

        if not messagebox.askyesno(
            "Confirmar", f"¿Deseas ejecutar la actualizacion con '{script}'?"
        ):
            return
        try:
            comando = _comando_python(ruta, args[1:])
        except RuntimeError as error:
            self.log(str(error), "ERROR")
            messagebox.showerror("Python no encontrado", str(error))
            return

        self.running = True
        self.btn_run.configure(state="disabled", text="Ejecutando...")
        self._estado("Actualizacion en progreso...", "#f0d060")
        self.log(f"Iniciando actualizacion: {script}", "INFO")

        threading.Thread(target=self._hilo_ejecucion, args=(comando,), daemon=True).start()

    def _hilo_ejecucion(self, comando: list[str]):
        try:
            self.process = subprocess.Popen(
                comando,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                errors="replace",
                cwd=BASE_DIR,
            )
            for linea in iter(self.process.stdout.readline, ""):
                if linea:
                    self.eventos.put(("log", linea.rstrip()))
            return_code = self.process.wait()
            self.eventos.put(("fin", return_code))
        except Exception as e:
            self.eventos.put(("excepcion", str(e)))

    def _procesar_eventos(self):
        try:
            while True:
                tipo, dato = self.eventos.get_nowait()
                if tipo == "log":
                    self.log(str(dato), "INFO")
                elif tipo == "fin":
                    self.running = False
                    self.process = None
                    self.btn_run.configure(state="normal", text=">  Ejecutar Actualizacion")
                    if dato == 0:
                        self.log("Actualizacion completada con exito.", "SUCCESS")
                        self._estado("Actualizacion completada", "#60d060")
                        messagebox.showinfo("Completado", "La actualizacion finalizo correctamente.")
                    else:
                        self.log(f"El script finalizo con codigo de error: {dato}", "ERROR")
                        self._estado("Error en la actualizacion", "#ff6060")
                        messagebox.showerror("Error", f"El script fallo con codigo {dato}. Revisa los logs.")
                elif tipo == "excepcion":
                    self.running = False
                    self.process = None
                    self.btn_run.configure(state="normal", text=">  Ejecutar Actualizacion")
                    self.log(f"Excepcion al ejecutar el script: {dato}", "ERROR")
                    self._estado("Error critico", "#ff6060")
                    messagebox.showerror("Error", str(dato))
                elif tipo == "puertos":
                    self.btn_actualizar_puertos.configure(state="normal", text="Actualizar lista")
                    self.puertos_consultados = True
                    encontrados = list(dato)
                    self.puertos = {
                        f"{puerto} — {descripcion}": puerto for puerto, descripcion in encontrados
                    }
                    opciones = list(self.puertos)
                    if opciones:
                        self.combo_puerto.configure(values=opciones)
                        self.puerto_seleccionado.set(opciones[0])
                        self.log(f"Puertos detectados: {len(opciones)}. Confirma cuál corresponde al ESP32-S3.", "INFO")
                    else:
                        aviso = "No se detectaron puertos COM. Conecta el ESP32-S3 por cable USB de datos."
                        self.combo_puerto.configure(values=[aviso])
                        self.puerto_seleccionado.set(aviso)
                        self.log(aviso, "WARNING")
                elif tipo == "puertos_error":
                    self.btn_actualizar_puertos.configure(state="normal", text="Actualizar lista")
                    self.puertos_consultados = True
                    self.log(f"No se pudieron consultar puertos: {dato}", "ERROR")
        except queue.Empty:
            pass
        self.after(75, self._procesar_eventos)


if __name__ == "__main__":
    try:
        import PIL  # noqa: F401
    except ImportError:
        print("Error: Necesitas instalar Pillow. Ejecuta: pip install Pillow")
        sys.exit(1)
    try:
        import customtkinter  # noqa: F401
    except ImportError:
        print("Error: Necesitas instalar customtkinter. Ejecuta: pip install customtkinter")
        sys.exit(1)

    app = FirmwareUpdaterApp()
    app.mainloop()
