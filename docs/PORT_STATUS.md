# Port status

Harry starts at the Midwich piano. He can solve the keyboard puzzle, watch
or skip the falling silver medallion, decline or accept it, walk through the
eastern hallway, descend the stairs, unlatch the first-floor door and enter
the southern hallway. The courtyard connects the southern and northern halls.
Harry can inspect the clock tower door and both medallion slots.
Upstairs, the east corridor connects through the north hall to the chemistry lab
and equipment room. Harry can collect the Chemical, open the hand statue
and take the gold medallion.
In the infirmary he can pick up both medical supplies and use the notebook.
A cold boot can resume at the desk with the puzzle,
door latch and pickups intact. The full game remains unplayable.

## Build and media

- BlocksDS 1.24.0 and Wonderful GCC 16.2.0 produce a TWL-exclusive ROM
  (unit code 3). A forced rebuild and repeat conversion reproduced the ROM
  and the local data files byte for byte.
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

PS1 references use Beetle PSX software rendering, USA 1.1, a digital pad
and the 59.94 Hz clock. The street replay starts a new game.
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
| PS1 upstairs north hall on the way to the lab | 392 buffer-change intervals | 372 at 33.37 ms and 20 at 50.05 ms. Mean 34.22 ms |
| PS1 chemistry lab entry and turn | 101 buffer-change intervals | All 33.37 ms |
| PS1 equipment room entry and return approach | 87 buffer-change intervals | 60 at 33.37 ms and 27 at 50.05 ms. Mean 38.54 ms |
| PS1 Chemical pickup | 126 buffer-change intervals across two prompts | All 50.05 ms |
| PS1 hand statue pour and dissolve | 235 buffer-change intervals | 102 at 33.37 ms and 133 at 50.05 ms |
| PS1 gold pickup | 127 buffer-change intervals across two prompts | 4 at 33.37 ms and 123 at 50.05 ms |
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
| melonDS upstairs north hall entry, walk and turn | 300 scene intervals | 16.719 / 33.374 / 33.443 ms min/mean/max, 3.869 ms mean ARM9 submission time |
| melonDS chemistry lab from entry to the east aisle | 300 scene intervals | 16.713 / 33.374 / 33.447 ms min/mean/max, 3.702 ms mean ARM9 submission time |
| melonDS equipment room from entry to the west aisle | 300 scene intervals | 16.719 / 33.374 / 33.445 ms min/mean/max, 4.413 ms mean ARM9 submission time |
| melonDS rotating Chemical with prompt | 300 scene intervals | 16.713 / 33.318 / 33.432 ms min/mean/max, 4.748 ms mean ARM9 submission time |
| melonDS statue close camera | 271 scene intervals | 16.711 / 33.368 / 34.249 ms min/mean/max, 4.014 ms mean ARM9 submission time |

## Room geometry submission

- The loader packs the room vertices into geometry FIFO commands and
  flushes the data cache once. DMA submits each visible batch in its
  original order. The existing local sidecars work with this build.
- In melonDS DSi mode, each case sampled 128 scene intervals. Mean
  intervals stayed at 33.43 ms in the music room and courtyard and
  33.30 ms at the infirmary desk.

| Scene | Previous mean submission | Packed mean submission |
| --- | --- | --- |
| Music room | 2.94 ms | 2.02 ms |
| Moving medallion | 2.94 ms | 2.00 ms |
| Courtyard entrance | 6.98 ms | 3.74 ms |
| Courtyard below the ramp | 7.00 ms | 3.66 ms |
| Infirmary desk | 3.99 ms | 2.43 ms |

- All 49,152 pixels matched in each of six comparisons covering the music
  room, moving medallion, two courtyard views and both infirmary item
  states. Collecting the infirmary items changed 541 pixels in both builds
  at the fixed diagnostic view.
- The courtyard geometry buffer grew from 82.75 to 144.81 KiB, and its
  emulator room-load sample rose from 45.80 to 47.98 ms. The music-room
  buffer grew from 24.69 to 43.20 KiB. Real hardware timing is pending.
- The production ROM streamed the music room and its corridor in both
  directions over DLDI. The virtual console SD replay crossed from the
  infirmary through the southern hall to the courtyard, turned and
  returned through the southern door. Both retained their expected
  spawn poses, inventory state and black lower screen.

