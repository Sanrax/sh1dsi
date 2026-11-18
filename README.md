# Silent Hill for Nintendo DSi

Native port in development using BlocksDS. Build with GNU Make and an installed
Wonderful Toolchain with BlocksDS 1.24.0.

```sh
make -j2
```

The output is `sh1dsi.nds`. Launch it in TWL mode from a DSi homebrew launcher
on console SD or from Pico-Launcher on DSpico. Physical boot testing is pending.
Development measurements and the upstream revision are in [port status](docs/PORT_STATUS.md).

Prepare local runtime data with Python 3.11 or later. Supply the CUE path and
leave the original BIN next to it. The converter verifies the entire track
before reading named files from the game's archive.

```sh
python3 tools/prepare_assets.py "/path/to/Silent Hill (USA).cue"
```

Copy `dist/sh1dsi/` to the root of the same SD card as the launcher will use
for the port. The data belongs to the original game and is for local use.
