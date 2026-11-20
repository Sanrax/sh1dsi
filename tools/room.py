"""Build Midwich room streams from the original IPD instances and embedded LM."""
from collections import defaultdict
import math
import struct

from harry import player_data
from pickup import medical_pickups, prompt_data, silver_pickup


def collision_edges(ipd, room):
    collision = memoryview(ipd)[0x54:]
    x, z, vertices, surfaces, count, cylinders, vert, surface, polygon = struct.unpack_from("<ii4B3I", collision)
    expected = {39: (-30720, 0, 51, 8, 51, 0), 32: (-10240, 0, 60, 10, 60, 0),
                31: (-20480, 10240, 69, 15, 71, 0), 12: (20480, 10240, 80, 21, 85, 0),
                13: (30720, 0, 80, 12, 80, 0), 9: (10240, 30720, 45, 11, 49, 0),
                23: (-10240, -20480, 204, 61, 236, 0), 16: (30720, 40960, 33, 6, 34, 0),
                33: (-10240, 40960, 40, 8, 40, 0), 34: (-20480, 20480, 38, 5, 38, 15),
                35: (-30720, 20480, 40, 7, 40, 0)}[room]
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
        if room == 12 and (floor in (0, 1, 2, 10, 11) or other != 255):
            continue
        if room == 13 and floor in (3, 4):
            continue
        # SC0304's inner shell follows the usable hallway around its alcove.
        if room == 16 and (floor == 1 or (floor == 0 and not 5 <= i <= 12)):
            continue
        # Use the inner shell and the furniture silhouettes on both floors.
        if room == 33 and 24 <= i < 32:
            continue
        if room == 34 and 22 <= i < 30:
            continue
        # The equipment room starts with eight edges outside the inner shell.
        if room == 35 and i < 8:
            continue
        # The infirmary walls meet at shared splits. Keep their exposed faces.
        if room == 9 and other != 255:
            continue
        # Keep the tower perimeter, raised planters, trees and posts.
        # The entrance and tower ramps remain traversable.
        if room == 23 and (floor in (1, 30, 37, 38, 39, 40) or 50 <= floor < 60 or other != 255):
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
    if room == 34:
        cylinder_offset, = struct.unpack_from("<I", collision, 0x18)
        for i in range(cylinders):
            flags, px, py, pz, radius = struct.unpack_from("<H4h", collision, cylinder_offset+10*i)
            if flags != 1 or py != -147 or not 0 < radius < 256:
                raise ValueError("unexpected lab obstacle cylinder")
            output.extend(struct.pack("<3h", px-5120, pz-5120, radius))
    if room != 23:
        return output

    floors = bytearray()
    for plane in (*range(37, 41), *range(50, 60)):
        corners = set()
        for i in range(count):
            a, b, left, right = struct.unpack_from("4B", collision, polygon+10*i+6)
            if plane in (left, right):
                corners.update(struct.unpack_from("<3h", collision, vert+6*k)[::2] for k in (a, b))
        if len(corners) != 4:
            raise ValueError("unexpected courtyard floor boundary")
        center_x = sum(x for x, _ in corners)
        center_z = sum(z for _, z in corners)
        # Store the convex floor's corners in winding order for the ARM9 test.
        for x, z in sorted(corners, key=lambda p: math.atan2(4*p[1]-center_z, 4*p[0]-center_x)):
            floors.extend(struct.pack("<2h", x-5120, z-5120))
        xref, y, zref, _, sx, sz = struct.unpack_from("<3hH2h", collision, surface+12*plane)
        floors.extend(struct.pack("<5h", xref-5120, y, zref-5120, sx, sz))
    return struct.pack("<H", len(output)//12) + output + floors


def model_directory(data):
    _, count, model_ptr = struct.unpack_from("<III", data, 4)
    models = {}
    for i in range(count):
        name, meshes, base, _, _, ptr = struct.unpack_from("<8s4BI", data, model_ptr + 16*i)
        models[name.rstrip(b"\0")] = data, meshes, base, ptr
    return models


def infirmary_items(disc):
    # BG_ITEM.PLM uses four opaque CLUTs across two PS1 pages. The three
    # models sample only the first 64 columns. Pack those rows into one page.
    pixels = bytearray(64*256//2)
    colors = bytearray()
    for name, words, rows, dest, cluts in (
            ("ITEM/TIM00.TIM", 64, 128, 0, (0, 1)),
            ("TIM/BG_ETC.TIM", 32, 32, 128, (9, 10))):
        tim = disc.read(name)
        magic, flags, length, _, _, width, palettes = struct.unpack_from("<III4H", tim)
        image = 8 + length
        size, _, _, stride, height = struct.unpack_from("<I4H", tim, image)
        if (magic, flags, width, stride, height) != (16, 8, 16, words, 256) or max(cluts) >= palettes:
            raise ValueError("unexpected common item texture")
        if size != 12+2*stride*height or image+size > len(tim):
            raise ValueError("truncated common item texture")
        for y in range(rows):
            start = image+12+y*stride*2
            pixels[(dest+y)*32:(dest+y+1)*32] = tim[start:start+32]
        for row in cluts:
            palette = tim[20+row*32:52+row*32]
            if 0 in struct.unpack("<16H", palette):
                raise ValueError("unexpected transparent common item color")
            colors.extend(palette)

    overlay = disc.read("VIN/MAP1_S00.BIN")
    # g_CommonWorldObjectPoses[3..4] and Map_WorldObjectsInit's notebook.
    poses = [(b"PAD_NEAR", (int(61.8531*4096), int(-0.805*4096), int(140.6099*4096)),
              int(92.9*4096/360))]
    for i, name in ((3, b"DRINK_NE"), (4, b"AIDKIT_N")):
        x,y,z,rx,ry,rz = struct.unpack_from("<3i3h", overlay, 0x800dd71c-0x800c9578+20*i)
        if rx or rz:
            raise ValueError("unsupported common item rotation")
        poses.append((name, (x,y,z), ry))
    return pixels, colors, poses


def vertical_bounds(points, depth_spread):
    # Some music-room decorations are slanted or split into triangles.
    for axis, across in ((0, 2), (2, 0)):
        depths = [p[axis] for p in points]
        if max(depths)-min(depths) > depth_spread:
            continue
        horizontal = [p[across] for p in points]
        vertical = [p[1] for p in points]
        winding = sum(a[across]*b[1]-a[1]*b[across] for a, b in zip(points, points[1:]+points[:1]))
        if not winding:
            continue
        side = 1 if (winding > 0) == (axis == 0) else -1
        depth = min(depths) if side == 1 else max(depths)
        return (axis, depth, side), (min(horizontal), max(horizontal), min(vertical), max(vertical))
    return None


def vertical_rectangle(points):
    for axis, across in ((0, 2), (2, 0)):
        if any(p[axis] != points[0][axis] for p in points[1:]):
            continue
        horizontal = sorted({p[across] for p in points})
        vertical = sorted({p[1] for p in points})
        if len(horizontal) != 2 or len(vertical) != 2 or len(set(points)) != 4:
            return None
        a, b, c, _ = points
        winding = (b[across]-a[across])*(c[1]-a[1]) - (b[1]-a[1])*(c[across]-a[across])
        # The renderer keeps clockwise faces. Move toward their visible side.
        side = 1 if (winding > 0) == (axis == 0) else -1
        return (axis, points[0][axis], side), (*horizontal, *vertical)
    return None


def school_room(disc, room=39):
    cx, cz = {39: (253, 0), 32: (255, 0), 31: (254, 1), 12: (2, 1), 13: (3, 0), 9: (1, 3),
              23: (255, 254), 16: (3, 4), 33: (255, 4), 34: (254, 2), 35: (253, 2)}[room]
    data = disc.read(f"BG/SC{cx:02X}{cz:02X}.IPD")
    if data[:4] != bytes((20, 0, cx, cz)):
        raise ValueError("unexpected school chunk coordinates")
    lm, = struct.unpack_from("<I", data, 4)
    mat_count = data[lm + 3]
    mat_ptr, = struct.unpack_from("<I", data, lm + 4)
    models = model_directory(memoryview(data)[lm:])

    pages = bytearray()
    palettes = bytearray()
    page_info = []
    color_keys = []
    remapped_regions = {}
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
        pixels = bytearray(tim[image+12:image+size])
        if room == 23 and name == "SC1FSE2":
            # The two tree and window palettes have different transparent
            # indices, but their tiles occupy separate rectangles on the page.
            for row, key, rect in ((2, 14, (0, 63, 0, 127)), (4, 15, (64, 159, 0, 63))):
                if {j for j, value in enumerate(cluts[row]) if not value} != {key}:
                    raise ValueError("unexpected courtyard cutout palette")
                remapped_regions[i, row] = rect
                left, right, top, bottom = rect
                for y in range(top, bottom+1):
                    for j in range(y*words*2+left//2, y*words*2+right//2+1):
                        pixels[j] ^= key | key << 4
                cluts[row] = tuple(cluts[row][j ^ key] for j in range(16))
        if room == 23 and name == "SC1FSEH":
            # The last CLUT is unreferenced in this IPD.
            cluts = cluts[:7]
            page_info[-1] = (*page_info[-1][:3], len(cluts))
        # A PS1 transparent texel is a zero palette word. DS paletted textures
        # support a transparent index of zero only. Permute the page once.
        indices = {idx for clut in cluts for idx, color in enumerate(clut) if color == 0}
        if len(indices) > 1:
            raise ValueError(f"incompatible transparent indices on {name}")
        key = next(iter(indices), 0)
        color_keys.append([clut[key] == 0 for clut in cluts])
        pages.extend(byte ^ (key | key << 4) for byte in pixels)
        for clut in cluts:
            palettes.extend(struct.pack("<16H", *(clut[idx ^ key] for idx in range(16))))

    # Group opaque polygons by texture and CLUT without duplicating any texture.
    groups = defaultdict(bytearray)
    walls = defaultdict(list)
    decorations = []
    music_decorations = []
    vertex_scale = 4 if room in (12, 16, 31) else 8
    def add_model(name, matrix, position, state=0, *, item_page=None):
        model_data, meshes, base, ptr = models[name]
        for j in range(meshes):
            faces, nverts, _, _, prim, xy, z, _, _ = struct.unpack_from("<4B5I", model_data, ptr + 24*j)
            vertices = []
            for v in range(nverts):
                coord = (*struct.unpack_from("<hh", model_data, xy+4*v),
                         *struct.unpack_from("<h", model_data, z+2*v))
                transformed = [position[a] + (sum(matrix[3*a+k]*coord[k] for k in range(3)) >> 12)
                               for a in range(3)]
                x, y, zpos = transformed
                # The longer hallways need the wider V16 range.
                vertices.append(((x-5120)*vertex_scale, -y*vertex_scale, -(zpos-5120)*vertex_scale))
            for k in range(faces):
                face = model_data[prim+20*k:prim+20*(k+1)]
                mat = face[7] & 127
                row = int.from_bytes(face[2:4], "little") >> 6
                v_offset = 0
                untextured = room == 23 and mat == 127
                if untextured:
                    # Model_MaterialFlagsApply uses the no-material sentinel.
                    mat = row = 0
                if item_page is not None:
                    # BG_ETC rows 9/10 occupy CLUTs 2/3 in the atlas.
                    if (mat, row) not in ((0, 0), (0, 1), (1, 9), (1, 10)):
                        raise ValueError("unexpected common item material")
                    if mat == 1:
                        row -= 7
                        v_offset = 128
                    mat = item_page
                if mat >= len(page_info) or row >= page_info[mat][3]:
                    raise ValueError(f"invalid material {mat} or CLUT {row} on {name!r}")
                if remapped_regions:
                    us, vs = zip(*(face[i:i+2] for i in (0, 4, 8, 10)))
                    for (page, clut), (left, right, top, bottom) in remapped_regions.items():
                        if page != mat:
                            continue
                        if row == clut:
                            valid = left <= min(us) <= max(us) <= right and top <= min(vs) <= max(vs) <= bottom
                        else:
                            valid = max(us) < left or min(us) > right or max(vs) < top or min(vs) > bottom
                        if not valid:
                            raise ValueError("courtyard texture region overlaps a different palette")
                flags = (face[7] >> 7) | (color_keys[mat][row] << 1) | (untextured << 2)
                group = groups[state, mat, row, flags]
                start = len(group)
                points = []
                for corner in (0, 1, 3, 2):  # PS1 quad strip to DS perimeter
                    idx = face[12+corner] - base
                    if not 0 <= idx < nverts:
                        raise ValueError("vertex outside LM mesh")
                    uv = (0, 4, 8, 10)[corner]
                    u, v = face[uv:uv+2]
                    if item_page is not None and (u >= 64 or v >= (32 if v_offset else 128)):
                        raise ValueError("common item UV outside atlas")
                    point = vertices[idx]
                    group.extend(struct.pack("<3hBB", *point, u, v+v_offset))
                    points.append(point)
                if not state and item_page is None and not flags & 1:
                    music = room == 39
                    poster = name == b"NULL_NEA" and mat == 2 and (j == 0 or (j == 1 and 40 <= k <= 47))
                    if music and poster:
                        face = vertical_bounds(points, 2*vertex_scale)
                        if face:
                            music_decorations.append((*face, group, start))
                        continue
                    if music and name in (b"NULL1", b"W5_FAR"):
                        rectangle = vertical_bounds(points, 0)
                    else:
                        rectangle = vertical_rectangle(points)
                    if rectangle is not None:
                        plane, bounds = rectangle
                        if name == b"NULL_NEA":
                            # The tower plaques share a plane with several
                            # masonry triangles from SC1FSE1.
                            plaque = room == 23 and j == 0 and k in (32, 33)
                            decorations.append((plane, bounds, group, start, plaque))
                        else:
                            walls[plane].append(bounds)

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

    # The PS1 ordering table layers wall decorations over their backing
    # faces. Separate confirmed coplanar pairs by two Q8 units (7.8 mm).
    for plane, bounds, group, start, plaque in decorations:
        left, right, bottom, top = bounds
        if plaque or any(bounds != b and b[0] <= left and b[1] >= right and b[2] <= bottom and b[3] >= top
                         for b in walls[plane]):
            axis, depth, side = plane
            for offset in range(start+2*axis, start+32, 8):
                struct.pack_into("<h", group, offset, depth+side*2*vertex_scale)

    # Preserve a two-Q8-unit gap for the music-room posters, including
    # vertices shared by several triangles of the same decoration.
    for (axis, depth, side), bounds, group, start in music_decorations:
        left, right, bottom, top = bounds
        for delta in range(-2*vertex_scale, 2*vertex_scale+1, vertex_scale):
            wall = depth+delta
            if not any(b[0] <= left and b[1] >= right and b[2] <= bottom and b[3] >= top
                       for b in walls[axis, wall, side]):
                continue
            minimum = side*wall + 2*vertex_scale
            for offset in range(start+2*axis, start+32, 8):
                coord, = struct.unpack_from("<h", group, offset)
                struct.pack_into("<h", group, offset, side*max(side*coord, minimum))

    if room == 9:
        page = len(page_info)
        pixels, colors, poses = infirmary_items(disc)
        page_info.append((len(pages), 64, len(palettes)//32, 4))
        pages.extend(pixels)
        palettes.extend(colors)
        color_keys.append([False]*4)
        plm = disc.read("BG/BG_ITEM.PLM")
        models.update(model_directory(plm))
        # Keep separate batches for the two props gated by pickup flags.
        for state, (name, (x,y,z), heading) in zip((0, 3, 4), poses, strict=True):
            angle = heading * math.tau/4096
            c, s = round(math.cos(angle)*4096), round(math.sin(angle)*4096)
            add_model(name, (c,0,s,0,4096,0,-s,0,c), ((x>>4)-10240, y>>4, (z>>4)-30720), state,
                      item_page=page)
    if room != 39:
        magic = {9: b"SHI3", 12: b"SHF3", 13: b"SHS2", 16: b"SHU1", 23: b"SHY2", 31: b"SHE1",
                 32: b"SHC1", 33: b"SHH1", 34: b"SHL1", 35: b"SHQ1"}[room]
        extra = b""
        if room == 9:
            extra = medical_pickups(disc)
        elif room == 12:
            extra = prompt_data(disc, 13, first_floor=True, choices=False)
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