### Compact vertex commands

- Each batch starts with a full vertex and uses two-coordinate commands
  where an axis matches the preceding vertex. The host check decoded
  all 67,096 vertices and UVs in the eleven room streams exactly.
  [BlocksDS vertex commands](https://blocksds.skylyrac.net/libnds/videoGL_8h.html)
- Six matched melonDS frames were pixel-identical, including the moving
  medallion and both infirmary item states. Decoding the DMA buffers
  from those runs also matched every vertex, UV and batch boundary.
- The courtyard allocation fell from 144.82 to 108.80 KiB and the music
  room from 43.21 to 32.76 KiB. Mean courtyard submission below the ramp
  fell from 3.62 to 3.35 ms over 128 intervals. Scene intervals averaged
  33.30–33.43 ms. The courtyard's sampled load rose from 48.70 to 52.12 ms.
- The production build also passed the music-room corridor round trip over
  DLDI and the courtyard round trip over virtual console SD.

## Animated character submission

- The actor draw uses ARM instructions for the fixed-point math and packs
  each transformed position once for the indexed corners. The packed
  position buffer adds 768 bytes. [GCC ARM function attributes](https://gcc.gnu.org/onlinedocs/gcc-11.5.0/gcc/ARM-Function-Attributes.html)
- Over 64 draws of each of ten poses in melonDS DSi mode, mean actor draw
  time fell from 1.233 to 1.073 ms. Bone interpolation fell from 0.112 to
  0.096 ms and vertex transforms from 0.350 to 0.196 ms. Geometry submission
  rose from 0.771 to 0.780 ms. Scene intervals averaged about 33.4 ms.
- The ten held poses matched every bone matrix, transformed vertex and
  top-screen pixel. The host check matched all 332 positions in each of
  8,316 frame and blend samples across the eight clips.
- The production DLDI run solved the piano and took the silver medallion
  through the kneeling inspection. A separate DLDI run passed the corridor
  round trip. The virtual console SD run walked and turned in the courtyard
  and returned to the southern hall.

### Actor geometry DMA

- Each palette batch sends pairs of textured vertices through the geometry
  FIFO. One scratch buffer serves all batches and uses 4,480 bytes for
  the current model. Odd batches send their last vertex directly.
- In melonDS DSi mode, 64 draws at each of ten fixed poses across three
  rooms reduced mean actor draw time from 1.038 to 0.983 ms. Submission,
  including packing and cache maintenance, fell from 0.741 to 0.695 ms.
  Mean scene intervals were 33.35 ms in both builds. All ten 256×192
  frames matched pixel for pixel. Real hardware timing remains pending.
- The production build passed the music-room corridor round trip over
  DLDI and the courtyard round trip over virtual console SD.

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

- All 28 first-floor poster faces now receive 62.5 mm of clearance. The
  converter checks sloped wall panels and posters authored 3.9 mm off the
  wall. In 126 matched melonDS DSi views, removing the backing panels
  exposed 22,952 marked pixels with the previous data and nine isolated
  edge pixels out of 462,895 with the new data. The ROM, collision,
  textures and other sidecars were byte-identical after regeneration.
- Production replays over DLDI and virtual console SD entered from the
  southern hall, climbed the stairs and returned at the expected landing pose.

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

## Chase camera clearance

- The original chase roads reserve one metre at each edge. The port now
  applies that inset to its chase-camera targets and uses the centre line
  if the road is under two metres wide. Door and fixed views retain their
  own positions.
- At the southern hall test pose, the old DSi camera settled 22 cm behind
  the wall. The updated camera settled 78 cm inside and the PS1 camera was
  1.43 m inside. The eye still approaches from a different angle. Whole-frame
  RGB error was 21.0/255 before the fix and 22.6/255 with the wall clearance.
- The final ROM kept the camera inside at four headings beside that wall
  on virtual console SD. The north round trip passed again over DLDI.
- The first-floor chase now uses the PS1's heading-dependent clearance to
  keep the camera away from Harry while it stays within the authored road.
  At Q12 X/Z (444000, 250000), facing east, its horizontal distance rose
  from 0.67 m to 2.49 m. The same PS1 pose measured 2.38 m. The native
  look target went from 3.62 m below the floor to 0.69 m above it, matching
  the PS1 target height. Mean whole-frame RGB error fell from 28.1 to
  26.6/255. Camera motion during turns still differs.
- The production DLDI replay walked into the first-floor hall, turned and
  crossed the stairs in both directions with the original warp poses.
  A shorter virtual console SD replay also passed both stair transitions.
- The southern hall now uses the same clearance along its length and
  the authored 1.69 m chase height. Across eight matched views, horizontal
  camera distance rose from 0.39–0.91 m to 2.08–2.77 m. The locker pose
  moved from 0.43 m to 2.77 m against 2.95 m on PS1. All eight native
  look targets were above the floor. Whole-frame RGB error at the locker
  rose from 22.51 to 22.99/255, while the centre pose improved from 44.20
  to 31.95/255. Both first-floor camera poses matched the previous build.
- At the settled locker turn endpoint, 64 melonDS scene intervals
  averaged 33.43 ms before and after the change. ARM9 submission averaged
  5.18 ms before and 5.17 ms after. The original NTSC-U idle locker trace
  had 33 intervals at 33.37 ms and 16 at 50.05 ms.
- The production routes entered from the infirmary, turned in the hall
  and crossed through the courtyard doors in both directions over DLDI
  and virtual console SD. Both routes preserved the inventory and latch.
- The music-room corridor, eastern hallway, both north halls and eastern
  equipment aisle now apply the same clearance along each road's long
  axis. Across ten matched views, horizontal distance rose from
  0.39–0.86 m to 2.37–2.69 m. The PS1 measured 2.26–3.35 m. All ten
  native look targets were above the floor after the change. Four of the
  five initial views moved closer to the PS1 in whole-frame RGB error.
  The eastern hallway rose from 23.78 to 24.15/255.
- Across 320 melonDS scene intervals after the turns, the mean was
  33.33 ms before and after. Mean ARM9 submission was 4.23 ms in both
  builds. The original samples mostly used 33.37 ms intervals and
  included 50.05 ms intervals in the eastern hallway and equipment aisle.
- The production DLDI run walked from the music room to the equipment
  aisle and returned to the upstairs north hall. The virtual console SD
  run crossed the courtyard, turned in the downstairs north hall and
  returned through the same door. Both verified the original spawn poses
  and a black lower screen.

## Chemistry lab route

- The PS1 replay used upstairs east corridor point 52 to enter room 33 at
  Q12 X/Z (−62668, 784384), yaw 3072. It crossed to point 45 and entered
  chemistry lab room 34 at (−261324, 398130), yaw 0. The return through
  points 49 and 42 spawned at (−118374, 789299), yaw 2048, then
  (−208896, 292864), yaw 1024.
- The production ROM walked this route from the piano and matched all four
  PS1 door poses in melonDS DSi mode with DLDI and virtual console SD.
  Mean entry and return RGB errors were 19.1, 12.8, 19.8 and 15.3 out of
  255. The PS1 replay had fog off in both rooms. Lighting still differs.
- SCFF04 streams 1,166 quads and 32 collision edges. SCFE02 streams 1,083
  quads, 30 edges and the 15 original obstacle cylinders. Each room uses
  64 KiB of textures. The lab uses its two original camera positions, and
  the melonDS replay verified the switch when crossing to the east aisle.
- At the west bench, melonDS stopped 7 mm from the PS1 stop. A host check
  backed away within the narrow aisle and reached the same contact again.
- The lab event requires the Chemical item before it releases the gold
  medallion. The intact hand adds 83 original SCFE02 quads. The production
  DLDI replay walked around the benches, opened the hand close-up with A,
  then dismissed it with B. Mean RGB error was 1.18/255 and all 271 sampled
  bright text pixels matched the PS1 close-up. The lower screen stayed black.

## Music-room posters

- Conversion gives 35 decoration faces 46.9 mm of wall clearance. It
  moves their vertices along the wall normal and preserves the textures,
  UVs, batch counts and file size.
- Across 144 matched camera positions in melonDS DSi mode, the previous
  7.8 mm clearance let the walls cover 8,025 marked poster pixels. The
  new clearance matched all 133,538 visible poster pixels in the
  comparison with the backing walls hidden. The ROM and all other data
  files were byte-identical on regeneration.
- The production ROM booted with the regenerated data and passed the
  music-room corridor round trip over DLDI in melonDS 1.1 and virtual
  console SD in `361ffa9`. The lower screen stayed black.

## Equipment room

- MAP1_S01 points 46 and 50 connect the upstairs hall to room 35. SCFD02
  supplies 1,424 quads, 32 collision edges and three paletted texture pages
  occupying 80 KiB. The west aisle uses two measured PS1 views with an
  approximate switch point. The original replay had fog disabled.
- The production ROM reached the Chemical shelf and returned to the hall
  in melonDS DSi mode on both DLDI and virtual console SD. Both door poses
  matched PS1 at Q12 X/Z (−403865, 398130), yaw 0, and (−131481, 789299),
  yaw 2048. Whole-frame mean RGB error was 19.0/255 on entry, 16.2/255 at
  the shelf in the SD replay and 18.5/255 on return. The lower screen stayed
  black.
- The PS1 Chemical event declined with flag 68 clear and item 96 absent,
  then accepted with the flag set and one item added. The model reached
  full scale after 48–49 VBlanks. Examining the empty shelf opened MAP1_S01
  message 19 and kept the item count at one.
- The native pickup uses the original point 13 facing segment, the 55-quad
  shelf prop and 188-triangle rotating item. The prompt streams a 2 KiB
  texture tile with one CLUT. In melonDS over DLDI and virtual console SD,
  Harry declined, cancelled, accepted before solving the piano, and found
  the shelf still empty after leaving and re-entering the room. All 182
  bright green pixels in the item name and 184 bright pixels in the empty
  shelf message matched the resampled PS1 captures. Lighting still differs.
- A diagnostic shortcut reached the infirmary notebook after pickup on both
  storage paths. The production ROM cold-loaded each written checkpoint
  with the Chemical, silver medallion, latch and desk pose intact. The host
  checkpoint recovery fixture also passed with the new flag.
- The PS1 statue callbacks preserve the Chemical after use and offer the
  gold medallion once the hand opens. The reference supplies the item
  selection directly to the event router. Initial lab traces used the
  core's embedded OpenBIOS because the expected US firmware path was
  absent. The statue replay was repeated with the owner's SCPH-7001 via
  a local firmware alias. The inspection and pour images were identical.
  [Beetle PSX firmware selection](https://github.com/libretro/beetle-psx-libretro/blob/master/libretro.c)

## Hand statue and gold medallion

- Select uses the Chemical when Harry faces the hand. This is a direct
  item-use control until the original inventory is ported. The event uses
  the original pour clip and fixed camera, the original smoke tiles with
  DS alpha blending, the ambient hiss loop and the two original hand models.
- The production DLDI replay walked from the piano, collected the Chemical,
  played the full pour, declined and accepted the gold medallion and
  returned to the empty hand after reloading the lab. The pour warped
  Harry to the PS1 Q12 position (−242483, 405094), yaw 0. All 286 bright
  green pixels in the gold medallion name matched the resampled PS1 capture.
- The native full sequence reached the end at 610 simulation ticks. The
  PS1 runs reached it at 609–610 VBlanks. Mean whole-frame RGB errors were
  20.0/255 during the pour, 20.4/255 during dissolution and 18.0/255 at the
  open hand. Lighting and smoke blending differ. A probe peaked at 400
  polygons and 1,541 vertices. An isolated melonDS audio recording captured
  the hiss during the smoke with silence after the sound stopped.
- Over virtual console SD, a lab fixture loaded a legacy checkpoint,
  completed the full event, and saved the open hand and both items in the
  notebook. The production ROM cold-loaded the new SHC2 slot. Start also
  finished the event and stopped the sound during the active hiss. The host
  fixture kept the older SHC1 slot readable across all 36 partial writes.

The original medallion placement replays filled each slot and retained both
items. The boiler remained off. The clock tower door requires the separate
boiler switch event.

## Next

1. Port both medallion placement events and the filled clock views.
2. Connect the boiler room and verify the switch that unlocks the clock tower.
3. Measure cold boot and checkpoint I/O on physical console SD and DSpico.

Based on the original GPL-3.0 decompilation, Copyright (C) 2026
shdecompilations. [Pinned upstream source](https://github.com/shdecompilations/silent-hill-decomp/tree/ecbb0d8e3881cf2f0cb9fa700f3bb1a74b6d34ce)
