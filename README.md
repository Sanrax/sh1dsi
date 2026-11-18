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

Run `make sdcard` after conversion. Copy the contents of `dist/` to the
card root. The directory includes the ROM and the two local data files. Keep
the generated game data on your own devices.

The current entry point is Midwich's music room. Press A to approach the
piano and A again to examine the keys. Move the cursor with the D-pad and
press A to strike a key. B backs out one view at a time. Solving the puzzle
starts the silver medallion cutscene. Start skips to its landing camera.
Movement and pickup are pending.
