"""Build the Midwich music room from SCFD00's IPD instances and embedded LM."""
from collections import defaultdict
import math
import struct


def music_room(disc):
    data = disc.read("BG/SCFD00.IPD")
    if data[:4] != bytes((20, 0, 253, 0)):
        raise ValueError("expected the school chunk at (-3, 0)")
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
                # Q8 chunk space to DS V16, centered at (-100, 0, 20).
                vertices.append(((x-5120)*8, -y*8, -(zpos-5120)*8))
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
    # Map_WorldObjectsInit and Map_WorldObjectsUpdate, EventFlag_71 set.
    def local(x,y,z):
        return round((x+120)*256), round(y*256), round(z*256)
    identity = (4096,0,0,0,4096,0,0,0,4096)
    add_model(b"FUTA1_HI", identity, local(-98.2798,-0.58,19.93))
    add_model(b"SILVER_H", identity, local(-99,-2.68,23.73), 1)
    angle = math.radians(28.5)
    c, s = round(math.cos(angle)*4096), round(math.sin(angle)*4096)
    add_model(b"SILVER_H", (c,-s,0,0,0,4096,-s,-c,0), local(-98.8,0,22.8), 2)

    result = bytearray(struct.pack("<4sHHII", b"SHR1", mat_count, len(groups), len(pages), len(palettes)))
    for page in page_info:
        result.extend(struct.pack("<IHHI", *page))
    result.extend(pages)
    result.extend(palettes)
    for key, quads in sorted(groups.items(), key=lambda g: (g[0][3] & 1, g[0])):
        result.extend(struct.pack("<4BI", *key, len(quads)//32))
        result.extend(quads)
    return result
