# Port status

The school piano interaction runs natively on ARM9. The D-pad moves the
original cursor. The two key regions, snapping, incorrect-sequence resets,
restart on the first key, note delays and five-key solution follow
`PianoPuzzle_Control` in MAP1_S01. The solved flag lasts for the session.
The build enters this event directly. Room traversal, messages,
the medallion cutscene, saving and the full game are pending.

## Verified

- BlocksDS 1.24.0 and Wonderful GCC 16.2.0 build a DSi-exclusive ROM with
  unit code 3. melonDS 1.1 direct boot used external BIOS, firmware and a
  private NAND copy. The SDK filesystem sample and port both reported TWL
  mode. melonDS reported 16 MB RAM.
- USA 1.1, single MODE2/2352 track, 616,494,480 bytes,
  SHA-1 `34278d31d9b9b12b3b5db5e45bcbe548991ecbc7`. All non-XA file-table
  entries agreed with upstream silentassets. MAP1_S01 matched the upstream
  SHA-1. Repeated local conversions were byte-identical.
- melonDS loaded the initial 99,860-byte event pack from virtual console SD, then
  from emulated DLDI with a fresh empty console SD. Both storage tests
  exercised examine, move and cancel. With console SD, button input solved
  the puzzle and the solved flag prevented re-entry. Physical tests pending.
- All 49,152 overview pixels matched the prepared 5-bit RGB values on
  the 256 x 192 top LCD. The close-up differed at the cursor as expected.
  This comparison checks presentation of the converted TIM, not a full
  PS1 framebuffer. The bottom LCD is black.
- Event simulation uses a provisional 16.667 ms step independent of display
  updates. A PS1 same-event reference and a gameplay frame-rate baseline are
  pending. The school event is the only playable interaction.

- A separate diagnostic build sampled 600 successive idle event loop intervals
  on the emulated ARM timer. Min/mean/max were 16.713/16.715/16.717 ms.
- The current 156,080-byte pack adds three one-shot samples from MEP101.VAB.
  Seven pitched keys share one waveform. Muted keys play the mechanical
  strike and delayed release. A melonDS-only audio capture registered a
  15,096 peak in signed 16-bit PCM after a strike and silence before.
  SPU reverb, envelopes and output mixing are pending fidelity comparisons.
  Repeating the five-key input test with audio enabled passed.

## Source

[silent-hill-decomp at ecbb0d8](https://github.com/shdecompilations/silent-hill-decomp/tree/ecbb0d8e3881cf2f0cb9fa700f3bb1a74b6d34ce),
Copyright (C) 2026 shdecompilations, GPL-3.0. Relevant sources include
`src/maps/map1_s01/map1_s01.c`, USA map symbols, `ControllerData_AnalogToDigital`,
`GameFs_BgEtcGfxLoad`, `Gfx_CursorDraw` and `tools/silentassets/extract.py`.

## Next

1. Convert the SCFD00 music room geometry and its three texture pages.
2. Render the room from the original medallion-event camera.
3. Capture PS1 reference timing and check the view and audio against it.
