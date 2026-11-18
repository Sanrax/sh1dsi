"""Convert Harry's unarmed HERO.ILM meshes and the original idle animation."""
from collections import defaultdict
import struct


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
                corners = (0, 1, 2) if face[15] == 255 else (0, 1, 3, 2)
                palette = int.from_bytes(face[2:4], "little") >> 6
                if face[7] or palette >= 15:
                    raise ValueError("unsupported player material")
                for k in corners:
                    uv = (0, 4, 8, 10)[k]
                    groups[palette,len(corners)].extend(struct.pack("<HBB", slots[face[12+k]], *face[uv:uv+2]))

    # Anim_BonesUpdate and HarryAnim_Idle. Retain the native signed matrix
    # samples and their hierarchy so interpolation stays on the game clock.
    anm = disc.read("ANIM/HB_BASE.ANM")
    offset, rot, trans, stride, bones, _, _, _, frames, scale, root_y = struct.unpack_from("<HBBHBBIIHBB", anm)
    if (rot, trans, stride, bones, scale, root_y) != (17, 1, 156, 18, 2, 255) or frames <= 542:
        raise ValueError("unexpected HB_BASE animation")
    data = bytearray(struct.pack("<HH", len(vertices), len(groups)))
    data.extend(anm[20:20+bones*6])
    clips = ((503, 542), (180, 192), (193, 205))
    data.extend(struct.pack("<H", len(clips)))
    for start, end in clips:
        data.extend(anm[offset+start*stride:offset+(end+1)*stride])
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
