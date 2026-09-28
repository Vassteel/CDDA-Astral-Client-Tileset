#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "$project_dir/tools/hybrid-updater/updater.py" gui --client "$project_dir/artifacts/client" "$@"
