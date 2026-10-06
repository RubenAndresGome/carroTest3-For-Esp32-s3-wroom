#!/usr/bin/env bash
# ==============================================================================
# scripts/pull_telemetry.sh
# Extracción automatizada y reproducible de base de datos de telemetría SQLite
# desde la tablet Android (vía ADB) hacia tmp_db/robot.sqlite3
# ==============================================================================
set -euo pipefail

PACKAGE_NAME="mx.ik.robots3"
INTERNAL_DB="files/robot_s3/robot.sqlite3"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET_DIR="${REPO_ROOT}/tmp_db"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
TARGET_BACKUP="${TARGET_DIR}/robot_${TIMESTAMP}.sqlite3"
TARGET_LATEST="${TARGET_DIR}/robot.sqlite3"

mkdir -p "${TARGET_DIR}"

echo "=== Extracción de Telemetría Forense vía ADB ==="
echo "[1/4] Verificando conectividad de dispositivo Android..."

DEVICES=$(adb devices | awk 'NR>1 && $2=="device" {print $1}')
if [ -z "${DEVICES}" ]; then
    echo "ERROR: No se detectó ningún dispositivo Android autorizado por ADB." >&2
    echo "Asegúrese de conectar la tablet por USB y autorizar la depuración." >&2
    exit 1
fi

DEVICE_ID=$(echo "${DEVICES}" | head -n1)
echo "      Dispositivo detectado: ${DEVICE_ID}"

echo "[2/4] Extrayendo base de datos SQLite (${PACKAGE_NAME})..."
adb -s "${DEVICE_ID}" exec-out run-as "${PACKAGE_NAME}" cat "${INTERNAL_DB}" > "${TARGET_BACKUP}"

FILE_SIZE=$(wc -c < "${TARGET_BACKUP}" | tr -d ' ')
if [ "${FILE_SIZE}" -lt 4096 ]; then
    echo "ERROR: El archivo extraído es demasiado pequeño (${FILE_SIZE} bytes). Posible error de lectura." >&2
    exit 1
fi
echo "      Base de datos extraída: ${FILE_SIZE} bytes -> ${TARGET_BACKUP}"

echo "[3/4] Verificando integridad de SQLite..."
if command -v sqlite3 >/dev/null 2>&1; then
    INTEGRITY=$(sqlite3 "${TARGET_BACKUP}" "PRAGMA integrity_check;")
    if [ "${INTEGRITY}" != "ok" ]; then
        echo "ERROR: Falló el chequeo de integridad de SQLite: ${INTEGRITY}" >&2
        exit 1
    fi
    echo "      Integridad validada: ok"
else
    echo "      sqlite3 CLI no disponible en host; omitiendo PRAGMA local."
fi

echo "[4/4] Actualizando enlace a ${TARGET_LATEST}..."
cp -f "${TARGET_BACKUP}" "${TARGET_LATEST}"

echo ">>> Extracción exitosa. Telemetría lista en tmp_db/robot.sqlite3 <<<"
