#!/usr/bin/env python3
import pathlib
import struct
import sys
import zlib

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def paeth(a, b, c):
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def read_png(path):
    data = pathlib.Path(path).read_bytes()
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError(f"{path}: not a PNG")

    offset = len(PNG_SIGNATURE)
    width = height = bit_depth = color_type = interlace = None
    compressed = bytearray()

    while offset < len(data):
        length = struct.unpack(">I", data[offset:offset + 4])[0]
        kind = data[offset + 4:offset + 8]
        payload = data[offset + 8:offset + 8 + length]
        offset += 12 + length

        if kind == b"IHDR":
            width, height, bit_depth, color_type, _compression, _filter, interlace = struct.unpack(
                ">IIBBBBB", payload
            )
        elif kind == b"IDAT":
            compressed.extend(payload)
        elif kind == b"IEND":
            break

    if bit_depth != 8 or color_type not in (2, 6) or interlace != 0:
        raise ValueError(
            f"{path}: unsupported PNG layout bit_depth={bit_depth} "
            f"color_type={color_type} interlace={interlace}"
        )

    channels = 3 if color_type == 2 else 4
    stride = width * channels
    raw = zlib.decompress(bytes(compressed))
    expected = height * (stride + 1)
    if len(raw) != expected:
        raise ValueError(f"{path}: decoded byte count {len(raw)} != {expected}")

    rows = []
    previous = bytearray(stride)
    cursor = 0
    for _y in range(height):
        filter_type = raw[cursor]
        cursor += 1
        scan = bytearray(raw[cursor:cursor + stride])
        cursor += stride

        for x in range(stride):
            left = scan[x - channels] if x >= channels else 0
            up = previous[x]
            up_left = previous[x - channels] if x >= channels else 0

            if filter_type == 0:
                value = scan[x]
            elif filter_type == 1:
                value = (scan[x] + left) & 0xFF
            elif filter_type == 2:
                value = (scan[x] + up) & 0xFF
            elif filter_type == 3:
                value = (scan[x] + ((left + up) // 2)) & 0xFF
            elif filter_type == 4:
                value = (scan[x] + paeth(left, up, up_left)) & 0xFF
            else:
                raise ValueError(f"{path}: unsupported filter {filter_type}")
            scan[x] = value

        rows.append(bytes(scan))
        previous = scan

    return width, height, channels, rows


def body_pixels(image):
    width, height, channels, rows = image
    y0 = min(210, height // 4)
    y1 = max(y0 + 1, height - 170)
    pixels = []
    for y in range(y0, y1, 3):
        row = rows[y]
        for x in range(0, width, 3):
            base = x * channels
            pixels.append((row[base], row[base + 1], row[base + 2]))
    return pixels


def assert_nonblank(path, image):
    width, height, _channels, _rows = image
    if width < 400 or height < 700:
        raise AssertionError(f"{path}: unexpected screenshot size {width}x{height}")

    pixels = body_pixels(image)
    unique = len(set(pixels))
    lumas = [(r * 3 + g * 6 + b) // 10 for r, g, b in pixels]
    spread = max(lumas) - min(lumas)
    if unique < 24 or spread < 28:
        raise AssertionError(
            f"{path}: image looks blank/flat (sample_colors={unique}, luma_spread={spread})"
        )
    return unique, spread


def changed_samples(first, second):
    if first[:3] != second[:3]:
        raise AssertionError("cannot compare screenshots with different dimensions/layout")
    a = body_pixels(first)
    b = body_pixels(second)
    return sum(
        1
        for pa, pb in zip(a, b)
        if abs(pa[0] - pb[0]) + abs(pa[1] - pb[1]) + abs(pa[2] - pb[2]) >= 24
    )


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: check-emulator-images.py EVIDENCE_DIR")

    root = pathlib.Path(sys.argv[1])
    lines = []

    for material in ("halite", "quartz", "bismuth"):
        images = {}
        for state in ("both", "solid", "net", "net-rotated"):
            path = root / f"{material}-{state}.png"
            image = read_png(path)
            unique, spread = assert_nonblank(path, image)
            images[state] = image
            lines.append(
                f"{material} {state}: {image[0]}x{image[1]}, "
                f"sample_colors={unique}, luma_spread={spread}"
            )

        solid_net = changed_samples(images["solid"], images["net"])
        net_rotated = changed_samples(images["net"], images["net-rotated"])
        lines.append(
            f"{material}: changed_samples solid->net={solid_net}, "
            f"net->rotated={net_rotated}"
        )

        if solid_net < 250:
            raise AssertionError(
                f"{material}: SOLID and NET do not differ enough in the 3D body ({solid_net})"
            )
        if net_rotated < 250:
            raise AssertionError(
                f"{material}: drag did not visibly rotate the NET view ({net_rotated})"
            )

    report = "\n".join(lines) + "\n"
    (root / "image-check.txt").write_text(report, encoding="utf-8")
    print(report, end="")


if __name__ == "__main__":
    main()
