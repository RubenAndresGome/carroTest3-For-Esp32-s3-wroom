"""CLI segura para analizar y compactar la base SQLite de la HMI."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "desktop_app"))

from robot_app.compaction import compact_database  # noqa: E402


def main() -> int:
    parser = argparse.ArgumentParser(description="Compactar repeticiones consecutivas de telemetría")
    parser.add_argument("--database", type=Path, default=ROOT / "tmp_db" / "robot.sqlite3")
    parser.add_argument("--apply", action="store_true", help="Aplicar cambios; crea respaldo automáticamente")
    parser.add_argument("--vacuum", action="store_true", help="Recuperar físicamente todo el espacio libre")
    args = parser.parse_args()
    if args.vacuum and not args.apply:
        parser.error("--vacuum requiere --apply")
    result = compact_database(args.database, apply=args.apply, vacuum=args.vacuum)
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
