# Port status

The native ARM9/ARM7 application boots in melonDS 1.1 with external DSi BIOS,
firmware, a private NAND copy and direct boot. Both the BlocksDS filesystem
sample and the port reported DSi mode. melonDS logged 16 MB RAM. The port
displays on the top LCD and blanks the lower screen.

The USA 1.1 CUE references one MODE2/2352 track, 616,494,480 bytes,
SHA-1 `34278d31d9b9b12b3b5db5e45bcbe548991ecbc7`.
Primary source is [silent-hill-decomp at ecbb0d8](https://github.com/shdecompilations/silent-hill-decomp/tree/ecbb0d8e3881cf2f0cb9fa700f3bb1a74b6d34ce).
BlocksDS 1.24.0 and Wonderful GCC 16.2.0 produced a TWL-only header (unit 3).

Gameplay, audio, frame pacing and physical boot on console SD and DSpico
are unverified. There is no PS1 gameplay baseline yet.

## Next

1. Read a specific map asset locally through the verified disc's file table.
2. Render the asset on the top LCD and port its associated interaction.
3. Capture the same event on PS1 and measure update intervals on both builds.
