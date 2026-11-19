# Port status

Harry starts at the Midwich piano. He can solve the keyboard puzzle, watch
or skip the falling silver medallion, decline or accept it, walk through the
eastern hallway, descend the stairs, unlatch the first-floor door and enter
the southern hallway. The courtyard connects the southern and northern halls.
Harry can inspect the clock tower door and both medallion slots.
In the infirmary he can pick up both medical supplies and use the notebook.
A cold boot can resume at the desk with the puzzle,
door latch and pickups intact. The full game remains unplayable.

## Build and media

- BlocksDS 1.24.0 and Wonderful GCC 16.2.0 produce a TWL-exclusive ROM
  (unit code 3). The converter reproduces all eleven local data files byte for
  byte.
- USA 1.1 CUE, one MODE2/2352 track, 616,494,480 bytes, SHA-1
  `34278d31d9b9b12b3b5db5e45bcbe548991ecbc7`. Non-XA file entries agreed
  with upstream silentassets. All converted data stays on the owner's machine.
- melonDS 1.1 direct boot uses external BIOS, firmware and private NAND.
  The SDK filesystem sample and port verified TWL mode, and the emulator
  reported 16 MB RAM. Physical console SD and DSpico/Pico-Launcher remain
  untested. The official DSpico DLDI driver built from `8ba45f6` fits the
  ROM's 16 KiB patch area. The lower screen stays black in production.
  [DSpico driver source](https://github.com/LNH-team/dspico-dldi/tree/8ba45f65690bc40d9279e663e1d89ca806451cc1)

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
| PS1 health drink and first-aid kit events | 130 drink and 126 kit buffer-change intervals | Drink had 9 at 33.37 ms and 121 at 50.05 ms. Kit intervals were all 50.05 ms |
| PS1 courtyard entry, walk and return approach | 183 buffer-change intervals | 45 at 33.37 ms, 117 at 50.05 ms and 21 at 66.73 ms. Mean 47.86 ms |
| PS1 northern hallway entry and walk to the return door | 206 buffer-change intervals | 1 at 16.68 ms, 203 at 33.37 ms and 2 at 50.05 ms. Mean 33.45 ms |
| melonDS idle music room with Harry | 300 scene submissions | 16.715 / 33.317 / 33.432 ms min/mean/max, 2.893 ms mean ARM9 submission time |
| melonDS rotating silver pickup with prompt | 300 scene intervals | 16.713 / 33.374 / 33.432 ms min/mean/max, 3.291 ms mean ARM9 submission time |
| melonDS idle corridor after collecting the medallion | 300 scene intervals | 16.713 / 33.317 / 33.432 ms min/mean/max, 4.137 ms mean ARM9 submission time |
| melonDS idle eastern hallway with medallion | 300 scene intervals | 16.717 / 33.373 / 49.966 ms min/mean/max, 5.506 ms mean ARM9 submission time |
| melonDS first-floor stair entry with medallion | 300 scene intervals | 16.717 / 33.374 / 33.438 ms min/mean/max, 5.468 ms mean ARM9 submission time |
| melonDS idle keyboard | 600 display-loop intervals | 16.713 / 16.715 / 16.717 ms min/mean/max |
| melonDS southern hallway with medallion | 300 scene intervals | 16.717 / 33.373 / 49.968 ms min/mean/max, 5.138 ms mean ARM9 submission time |
| melonDS infirmary entrance with medallion | 300 scene intervals | 16.711 / 33.317 / 33.434 ms min/mean/max, 3.920 ms mean ARM9 submission time |
| melonDS infirmary health drink | 300 scene submissions during the pickup event | 16.713 / 33.374 / 33.432 ms min/mean/max, 4.263 ms mean ARM9 submission time |
| melonDS courtyard entry, straight walk and turn | 300 scene intervals | 16.711 / 33.318 / 33.457 ms min/mean/max, 6.911 ms mean ARM9 submission time |
| melonDS northern hallway entry, walk and turn | 300 scene intervals | 16.711 / 33.374 / 33.443 ms min/mean/max, 3.756 ms mean ARM9 submission time |

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
  The separate PS1 `save` replay writes and verifies the first slot on a
  scratch memory card, including its checksum, location 4, flags and item
  count. In this replay the PS1 stored (251381, 576233), yaw 1002.

- The DLDI production build streamed room 9 (1,211 scenery quads and 45
  collision edges). Harry landed at (233472, 587366),
  yaw 2048, and returned to room 13 at (591462, 77004), yaw 0. Both matched
  the PS1 trace. He could reach the eastern desk camera, walk into the counter
  twice to the same stopping X position and back away. A console SD replay
  kept the medallion through both doors. At entry the GPU retained 411
  polygons and 1,589 vertices. Whole-frame RGB error
  was 23.2/255 at entry and 27.0/255 at return. Lighting and camera transitions
  still differ from PS1.
- The notebook, drink and kit add 34 original BG_ITEM.PLM quads, four CLUTs
  and an 8 KiB texture page. The production DLDI build reached the desk
  with a whole-frame RGB error of 30.3/255 against the PS1 desk view.
  A console SD diagnostic hid and restored the items at a fixed camera:
  they changed 98 stationary pixels at the desk and 133 near the cabinet.
  At the entrance none were in view. With items loaded, 300 idle intervals
  measured 16.717 / 33.318 / 33.430 ms, with 3.992 ms mean submission time.

## Notebook checkpoint

- The notebook at point 89 uses the original first-visit text, font and save
  background. All 1,155 bright PS1 text pixels matched after resampling to
  256×192. Slot selection and confirmation use native controls. The port
  writes its own checkpoint format, distinct from a PS1 memory-card file.
- In melonDS 1.1, DLDI loaded a seeded desk position with the medallion.
  The port created slot 2, cancelled and confirmed overwrites, then cold-loaded
  its own slot at (250931, 576228), yaw 1028. The inventory contained item
  98 ×1 and the first-floor latch remained open. Host checks validated both
  generations, all eleven slots and recovery at each of 32 truncated lengths.
- On virtual console SD, the supplied melonDS 1.1 stalled during both the
  SDK sample write and the port’s write. Upstream build `361ffa9` completed
  both. A fresh playthrough walked from the piano, unlocked the latch and
  created a checkpoint. With the emulator restarted, the same ROM returned
  to (250982, 576220), yaw 1028, with the latch open and inventory empty.
  Both modes reported 16 MB; physical storage remains untested.
  [Upstream SD controller fix](https://github.com/melonDS-emu/melonDS/pull/2438)

## Infirmary pickups

- MAP1_S00 points 97 and 98 use the original standing pickup flow, with
  locally converted models, texture and prompts. The native prompts matched
  all 265 bright green drink-name pixels and all 254 kit-name pixels in the
  resampled PS1 captures. The scene camera and item lighting still differ.
- The melonDS 1.1 DLDI run loaded an existing-format slot with the medallion.
  Harry declined and then took each item, left and re-entered the infirmary,
  and wrote a new notebook slot with the supplies, medallion and latch.
  After restarting melonDS, the same build loaded the saved pose and flags.
  The same replay passed on virtual console SD in upstream build `361ffa9`.
  The regenerated sidecar is required for the interactions.
- The silver medallion and medical prompts use BASE.VAB effects 1305–1307
  for changing the selection, declining and accepting. An isolated melonDS
  recording verified three distinct PCM onsets and silence when pressing
  Up on the already selected first option.

## Downstairs collision

- Four matched PS1 and melonDS approaches reproduced walk-through at the
  first-floor double doors, the western wall fixture, the southern hall
  lockers and the infirmary door. The port now keeps the relevant IPD
  edges and blocks walking across the closed doors. All four stops are
  within 1 cm of PS1 along the blocked axis. The first-floor latch and
  both action-button transitions still work.

## Wall decorations

- The converter offsets 30 confirmed coplanar decoration quads by 7.8 mm
  toward the visible side. Texture coordinates, the backing walls and
  runtime geometry counts stay the same.
- A melonDS DSi-mode sweep sampled 13 angles at a first-floor poster.
  In the old data, wall texels covered the entire 18 × 16 sample at three
  angles. In the updated data, all 13 samples showed the poster with zero
  wall-only texels in that region.

## Infirmary camera

- The infirmary now applies the PS1 vertical framing limit that keeps
  Harry in view. With matching player poses in PS1 and melonDS, whole-frame
  mean RGB error fell from 46.2 to 32.2/255 by the drink and from 32.7 to
  27.5/255 by the kit. Lighting and camera transitions still differ.

## Courtyard

- MAP1_S00 points 5 and 3 connect the southern hallway to room 23. The
  room streams SCFFFE with 2,648 scenery quads, 88 collision edges, 14
  ramp and landing planes, and three paletted texture pages occupying
  80 KiB. The conversion retains the tree and window cutouts.
- The original NTSC-U replay measured the entry at X/Z (−80691, −280576),
  yaw 0, and the return at (573440, 93389), yaw 2048, in Q12 coordinates.
  The production ROM matched both poses in melonDS 1.1 over DLDI and
  `361ffa9` over virtual console SD. Both replays kept the medallion,
  medical supplies and first-floor latch. A separate probe verified the
  0.60 m entry landing and ground height after the ramp.
- A diagnostic pass reading geometry RAM before each swap peaked at 1,025
  polygons and 4,071 vertices with no overflow during the entry, walk and
  turn. That pass included one 49.97 ms interval.
- The PS1 trace had fog disabled in this scene. Low-light shading and
  camera following still differ. In the DLDI replay, whole-frame mean RGB
  error was 20.6/255 on entry, 17.1/255 after descending the ramp and
  13.9/255 after returning to the hall.

## North hallway

- MAP1_S00 points 4 and 6 connect the courtyard to first-floor room 16.
  SC0304 streams 1,143 quads, 24 IPD collision edges and 80 KiB of
  textures. The transition uses the original double-door sound pair.
- In melonDS DSi mode, both DLDI and virtual console SD crossed the
  courtyard, entered at Q12 X/Z (546816, 772914), yaw 0, and returned at
  (−80691, −216678), yaw 2048. Both poses matched the original and kept
  the medallion, supplies and latch. Mean whole-frame RGB error was
  11.9/255 on entry, 17.8/255 while walking and 19.9/255 on return.
- The PS1 trace had fog off in the hall. The port still differs in
  lighting and chase camera motion. The other hallway doors are pending.

## Clock tower

- MAP1_S00 points 13–15 show the locked door, the stopped clock and both
  empty medallion slots. The port loads the original messages and TIM art
  from the locally converted sidecar. The views use the PS1 texture sample
  positions and 5-bit dither at native 256×192.
- The PS1 lock message drew at 50.05 ms intervals. The three close-ups
  drew at 16.68 ms intervals when fully visible. The PS1 inspection replays
  returned to the courtyard with the medallion and latch intact.
- DSi-mode replays in melonDS 1.1 over DLDI and `361ffa9` over virtual
  console SD reached all three points from the infirmary, dismissed each
  view and kept the inventory, latch and player pose. All 830 sampled
  bright text pixels matched the PS1 at 5-bit precision. In the top 152
  rows, 99.5–99.7% of close-up pixels were within one 5-bit step per channel.
- The two plaques now sit 7.8 mm in front of their coplanar masonry. In a
  16-angle melonDS DSi-mode sweep, the lowest counts of plaque-colored
  pixels in the 112-pixel test regions rose from 1 to 87 for the moon and
  from 0 to 112 for the sun.
- The original tower planes raise Harry along the ramp to the 0.60 m
  landing. The tower perimeter stops further forward movement. The old
  sidecar kept Harry at ground level and allowed him through the tower.
  The new sidecar ascended, stopped at the door and descended back to
  ground in melonDS. A host check at 81 PS1 replay positions matched 73
  heights within 4 mm. The largest difference was 3.1 cm.
- The medallion-use events and tower interior are pending. The current
  build starts directly at the piano and allows inspection of the tower
  with both slots empty.

## Next

1. Trace the chemistry lab entrance and the event that releases the gold medallion.
2. Port medallion placement and its clock tower state changes.
3. Measure cold boot and checkpoint I/O on physical console SD and DSpico.

Based on the original GPL-3.0 decompilation, Copyright (C) 2026
shdecompilations. [Pinned upstream source](https://github.com/shdecompilations/silent-hill-decomp/tree/ecbb0d8e3881cf2f0cb9fa700f3bb1a74b6d34ce)
