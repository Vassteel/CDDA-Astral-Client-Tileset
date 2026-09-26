#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
client_dir="$project_dir/artifacts/client"
cd -- "$client_dir"
export LD_LIBRARY_PATH="$client_dir/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec ./cataclysm-tiles --basepath "" --userdir "$client_dir/" "$@"
