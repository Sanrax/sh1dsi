"""Convert Harry's unarmed HERO.ILM meshes and the original idle animation."""
from collections import defaultdict
import struct


def pour_frames(disc):
    # Player state 83 uses MAP1_S01 clip 51 at 15 frames per second.
    source = disc.read("ANIM/HB_M1S01.ANM")
    frames = source[(900-676)*156:(949-676)*156]
    if len(frames) != 49*156:
        raise ValueError("incomplete Harry pour animation")
    return frames


def ladder_frames(disc):
    # MAP1_S00 states 88 and 87 use clips 50 and 49, respectively.
    source = disc.read("ANIM/HB_M1S00.ANM")
    frames = b"".join(source[(start-676)*156:(start+37-676)*156] for start in (910, 873))
    if len(frames) != 2*37*156:
        raise ValueError("incomplete Harry ladder animation")
    return frames


def courtyard_look_frames(disc):
    # MAP1_S02 state 74 plays clip 53 at 8.9 frames per second.
    source = disc.read("ANIM/HB_M1S02.ANM")
    frames = source[(970-676)*156:(1017-676)*156]
    if len(frames) != 47*156:
        raise ValueError("incomplete courtyard look animation")
    return frames


def card_slot_frames(disc):
    # MAP1_S02 event 11 plays keyframes 1017–1037, then reverses their tail.
    source = disc.read("ANIM/HB_M1S02.ANM")
    frames = source[(1017-676)*156:(1038-676)*156]
    if len(frames) != 21*156:
        raise ValueError("incomplete Picture Card animation")
    return frames


def player_data(disc):
    model = disc.read("CHARA/HERO.ILM")
    _, models, headers, order = struct.unpack_from("<4I", model, 4)
    if model[:4] != bytes((48, 6, 0, 1)) or models != 23:
        raise ValueError("unexpected Harry model")
    # The PS1 transforms into a shared vertex workspace in modelOrder.
    # Resolve each face before the next mesh overwrites shared vertex slots.
    slots = {}
    vertices = []
    groups = defaultdict(bytearray)
    for index in model[order:order+models]:
        name, meshes, base, _, _, mesh = struct.unpack_from("<8s4BI", model, headers+16*index)
        if name.rstrip(b"\0") in (b"06LHAND2", b"10RHAND2", b"10RHAND3", b"10RHAND4"):
            continue
        bone = int(name[:2])
        for j in range(meshes):
            count, nverts, _, _, prim, xy, z, _, _ = struct.unpack_from("<4B5I", model, mesh+24*j)
            for v in range(nverts):
                slots[base+v] = len(vertices)
                vertices.append((*struct.unpack_from("<hh", model, xy+4*v),
                                 *struct.unpack_from("<h", model, z+2*v), bone))
            for f in range(count):
                face = model[prim+20*f:prim+20*f+20]
                palette = int.from_bytes(face[2:4], "little") >> 6
                if face[7] or palette >= 15:
                    raise ValueError("unsupported player material")
                sides = 3 if face[15] == 255 else 4
                indices = [slots[i] for i in face[12:12+sides]]
                if sides == 3:
                    polygons = ((0, 1, 2),)
                elif len({vertices[i][3] for i in indices}) > 1:
                    # Quads spanning joints can fold as the bones move.
                    # Cull each half of the PS1 strip independently.
                    polygons = ((0, 1, 2), (1, 3, 2))
                else:
                    polygons = ((0, 1, 3, 2),)
                for corners in polygons:
                    for k in corners:
                        uv = (0, 4, 8, 10)[k]
                        groups[palette,len(corners)].extend(struct.pack("<HBB", indices[k], *face[uv:uv+2]))

    # Anim_BonesUpdate and HarryAnim_Idle. Retain the native signed matrix
    # samples and their hierarchy so interpolation stays on the game clock.
    anm = disc.read("ANIM/HB_BASE.ANM")
    offset, rot, trans, stride, bones, _, _, _, frames, scale, root_y = struct.unpack_from("<HBBHBBIIHBB", anm)
    if (rot, trans, stride, bones, scale, root_y) != (17, 1, 156, 18, 2, 255) or frames <= 542:
        raise ValueError("unexpected HB_BASE animation")
    data = bytearray(struct.pack("<HH", len(vertices), len(groups)))
    data.extend(anm[20:20+bones*6])
    clips = ((503, 542), (180, 192), (193, 205), (1, 23), (46, 69))
    data.extend(struct.pack("<H", len(clips)+2))
    for start, end in clips:
        data.extend(anm[offset+start*stride:offset+(end+1)*stride])
    # FS_BUFFER_4 starts at keyframe 676 in Harry's shared animation buffer.
    map_anim = disc.read("ANIM/HB_M1S01.ANM")
    data.extend(map_anim[(873-676)*stride:(900-676)*stride])
    for v in vertices:
        data.extend(struct.pack("<3hH", *v))
    for (palette, corners), stream in sorted(groups.items()):
        data.extend(struct.pack("<BBH", palette, corners, len(stream)//4))
        data.extend(stream)

    tim = disc.read("CHARA/HERO.TIM")
    magic, flags, pal_len, _, _, colors, rows = struct.unpack_from("<III4H", tim)
    image = 8+pal_len
    length, _, _, width, height = struct.unpack_from("<I4H", tim, image)
    if (magic, flags, colors, rows, width, height, length) != (16, 8, 16, 15, 64, 192, 24588):
        raise ValueError("unexpected HERO texture")
    pixels = tim[image+12:image+length] + bytes(128*64)
    return data, pixels, tim[20:image]
