# Main-menu tileset picker

Adds **Tilesets** to CDDA's main menu (shortcut **S**). Browse installed world
tilesets using the same sample street, room, characters, zombies and vehicle.
Zoom with **+ / -**, choose **Apply** to save, or **Esc / Cancel** to leave the
current choice unchanged. Isometric and ASCII tilesets are supported.

Overmap-only tilesets are identified in the picker and remain configurable in
**Settings → Options → Graphics → Choose overmap tileset**. The preview uses
base tileset artwork; world-specific mod graphics load when entering that world.

## Source and build

Based on the installed Steam 0.I client source:
`KorGgenT/Cataclysm-DDA@588986c1f8542ca74b7efd8ffd84594ff065b6de`.
This is a client modification, not a JSON content mod.

On Linux, install a C++17 compiler, CMake, Ninja, SDL2, SDL2_image, SDL2_ttf,
SDL2_mixer, zlib, and the corresponding development packages. This Steam branch
also needs a platform-matching Steamworks API library.

```sh
cmake -S . -B build -G Ninja \
  -DTILES=ON -DCURSES=OFF -DSOUND=ON -DTESTS=OFF \
  -DUSE_HOME_DIR=OFF -DUSE_PREFIX_DATA_DIR=OFF \
  -DLOCALIZE=OFF -DCMAKE_BUILD_TYPE=Release \
  -DSTEAM_API_LIBRARY=/absolute/path/to/libsteam_api.so
cmake --build build --parallel 8
```

Run from a matching game data directory. Keep the game's original binary for
rollback. The source retains CDDA's existing license and attribution.

## Validation

Use an isolated profile with `--userdir` when testing. Check mouse and keyboard
selection, ordinary/isometric/ASCII previews, zoom, cancellation without writes,
Apply followed by restart, broken tileset recovery, and starting a world after
applying. See the local `artifacts/qa` folder for screenshots and test evidence.
