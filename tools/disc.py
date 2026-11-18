"""Read selected USA 1.1 assets from the owner's raw track without expanding it.

The packed SLUS file table follows silent-hill-decomp/tools/silentassets/extract.py.
Copyright (C) 2026 shdecompilations. SPDX-License-Identifier: GPL-3.0-only
"""
import hashlib
import re
import struct
from contextlib import contextmanager
from pathlib import Path

TRACK_BYTES = 616494480
TRACK_SHA1 = "34278d31d9b9b12b3b5db5e45bcbe548991ecbc7"


def track_from_cue(cue):
    lines = cue.read_text(encoding="utf-8-sig").splitlines()
    commands = [line.strip() for line in lines if line.strip()
                and not line.lstrip().upper().startswith("REM ")]
    match = re.fullmatch(r'FILE\s+(?:"([^"]+)"|(\S+))\s+BINARY', commands[0] if commands else "", re.I)
    if (not match or len(commands) != 3
            or commands[1].upper().split() != ["TRACK", "01", "MODE2/2352"]
            or commands[2].upper().split() != ["INDEX", "01", "00:00:00"]):
        raise ValueError("expected a single raw MODE2/2352 track at index 01 00:00:00")
    return cue.parent / (match[1] or match[2]).replace("\\", "/")


def read_sectors(track, lba, size):
    data = bytearray()
    while len(data) < size:
        track.seek(lba * 2352 + 24)
        length = min(size - len(data), 2048)
        block = track.read(length)
        if len(block) != length:
            raise ValueError("truncated raw track")
        data.extend(block)
        lba += 1
    return data


def extent(record):
    return struct.unpack_from("<I", record, 2)[0], struct.unpack_from("<I", record, 10)[0]


def root_files(track):
    pvd = read_sectors(track, 16, 2048)
    if pvd[:7] != b"\x01CD001\x01":
        raise ValueError("missing ISO9660 volume descriptor")
    directory = read_sectors(track, *extent(pvd[156:190]))
    entries = {}
    pos = 0
    while pos < len(directory):
        length = directory[pos]
        if not length:
            pos += 2048 - pos % 2048
            continue
        record = directory[pos:pos + length]
        name = record[33:33 + record[32]].decode("ascii").split(";")[0]
        entries[name] = extent(record)
        pos += length
    return entries


class GameDisc:
    def __init__(self, track):
        self.track = track
        files = root_files(track)
        exe = read_sectors(track, *files["SLUS_007.07"])
        self.files = {}
        dirs = "1ST ANIM BG CHARA ITEM MISC SND TEST TIM VIN XA".split()
        types = "TIM VAB BIN DMS ANM PLM IPD ILM TMD DAT KDT CMP TXT UU1 UU2 XA".split()
        first_lba = struct.unpack_from("<I", exe, 0xB91C)[0] & 0x7ffff
        for meta, lo, hi in struct.iter_unpack("<III", exe[0xB91C:0xB91C + 2074 * 12]):
            directory = dirs[lo & 15]
            if directory == "XA":
                continue
            chars = ((lo >> 4) & 0xffffff) | ((hi & 0xffffff) << 24)
            name = "".join(chr(32 + ((chars >> bit) & 63)) for bit in range(0, 48, 6)).strip()
            lba = files["SILENT."][0] + (meta & 0x7ffff) - first_lba
            self.files[f"{directory}/{name}.{types[(hi >> 24) & 15]}"] = (lba, (meta >> 19) * 256)

    def read(self, name):
        lba, size = self.files[name]
        if not size:
            raise ValueError(f"{name} requires an implicit size")
        return read_sectors(self.track, lba, size)


@contextmanager
def open_disc(cue: Path):
    path = track_from_cue(cue)
    with path.open("rb") as track:
        if path.stat().st_size != TRACK_BYTES:
            raise ValueError("unexpected track size for USA 1.1")
        digest = hashlib.file_digest(track, "sha1").hexdigest()
        if digest != TRACK_SHA1:
            raise ValueError(f"unsupported track SHA-1 {digest}")
        yield GameDisc(track)
