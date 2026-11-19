"""Build Midwich room streams from the original IPD instances and embedded LM."""
from collections import defaultdict
import math
import struct

from harry import player_data
from pickup import prompt_data, silver_pickup


def collision_edges(ipd, room):
    collision = memoryview(ipd)[0x54:]
    x, z, vertices, surfaces, count, cylinders, vert, surface, polygon = struct.unpack_from("<ii4B3I", collision)
    expected = {39: (-30720, 0, 51, 8, 51, 0), 32: (-10240, 0, 60, 10, 60, 0),
                31: (-20480, 10240, 69, 15, 71, 0), 12: (20480, 10240, 80, 21, 85, 0),
                13: (30720, 0, 80, 12, 80, 0)}[room]
    if (x, z, vertices, surfaces, count, cylinders) != expected:
        raise ValueError("unexpected school collision header")
    output = bytearray()
    for i in range(count):
        tx, tz, length, a, b, floor, other = struct.unpack_from("<HHh4B", collision, polygon+10*i)
        if max(a, b) >= vertices or floor >= surfaces or length <= 0:
            raise ValueError("invalid school collision split")
        # Room 32's level path intersects the floor boundary, benches and
        # counter. The other surfaces are overhead or on the floor below.
        if room == 32 and floor not in (1, 2, 3, 4, 6):
            continue
        # SCFE01 includes the connected stair ramp and landing. Keep their
        # outside walls and leave the two shared walkable edges open.
        if room == 31 and (floor in (1, 6, 10, 14) or other != 255):
            continue
        # The upper and basement stair flights share this first-floor chunk.
        if room == 12 and (floor not in (3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 16, 18, 20) or other != 255):
            continue
        if room == 13 and floor in (3, 4, 10):
            continue
        if other != 255:
            raise ValueError("unexpected shared collision edge")
        ax, _, az = struct.unpack_from("<3h", collision, vert+6*a)
        bx, _, bz = struct.unpack_from("<3h", collision, vert+6*b)
        # The top two bits of each tangent component are trigger flags.
        slope = struct.unpack_from("<2h", collision, surface+12*floor+8)
        expected_slope = {(31, 11): (2731, 0), (12, 7): (2778, 5), (12, 9): (-2753, 0)}.get((room, floor), (0, 0))
        if (tx | tz) & 0xc000 or slope != expected_slope:
            raise ValueError("unsupported school collision flags or slope")
        tx = ((tx ^ 8192) - 8192)
        tz = ((tz ^ 8192) - 8192)
        scale = 16 if room == 39 else 1
        output.extend(struct.pack("<6h", (ax-5120)*scale, (az-5120)*scale,
                                  (bx-5120)*scale, (bz-5120)*scale, tx, tz))
    return output


