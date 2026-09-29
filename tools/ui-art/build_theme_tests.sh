#!/usr/bin/env bash
# Build and run the Astral UI theme tests (tests/ui_hybrid_theme_test.cpp) as a small
# stand-alone Catch2 executable linked against the already-built game objects, so the
# full `cata_test` target (every test file) does not have to be compiled.
#
#   tools/ui-art/build_theme_tests.sh [build_dir]        # default: build
#
# Requires a completed `ninja cataclysm-tiles` in that build dir (the objects are reused).
set -euo pipefail
repo="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
build="${1:-$repo/build}"
out="$build/astral-test"
mkdir -p "$out"
python3 - "$build" "$out" <<'EOF'
import re, sys
build, out = sys.argv[1], sys.argv[2]
s = open(build + "/build.ninja").read()
i = s.index("build src/cataclysm-tiles:")
block = s[i:s.index("\n\n", i)]
objs = re.findall(r"src/CMakeFiles/cataclysm-tiles(?:-common)?\.dir/\S+\.o", block)
objs = [o for o in objs if not o.endswith("/main.cpp.o")]
link = re.search(r"LINK_LIBRARIES = (.*)", block).group(1)
j = s.index("build src/CMakeFiles/cataclysm-tiles-common.dir/ui_hybrid_widgets.cpp.o:")
blk = s[j:s.index("\n\n", j)]
defines = re.search(r"DEFINES = (.*)", blk).group(1)
includes = re.search(r"INCLUDES = (.*)", blk).group(1)
open(out + "/objs.txt", "w").write(" ".join(build + "/" + o for o in objs))
open(out + "/link.txt", "w").write(link.replace(" src/third-party/", " " + build + "/src/third-party/"))
open(out + "/defines.txt", "w").write(defines)
open(out + "/includes.txt", "w").write(includes)
EOF
cat > "$out/main.cpp" <<'EOF'
#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"
EOF
flags="-fsigned-char -O1 -g0 -DNDEBUG -std=c++17 -pthread -Wall -Wextra -Wno-unused-parameter"
defines="$(cat "$out/defines.txt")"
includes="$(cat "$out/includes.txt") -I$repo/tests"
# shellcheck disable=SC2086
g++ $defines $includes $flags -c "$out/main.cpp" -o "$out/main.o"
# shellcheck disable=SC2086
g++ $defines $includes $flags -c "$repo/tests/ui_hybrid_theme_test.cpp" -o "$out/theme_test.o"
# shellcheck disable=SC2046,SC2086
g++ -pthread -o "$out/astral_ui_test" "$out/main.o" "$out/theme_test.o" $(cat "$out/objs.txt") $(cat "$out/link.txt")
cd "$repo"
LD_LIBRARY_PATH="/home/claude/deps/install/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" "$out/astral_ui_test" "[astral]" "$@"
