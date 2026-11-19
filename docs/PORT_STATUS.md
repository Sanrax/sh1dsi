# Port status

Harry starts at the Midwich piano. He can solve the keyboard puzzle, watch
or skip the falling silver medallion, decline or accept it, walk through the
eastern hallway, descend the stairs, unlatch the first-floor door and enter
the southern hallway. He can enter the infirmary, walk to the desk and return.
Inventory and unlocked doors last for the session. Saving and the full game
remain unplayable.

## Build and media

- BlocksDS 1.24.0 and Wonderful GCC 16.2.0 produce a TWL-exclusive ROM
  (unit code 3). Conversion and clean compilation are reproducible.
- USA 1.1 CUE, one MODE2/2352 track, 616,494,480 bytes, SHA-1
  `34278d31d9b9b12b3b5db5e45bcbe548991ecbc7`. Non-XA file entries agreed
  with upstream silentassets. All converted data stays on the owner's machine.
- melonDS 1.1 direct boot uses external BIOS, firmware and private NAND.
  The SDK filesystem sample and port verified TWL mode, and the emulator
  reported 16 MB RAM. Physical console SD and DSpico/Pico-Launcher remain
  untested. The lower screen stays black in production.

## Frame pacing

PS1 references use Beetle PSX software rendering, USA 1.1, SCPH-7001 BIOS,
digital pad and the 59.94 Hz clock. The street replay starts a new game.
School replays seed a private Continue slot and then use controller input.
See the replay instructions on `diagnostics/dsi`. Native simulation uses
16.683 ms steps. Hardware timing is pending.

| Case | Sample | Measured interval |
| --- | --- | --- |
| PS1 opening street | 368 buffer-change intervals | All 2 VBlanks, 33.37 ms |
| PS1 school keyboard | 1,322 buffer-change intervals | All 1 VBlank, 16.68 ms |
| PS1 silver pickup and first corridor view | 47 intervals on each of two prompts and 67 in room 32 | All 2 VBlanks, 33.37 ms |
| PS1 crossing room 32 after pickup | 474 buffer-change intervals | 442 at 33.37 ms and 32 at 50.05 ms |
| PS1 eastern hallway through stair descent | 357 buffer-change intervals | All 33.37 ms |
| PS1 crossing the southern hallway | 176 buffer-change intervals | 70 at 33.37 ms and 106 at 50.05 ms |
| PS1 infirmary from entrance to save interaction | 255 buffer-change intervals | 1 at 16.68 ms, 247 at 33.37 ms and 7 at 50.05 ms |
| melonDS idle music room with Harry | 300 scene submissions | 16.715 / 33.317 / 33.432 ms min/mean/max, 2.893 ms mean ARM9 submission time |
| melonDS rotating silver pickup with prompt | 300 scene intervals | 16.713 / 33.374 / 33.432 ms min/mean/max, 3.291 ms mean ARM9 submission time |
| melonDS idle corridor after collecting the medallion | 300 scene intervals | 16.713 / 33.317 / 33.432 ms min/mean/max, 4.137 ms mean ARM9 submission time |
| melonDS idle eastern hallway with medallion | 300 scene intervals | 16.717 / 33.373 / 49.966 ms min/mean/max, 5.506 ms mean ARM9 submission time |
| melonDS first-floor stair entry with medallion | 300 scene intervals | 16.717 / 33.374 / 33.438 ms min/mean/max, 5.468 ms mean ARM9 submission time |
| melonDS idle keyboard | 600 display-loop intervals | 16.713 / 16.715 / 16.717 ms min/mean/max |
| melonDS southern hallway with medallion | 300 scene intervals | 16.717 / 33.373 / 49.968 ms min/mean/max, 5.138 ms mean ARM9 submission time |
| melonDS infirmary entrance with medallion | 300 scene intervals | 16.711 / 33.317 / 33.434 ms min/mean/max, 3.920 ms mean ARM9 submission time |

## Scene and interaction checks

- The top screen uses the native 256×192 viewport. Harry has 19 meshes and
  332 transformed vertices. All 17 non-root transforms matched the original
  in sampled idle and pickup poses. A 1,020-VBlank movement replay matched
  13 sampled headings with up to 3.6 cm of horizontal position error.
- The five-key puzzle, wrong-key resets and medallion drop use the original
  assets and event timings. The native overview matched all 49,152 prepared
  5-bit pixels. The rotating pickup uses original model, pose, text and font.
  Virtual console SD and DLDI replays declined, accepted and checked the
  single-item inventory. The native prompt matched all 329 bright green
  item-name pixels in the resampled PS1 image.
- Music room 39, hallways 32 and 31, and first-floor room 12 stream from
  local sidecars. Two door pairs use their original BASE.VAB samples.
  The PS1 and native stair warps matched in both directions. Whole-frame
  mean absolute RGB error (out of 255) was 4.0 at the keyboard, 19.3 at
  the first corridor entrance, 17.9 at the eastern entrance, 17.5 on the
  first floor and 15.3 upstairs. Camera smoothing, shading, footsteps,
  enemies, fog and PS1 audio envelopes/reverb remain incomplete.
- Point 50 uses the original half-metre facing ray, plays BASE.VAB effect
  1342, waits 0.2 s and displays MAP1_S00 message 13. Acknowledging the
  message releases the latch. The next action enters room 13 through point
  34. A console SD diagnostic kept the medallion on two visits and matched
  PS1 spawns at (611123, 81920), yaw 3072, and (438271, 201113), yaw 1024.
  The production DLDI build displayed the unlock message (243 bright pixels)
  and matched both spawn positions and headings using the release ELF symbols.
  Room 13 streams SC0300, with 1,843 quads, three 4-bit textures and 43
  collision edges. At entry, the GPU retained 1,143 polygons and 4,581
  vertices. One of 300 intervals exceeded 34 ms. Entry RGB error was 17.7/255, and return error was 18.0/255.
- The PS1 `infirmary` route checks flag 1079 after the latch, carries one
  medallion into room 9, and operates the save point at MAP1_S00 point 89.
  It stops at the save interaction before writing a memory card.

- The DLDI production build streamed room 9 (1,211 scenery quads, 30 batches,
  three indexed textures, 45 collision edges). Harry landed at (233472, 587366),
  yaw 2048, and returned to room 13 at (591462, 77004), yaw 0. Both matched
  the PS1 trace. He could reach the eastern desk camera, walk into the counter
  twice to the same stopping X position and back away. A console SD replay
  kept the medallion through both doors. At entry the GPU retained 411
  polygons and 1,589 vertices. Whole-frame RGB error
  was 23.2/255 at entry and 27.0/255 at return. Lighting, camera transitions,
  the notebook and the two medical items still differ from PS1.

## Next

1. Load the original notebook, first-aid kit and health drink models at their map positions.
2. Implement the original save-point interaction and verify save/load across a fresh DSi boot.
3. Match walking and turning sounds, then compare camera movement and lighting along the hallway.

Based on the original GPL-3.0 decompilation, Copyright (C) 2026
shdecompilations. [Pinned upstream source](https://github.com/shdecompilations/silent-hill-decomp/tree/ecbb0d8e3881cf2f0cb9fa700f3bb1a74b6d34ce)
