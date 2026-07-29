"""Punto de entrada de la aplicación local del robot."""

from __future__ import annotations

import argparse
import threading
import webbrowser


def _load_application() -> tuple[object, object]:
    """Carga dependencias tarde para mostrar un error útil al hacer doble clic."""
    try:
        from waitress import create_server
        from robot_app.app_factory import create_app
    except ModuleNotFoundError as exc:
        message = (
            f"Falta la dependencia Python: {exc.name}\n\n"
            "Primero ejecuta INICIAR_ROBOT.bat desde la carpeta principal. "
            "Ese archivo instalará el entorno local y después abrirá el panel."
        )
        try:
            from tkinter import messagebox
            messagebox.showerror("Robot ESP32-S3 · instalación incompleta", message)
        except Exception:
            print(message)
            input("Presiona Enter para cerrar...")
        raise SystemExit(2) from exc
    return create_server, create_app


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Panel local del Robot ESP32-S3")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", default=8080, type=int)
    parser.add_argument("--no-browser", action="store_true")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    create_server, create_app = _load_application()
    app = create_app()
    url = f"http://localhost:{args.port}/"
    print(f"Panel del Robot ESP32-S3: {url}")
    print("Ctrl+C para cerrar de forma segura.")
    if not args.no_browser:
        threading.Timer(0.8, webbrowser.open, args=(url,)).start()
    server = create_server(app, host=args.host, port=args.port, threads=8, ident="RobotS3")

    def shutdown_host() -> None:
        server.close()
        for channel in list(server._map.values()):
            try:
                channel.close()
            except Exception:
                pass
        server.task_dispatcher.shutdown(cancel_pending=True)

    app.extensions["host_shutdown"] = shutdown_host
    try:
        server.run()
    finally:
        app.extensions["robot_service"].close()


if __name__ == "__main__":
    main()
