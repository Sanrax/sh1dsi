"""Convert one-shot PS1 VAB tones to PCM for native sound channels."""
from array import array
import struct
import sys


def decode_vag(data):
    coefficients = ((0, 0), (60, 0), (115, -52), (98, -55), (122, -60))
    pcm = array("h")
    prev = older = 0
    for off in range(0, len(data), 16):
        param, flags = data[off:off + 2]
        shift, filter_id = param & 15, param >> 4
        if filter_id >= 5 or shift > 12:
            raise ValueError("unsupported SPU ADPCM predictor")
        a, b = coefficients[filter_id]
        for packed in data[off + 2:off + 16]:
            for nibble in (packed & 15, packed >> 4):
                sample = ((nibble ^ 8) - 8) << (12 - shift)
                sample += (prev*a + older*b + 32) >> 6
                older, prev = prev, max(-32768, min(32767, sample))
                pcm.append(prev)
        if flags & 1:
            if flags & 2:
                raise ValueError("looping sample in a one-shot sound bank")
            if sys.byteorder == "big":
                pcm.byteswap()
            return pcm.tobytes()
    raise ValueError("unterminated VAG sample")


def convert_tones(vab, notes, samples):
    programs = struct.unpack_from("<H", vab, 18)[0]
    offsets = 0x820 + programs * 512
    sizes = struct.unpack_from("<256H", vab, offsets)
    unique = {}
    tones = bytearray()
    for program, note, level in notes:
        count = vab[32 + program * 16]
        rows = 0x820 + 512 * sum(bool(vab[32 + i * 16]) for i in range(program))
        matches = [rows + 32*i for i in range(count)
                   if vab[rows + 32*i + 6] <= note <= vab[rows + 32*i + 7]]
        if len(matches) != 1:
            raise ValueError("expected one VAB tone per note")
        tone = matches[0]
        vag = struct.unpack_from("<H", vab, tone + 22)[0]
        if vag not in unique:
            start = offsets + 512 + sum(sizes[:vag]) * 8
            pcm = decode_vag(vab[start:start + sizes[vag]*8])
            unique[vag] = len(samples), len(pcm)
            samples.extend(pcm)
        center, fine = vab[tone + 4:tone + 6]
        pitch = int(4096 * 2 ** ((note - center + fine / 128) / 12))
        rate = round(44100 * pitch / 4096)
        volume = vab[tone + 2] * vab[33 + program * 16] // 127
        volume = volume * level // 127
        tones.extend(struct.pack("<IIHBB", *unique[vag], rate, volume, vab[tone + 3]))
    return tones


def school_bank(map_vab, base_vab):
    # MAP1_S01 and g_Vab_InfoTable: SFX_PAIRS[1, 2] use BASE.VAB.
    samples = bytearray()
    notes = [(1, n, 127) for n in (48, 51, 52, 53, 54, 56, 59, 61)]
    tones = convert_tones(map_vab, notes + [(0, 60, 97), (0, 52, 127)], samples)
    # Event_DisplayMapMsgWithSfx starts the latch at half volume.
    tones.extend(convert_tones(base_vab, [(0, 53, 127), (0, 54, 107),
                                         (1, 54, 57), (1, 55, 57), (2, 54, 64)], samples))
    return tones + samples
