# Port status

The school piano interaction runs natively on ARM9. The D-pad moves the
original cursor. The two key regions, snapping, incorrect-sequence resets,
restart on the first key, note delays and five-key solution follow
`PianoPuzzle_Control` in MAP1_S01. The solved flag lasts for the session.
The build enters the 3D music room directly. Solving the piano plays the
silver medallion fall with its impact sound and landing camera. Room traversal,
Harry, messages, pickup, saving and progression are pending. The full game is unplayable.

## DSi verification

- BlocksDS 1.24.0 and Wonderful GCC 16.2.0 build a DSi-exclusive ROM with
  unit code 3. melonDS 1.1 direct boot used external BIOS, firmware and a
  private NAND copy. The SDK filesystem sample and port both reported TWL
  mode. melonDS reported 16 MB RAM.
- USA 1.1, single MODE2/2352 track, 616,494,480 bytes,
  SHA-1 `34278d31d9b9b12b3b5db5e45bcbe548991ecbc7`. All non-XA file-table
  entries agreed with upstream silentassets. MAP1_S01 matched the upstream
  SHA-1. Repeated local conversions were byte-identical.
- The room and piano loaded from virtual console SD and emulated DLDI in
  separate tests. Each route entered the piano, backed out to the same room
  frame, and completed the five-key solution. The lower LCD stayed black.
  Physical console SD and DSpico boot tests are pending.
- Three 4-bit texture pages use 80 KiB of texture VRAM and 1 KiB of palettes.
  Eight SCFD00 instances, the open lid and the medallion contribute 764 quads
  per view. Camera position, zoom and cutscene borders come from MAP1_S01.
  Static frames persist without geometry resubmission. Whole-polygon alpha,
  perspective texture interpolation, missing characters and lighting still
  differ from the PS1.
- All 49,152 overview pixels matched the prepared 5-bit colors. The cursor
  changed 49 pixels in the initial keyboard view. The event pack stores four
  MEP101.VAB samples and plays them through native sound channels. An isolated
  melonDS recording peaked at 15,096 in signed 16-bit PCM after a key strike.
  PS1 reverb and envelope matching are pending.
- After the fifth key, the PS1 event held its first camera for 90 VBlanks,
  swung the medallion for 32, dropped it for 44 and held for 62 before the
  second camera. The port advances the fixed-point motion every two simulation
  ticks. A DLDI replay in melonDS showed the zoom, swing, drop and stationary
  hold. The isolated impact recording peaked at 15,710 in signed 16-bit PCM.
  Both the full replay and a Start skip from virtual console SD ended at the
  previously verified landing frame, identical across all 49,152 pixels.
  The event holds that view until player traversal is available.

## Reference measurements

Beetle PSX software core, Debian build 0.9.38.6+git20260515+ds-2, NTSC-U
USA 1.1, digital pad, nominal 59.94 Hz. The owner supplied an SCPH-7001
BIOS. The core warned that its SHA-1 differed from its expected SCPH-5501.
The street route started a new game. For the school route the diagnostic
frontend seeded a private in-memory Continue slot at the piano and played
the event with controller inputs. The diagnostic branch contains the runner.

| Case | Sample | Measured interval |
| --- | --- | --- |
| PS1 opening street, including forward input | 368 consecutive buffer changes | All 2 VBlanks, 33.37 ms, 29.97 updates/s |
| PS1 school keyboard | 1,322 consecutive buffer changes | All 1 VBlank, 16.68 ms, 59.94 updates/s |
| melonDS idle piano event, ARM timer | 600 display-loop intervals | 16.713 / 16.715 / 16.717 ms min/mean/max |

The game's uncapped VBlank counter corroborated both PS1 measurements. The
port uses a 16.683 ms keyboard simulation step. The DSi timer measurement
covers the idle event only. Real DSi and general gameplay performance are open.

For both medallion cameras the PS1 capture measured 192 content lines
in a 240-line picture. After matching the projection and borders, mean absolute
RGB channel differences against the resampled PS1 frames were 10.5/255
(hanging) and 8.7/255 (fallen). The keyboard comparison measured 4.0/255.
PS1 frames use the 320-pixel active width starting at column 14 in the core's
350-pixel output. The fixed views sample steps 11 and 15 of the event.

The PS1 room reference now includes gameplay frames before the puzzle and
after control returns. In both, the locally decoded HB_BASE.ANM idle pose
matched all 17 non-root bone matrices and translations exactly. HERO.ILM
uses 19 active meshes and 332 distinct vertices in this unarmed pose. Faces
can reuse vertices transformed by an earlier bone in the model draw order.
Harry's geometry and pose have only been checked on the host so far.

## Source

[silent-hill-decomp at ecbb0d8](https://github.com/shdecompilations/silent-hill-decomp/tree/ecbb0d8e3881cf2f0cb9fa700f3bb1a74b6d34ce),
Copyright (C) 2026 shdecompilations, GPL-3.0. Relevant sources include
`src/maps/map1_s01/map1_s01.c`, USA map symbols, `ControllerData_AnalogToDigital`,
`GameFs_BgEtcGfxLoad`, `Gfx_CursorDraw` and `tools/silentassets/extract.py`.

## Next

1. Render Harry from HERO.ILM and HB_BASE.ANM in the measured gameplay camera.
2. Integrate room collision, player movement and the two interaction triggers.
3. Complete the medallion pickup and persist its progression flags.
