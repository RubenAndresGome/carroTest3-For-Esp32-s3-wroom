#!/usr/bin/env sh
set -eu
APP_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
if [ ! -x "$APP_ROOT/.venv/bin/python" ]; then
  printf '%s\n' 'Falta .venv. Crea el entorno e instala requirements.lock.'
  exit 1
fi
exec "$APP_ROOT/.venv/bin/python" "$APP_ROOT/app.py" "$@"