def school_room(disc, room=39):
    cx, cz = {39: (253, 0), 32: (255, 0), 31: (254, 1), 12: (2, 1), 13: (3, 0)}[room]
    data = disc.read(f"BG/SC{cx:02X}{cz:02X}.IPD")
    if data[:4] != bytes((20, 0, cx, cz)):
        raise ValueError("unexpected school chunk coordinates")
    lm, = struct.unpack_from("<I", data, 4)
    mat_count = data[lm + 3]
    mat_ptr, count, model_ptr = struct.unpack_from("<III", data, lm + 4)
    models = {}
    for i in range(count):
        name, meshes, base, _, _, ptr = struct.unpack_from("<8s4BI", data, lm + model_ptr + 16*i)
        models[name.rstrip(b"\0")] = meshes, base, lm + ptr

    pages = bytearray()
    palettes = bytearray()
    page_info = []
    color_keys = []
    for i in range(mat_count):
        material = lm + mat_ptr + i*24
        name = data[material:material+8].rstrip(b"\0").decode("ascii")
        tim = disc.read(f"BG/{name}.TIM")
        magic, flags, length, _, _, colors, rows = struct.unpack_from("<III4H", tim)
        image = 8 + length
        size, _, _, words, height = struct.unpack_from("<I4H", tim, image)
        if (magic, flags, colors, height) != (16, 8, 16, 256) or words not in (32, 64):
            raise ValueError(f"unexpected texture layout for {name}")
        page_info.append((len(pages), words*4, len(palettes)//32, rows))
        cluts = list(struct.iter_unpack("<16H", tim[20:20 + rows*32]))
        # A PS1 transparent texel is a zero palette word. DS paletted textures
        # support a transparent index of zero only. Permute the page once.
        indices = {idx for clut in cluts for idx, color in enumerate(clut) if color == 0}
        if len(indices) > 1:
            raise ValueError(f"incompatible transparent indices on {name}")
        key = next(iter(indices), 0)
        color_keys.append([clut[key] == 0 for clut in cluts])
        pages.extend(byte ^ (key | key << 4) for byte in tim[image+12:image+size])
        for clut in cluts:
            palettes.extend(struct.pack("<16H", *(clut[idx ^ key] for idx in range(16))))

    # Group opaque polygons by texture and CLUT without duplicating any texture.
    groups = defaultdict(bytearray)
    vertex_scale = 4 if room in (12, 31) else 8
    def add_model(name, matrix, position, state=0):
        meshes, base, ptr = models[name]
        for j in range(meshes):
            faces, nverts, _, _, prim, xy, z, _, _ = struct.unpack_from("<4B5I", data, ptr + 24*j)
            vertices = []
            for v in range(nverts):
                coord = (*struct.unpack_from("<hh", data, lm+xy+4*v),
                         *struct.unpack_from("<h", data, lm+z+2*v))
                transformed = [position[a] + (sum(matrix[3*a+k]*coord[k] for k in range(3)) >> 12)
                               for a in range(3)]
                x, y, zpos = transformed
                # Room 31 is longer than the V16 range at two metres per unit.
                vertices.append(((x-5120)*vertex_scale, -y*vertex_scale, -(zpos-5120)*vertex_scale))
            for k in range(faces):
                face = data[lm+prim+20*k:lm+prim+20*(k+1)]
                mat = face[7] & 127
                row = int.from_bytes(face[2:4], "little") >> 6
                if mat >= mat_count or row >= page_info[mat][3]:
                    raise ValueError("invalid material or CLUT")
                flags = (face[7] >> 7) | (color_keys[mat][row] << 1)
                group = groups[state, mat, row, flags]
                for corner in (0, 1, 3, 2):  # PS1 quad strip to DS perimeter
                    idx = face[12+corner] - base
                    if not 0 <= idx < nverts:
                        raise ValueError("vertex outside LM mesh")
                    uv = (0, 4, 8, 10)[corner]
                    group.extend(struct.pack("<3hBB", *vertices[idx], *face[uv:uv+2]))

    infos, buffers = struct.unpack_from("<II", data, 20)
    for i in range(data[9]):
        buf = buffers + 24*i
        instances, = struct.unpack_from("<I", data, buf+12)
        for j in range(data[buf]):
            model, *pose = struct.unpack_from("<I9h2x3i", data, instances+36*j)
            info = infos + 16*model
            if data[info]:
                raise ValueError("music room references an external PLM")
            add_model(bytes(data[info+4:info+12]).rstrip(b"\0"), pose[:9], pose[9:])
    if room != 39:
        magic = {12: b"SHF2", 13: b"SHS1", 31: b"SHE1", 32: b"SHC1"}[room]
        extra = prompt_data(disc, latch=True) if room == 12 else b""
    else:
        magic = b"SHR5"
        # Map_WorldObjectsInit and Map_WorldObjectsUpdate, EventFlag_71 set.
        def local(x,y,z):
            return round((x+120)*256), round(y*256), round(z*256)
        identity = (4096,0,0,0,4096,0,0,0,4096)
        add_model(b"FUTA1_HI", identity, local(-98.2798,-0.58,19.93))
        add_model(b"SILVER_H", identity, local(-99,-2.68,23.73), 1)
        angle = math.radians(28.5)
        c, s = round(math.cos(angle)*4096), round(math.sin(angle)*4096)
        add_model(b"SILVER_H", (c,-s,0,0,0,4096,-s,-c,0), local(-98.8,0,22.8), 2)

        player, skin, cluts = player_data(disc)
        page_info.append((len(pages), 256, len(palettes)//32, len(cluts)//32))
        pages.extend(skin)
        palettes.extend(cluts)
        pickup, texture, clut = silver_pickup(disc)
        pages.extend(texture)
        palettes.extend(clut)
        extra = player + pickup
    result = bytearray(struct.pack("<4sHHII", magic, len(page_info), len(groups), len(pages), len(palettes)))
    for page in page_info:
        result.extend(struct.pack("<IHHI", *page))
    result.extend(pages)
    result.extend(palettes)
    result.extend(collision_edges(data, room))
    result.extend(extra)
    for key, quads in sorted(groups.items(), key=lambda g: (g[0][3] & 1, g[0])):
        result.extend(struct.pack("<4BI", *key, len(quads)//32))
        result.extend(quads)
    return result
