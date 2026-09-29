#!/usr/bin/env bash
set -euo pipefail
cd /work/build
CMD=$(ninja -t commands src/CMakeFiles/cataclysm-tiles-common.dir/zzip.cpp.o | tail -1)
CMD=${CMD//\/work\/src\/zzip.cpp/\/work\/tools\/zzip_update_file.cpp}
CMD=${CMD//src\/CMakeFiles\/cataclysm-tiles-common.dir\/zzip.cpp.o.d/\/tmp\/zzip_update_file.cpp.o.d}
CMD=${CMD//src\/CMakeFiles\/cataclysm-tiles-common.dir\/zzip.cpp.o/\/tmp\/zzip_update_file.cpp.o}
CMD=${CMD// -Werror / }
echo "COMPILE: ${CMD:0:120}..."
eval "$CMD"
ls -la /tmp/zzip_update_file.cpp.o
LINK=$(ninja -t commands src/cataclysm-tiles | tail -1)
LINK=${LINK//src\/CMakeFiles\/cataclysm-tiles.dir\/main.cpp.o/\/tmp\/zzip_update_file.cpp.o}
LINK=${LINK//-o src\/cataclysm-tiles/-o \/tmp\/zzip_update_file}
echo "LINK: ${LINK:0:120}..."
eval "$LINK"
ls -la /tmp/zzip_update_file
