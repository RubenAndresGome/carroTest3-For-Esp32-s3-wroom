# Scripts del HMI de escritorio

- `run.bat` y `run.sh`: ejecutan el HMI desde código fuente.
- `install_online.ps1`: prepara dependencias con acceso a Internet.
- `install_offline.bat`: instala usando las ruedas locales versionadas.
- `download_wheels.ps1`: actualiza el almacén local de dependencias.
- `build_exe.ps1`: construye `dist/RobotS3.exe`; acepta `-Offline`.
- `build_exe_offline.bat`: acceso directo para la compilación offline.
- `frontend/`: prepara, desarrolla y compila el frontend experimental Vite.

Los entornos virtuales, salidas de compilación y el ejecutable son artefactos
locales y están excluidos por `.gitignore`.
