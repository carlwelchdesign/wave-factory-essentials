#!/usr/bin/env python3
"""Deterministic, code-authored satin material. No baked-in labels or controls."""
from pathlib import Path
import math
import struct
import zlib


def png(path, width, height, scale):
    rows = bytearray()
    for y in range(height):
        rows.append(0)
        for x in range(width):
            # Fine brushed grain, subtle vertical lighting; same logical surface at both scales.
            grain = (((x // scale * 73 + y // scale * 151) % 127) / 127 - .5) * 2
            level = 220 - 13 * y / height + 4 * math.cos(x / width * math.pi) + grain
            rows.extend(max(0, min(255, round(level + offset))) for offset in (0, 3, 6))
    def chunk(kind, data):
        return struct.pack('!I', len(data)) + kind + data + struct.pack('!I', zlib.crc32(kind + data))
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('!IIBBBBB', width, height, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(rows, 9)) + chunk(b'IEND', b''))


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1] / 'plugins/PadSampler/resources/img'
    root.mkdir(parents=True, exist_ok=True)
    for scale in (1, 2):
        png(root / ('precision-satin' + ('@2x' if scale == 2 else '') + '.png'), 980 * scale, 760 * scale, scale)
