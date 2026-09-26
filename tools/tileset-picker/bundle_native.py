#!/usr/bin/env python3
"""Collect a locally built Linux client and its non-glibc shared libraries.

Run inside the same environment used to build the executable. Game data and
the licensed Steamworks library are for the owner's local installation.
"""

import argparse
from pathlib import Path
import re
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    libraries = output / "lib"
    libraries.mkdir(parents=True, exist_ok=True)
    binary = output / "cataclysm-tiles"
    staged_binary = output / "cataclysm-tiles.new"
    shutil.copy2(args.binary, staged_binary)
    subprocess.run(["strip", "--strip-unneeded", str(staged_binary)], check=True)
    staged_binary.replace(binary)
    dependencies = subprocess.check_output(["ldd", str(args.binary)], text=True)
    if "not found" in dependencies:
        raise RuntimeError(dependencies)
    system = {"libc.so.6", "libm.so.6", "libdl.so.2", "libpthread.so.0", "librt.so.1"}
    manifest = []
    for name, source in re.findall(r"\s+(\S+) => (/\S+) \(", dependencies):
        if name in system:
            continue
        shutil.copy2(source, libraries / name)
        manifest.append(f"{name}\t{source}")
    (output / "native-libraries.txt").write_text("\n".join(manifest) + "\n")
    (output / "steam_appid.txt").write_text("2330750\n")
    print(f"Bundled {binary} with {len(manifest)} libraries.")


if __name__ == "__main__":
    main()
