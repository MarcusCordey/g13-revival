#!/usr/bin/env python3
"""Convert PNG images to the Logitech G13 native 160x48 framebuffer layout.

The G13 stores six vertical banks of eight pixels. Byte offset and bit position:

    offset = x + (y // 8) * 160
    bit    = y & 7

A set bit represents a white/active pixel. The decoder uses only Python's
standard library and accepts non-interlaced PNG images in the common grayscale,
RGB, palette, grayscale+alpha and RGBA color types.
"""

from __future__ import annotations

import argparse
import hashlib
import re
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
G13_WIDTH = 160
G13_HEIGHT = 48
G13_FRAME_BYTES = G13_WIDTH * G13_HEIGHT // 8


class ConversionError(ValueError):
    """Raised when an input cannot be converted safely."""


@dataclass(frozen=True)
class DecodedPng:
    width: int
    height: int
    bit_depth: int
    color_type: int
    rgba: bytes

    @property
    def is_native_one_bit(self) -> bool:
        return self.bit_depth == 1 and self.color_type == 0


def _paeth(left: int, above: int, upper_left: int) -> int:
    estimate = left + above - upper_left
    distance_left = abs(estimate - left)
    distance_above = abs(estimate - above)
    distance_upper_left = abs(estimate - upper_left)
    if distance_left <= distance_above and distance_left <= distance_upper_left:
        return left
    if distance_above <= distance_upper_left:
        return above
    return upper_left


def _unfilter_scanlines(
    compressed: bytes,
    width: int,
    height: int,
    channels: int,
    bit_depth: int,
) -> list[bytes]:
    row_bytes = (width * channels * bit_depth + 7) // 8
    bytes_per_pixel = max(1, (channels * bit_depth + 7) // 8)
    expected = height * (row_bytes + 1)
    if len(compressed) != expected:
        raise ConversionError(
            f"decompressed PNG data has {len(compressed)} bytes; expected {expected}"
        )

    rows: list[bytes] = []
    previous = bytearray(row_bytes)
    cursor = 0
    for row_number in range(height):
        filter_type = compressed[cursor]
        cursor += 1
        encoded = compressed[cursor : cursor + row_bytes]
        cursor += row_bytes
        decoded = bytearray(row_bytes)

        for index, value in enumerate(encoded):
            left = decoded[index - bytes_per_pixel] if index >= bytes_per_pixel else 0
            above = previous[index]
            upper_left = (
                previous[index - bytes_per_pixel]
                if index >= bytes_per_pixel
                else 0
            )
            if filter_type == 0:
                predictor = 0
            elif filter_type == 1:
                predictor = left
            elif filter_type == 2:
                predictor = above
            elif filter_type == 3:
                predictor = (left + above) // 2
            elif filter_type == 4:
                predictor = _paeth(left, above, upper_left)
            else:
                raise ConversionError(
                    f"unsupported PNG filter {filter_type} in row {row_number}"
                )
            decoded[index] = (value + predictor) & 0xFF

        rows.append(bytes(decoded))
        previous = decoded
    return rows


def _unpack_samples(row: bytes, bit_depth: int, count: int) -> list[int]:
    if bit_depth == 8:
        if len(row) < count:
            raise ConversionError("PNG scanline contains too few 8-bit samples")
        return list(row[:count])
    if bit_depth == 16:
        if len(row) < count * 2:
            raise ConversionError("PNG scanline contains too few 16-bit samples")
        return [
            (row[index * 2] << 8) | row[index * 2 + 1]
            for index in range(count)
        ]
    if bit_depth not in (1, 2, 4):
        raise ConversionError(f"unsupported PNG bit depth {bit_depth}")

    mask = (1 << bit_depth) - 1
    samples: list[int] = []
    for sample_index in range(count):
        bit_offset = sample_index * bit_depth
        byte_index = bit_offset // 8
        shift = 8 - bit_depth - (bit_offset % 8)
        samples.append((row[byte_index] >> shift) & mask)
    return samples


def _scale_sample(value: int, bit_depth: int) -> int:
    maximum = (1 << bit_depth) - 1
    return (value * 255 + maximum // 2) // maximum


def read_png(path: Path | str) -> DecodedPng:
    """Read a non-interlaced PNG and return canonical 8-bit RGBA pixels."""

    source = Path(path)
    try:
        data = source.read_bytes()
    except OSError as error:
        raise ConversionError(f"cannot read {source}: {error}") from error

    if not data.startswith(PNG_SIGNATURE):
        raise ConversionError(f"{source} is not a PNG file")

    cursor = len(PNG_SIGNATURE)
    width = height = bit_depth = color_type = interlace = None
    palette: list[tuple[int, int, int]] | None = None
    transparency = b""
    idat = bytearray()
    saw_end = False

    while cursor < len(data):
        if cursor + 12 > len(data):
            raise ConversionError(f"{source} has a truncated PNG chunk header")
        length = struct.unpack(">I", data[cursor : cursor + 4])[0]
        chunk_type = data[cursor + 4 : cursor + 8]
        chunk_end = cursor + 12 + length
        if chunk_end > len(data):
            name = chunk_type.decode("ascii", "replace")
            raise ConversionError(f"{source} has truncated {name} chunk data")
        payload = data[cursor + 8 : cursor + 8 + length]
        expected_crc = struct.unpack(">I", data[cursor + 8 + length : chunk_end])[0]
        actual_crc = zlib.crc32(chunk_type)
        actual_crc = zlib.crc32(payload, actual_crc) & 0xFFFFFFFF
        if actual_crc != expected_crc:
            name = chunk_type.decode("ascii", "replace")
            raise ConversionError(f"{source} has an invalid {name} chunk CRC")

        if chunk_type == b"IHDR":
            if length != 13 or width is not None:
                raise ConversionError(f"{source} has an invalid IHDR chunk")
            (
                width,
                height,
                bit_depth,
                color_type,
                compression,
                filter_method,
                interlace,
            ) = struct.unpack(">IIBBBBB", payload)
            if width == 0 or height == 0:
                raise ConversionError(f"{source} has zero width or height")
            if compression != 0 or filter_method != 0:
                raise ConversionError(f"{source} uses an unsupported PNG method")
            if interlace != 0:
                raise ConversionError(
                    f"{source} is interlaced; save it as a non-interlaced PNG"
                )
        elif chunk_type == b"PLTE":
            if length == 0 or length % 3 != 0 or length > 768:
                raise ConversionError(f"{source} has an invalid PLTE chunk")
            palette = [
                (payload[index], payload[index + 1], payload[index + 2])
                for index in range(0, length, 3)
            ]
        elif chunk_type == b"tRNS":
            transparency = payload
        elif chunk_type == b"IDAT":
            idat.extend(payload)
        elif chunk_type == b"IEND":
            saw_end = True
            break
        elif chunk_type and 65 <= chunk_type[0] <= 90:
            name = chunk_type.decode("ascii", "replace")
            raise ConversionError(f"{source} contains unsupported critical chunk {name}")
        cursor = chunk_end

    if width is None or not saw_end or not idat:
        raise ConversionError(f"{source} is missing required PNG chunks")

    valid_depths = {
        0: (1, 2, 4, 8, 16),
        2: (8, 16),
        3: (1, 2, 4, 8),
        4: (8, 16),
        6: (8, 16),
    }
    if color_type not in valid_depths or bit_depth not in valid_depths[color_type]:
        raise ConversionError(
            f"{source} uses unsupported color type {color_type} "
            f"with bit depth {bit_depth}"
        )
    if color_type == 3 and palette is None:
        raise ConversionError(f"{source} is indexed but has no palette")

    try:
        decompressed = zlib.decompress(bytes(idat))
    except zlib.error as error:
        raise ConversionError(f"{source} has invalid compressed pixel data: {error}") from error

    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[color_type]
    rows = _unfilter_scanlines(
        decompressed, width, height, channels, bit_depth
    )

    rgba = bytearray(width * height * 4)
    target = 0
    for row in rows:
        samples = _unpack_samples(row, bit_depth, width * channels)
        for x in range(width):
            base = x * channels
            if color_type == 0:
                raw_gray = samples[base]
                gray = _scale_sample(raw_gray, bit_depth)
                alpha = 255
                if len(transparency) == 2:
                    transparent_gray = struct.unpack(">H", transparency)[0]
                    if raw_gray == transparent_gray:
                        alpha = 0
                red = green = blue = gray
            elif color_type == 2:
                raw = samples[base : base + 3]
                red, green, blue = (
                    _scale_sample(sample, bit_depth) for sample in raw
                )
                alpha = 255
                if len(transparency) == 6:
                    transparent_rgb = struct.unpack(">HHH", transparency)
                    if tuple(raw) == transparent_rgb:
                        alpha = 0
            elif color_type == 3:
                palette_index = samples[base]
                if palette_index >= len(palette):
                    raise ConversionError(
                        f"{source} references missing palette index {palette_index}"
                    )
                red, green, blue = palette[palette_index]
                alpha = (
                    transparency[palette_index]
                    if palette_index < len(transparency)
                    else 255
                )
            elif color_type == 4:
                gray = _scale_sample(samples[base], bit_depth)
                red = green = blue = gray
                alpha = _scale_sample(samples[base + 1], bit_depth)
            else:
                red, green, blue, alpha = (
                    _scale_sample(sample, bit_depth)
                    for sample in samples[base : base + 4]
                )

            rgba[target : target + 4] = bytes((red, green, blue, alpha))
            target += 4

    return DecodedPng(width, height, bit_depth, color_type, bytes(rgba))


def png_to_monochrome(
    image: DecodedPng,
    threshold: int = 128,
    strict_monochrome: bool = False,
) -> tuple[bool, ...]:
    """Convert canonical RGBA pixels to black/white over a black background."""

    if not 0 <= threshold <= 255:
        raise ConversionError("threshold must be between 0 and 255")

    pixels: list[bool] = []
    for offset in range(0, len(image.rgba), 4):
        red, green, blue, alpha = image.rgba[offset : offset + 4]
        if strict_monochrome:
            is_black = red == green == blue == 0 and alpha == 255
            is_white = red == green == blue == 255 and alpha == 255
            if not (is_black or is_white):
                pixel_index = offset // 4
                x = pixel_index % image.width
                y = pixel_index // image.width
                raise ConversionError(
                    f"pixel ({x},{y}) is not opaque pure black or white"
                )
            pixels.append(is_white)
            continue

        # Integer Rec. 601 luma, composited over the G13's black background.
        luminance = (299 * red + 587 * green + 114 * blue + 500) // 1000
        composited = (luminance * alpha + 127) // 255
        pixels.append(composited >= threshold)
    return tuple(pixels)


def monochrome_to_g13(
    pixels: Sequence[bool],
    width: int = G13_WIDTH,
    height: int = G13_HEIGHT,
) -> bytes:
    """Pack row-major monochrome pixels into the G13 vertical-byte layout."""

    if width != G13_WIDTH or height != G13_HEIGHT:
        raise ConversionError(
            f"G13 source must be exactly {G13_WIDTH}x{G13_HEIGHT}; "
            f"got {width}x{height}"
        )
    if len(pixels) != width * height:
        raise ConversionError(
            f"pixel buffer has {len(pixels)} entries; expected {width * height}"
        )

    frame = bytearray(G13_FRAME_BYTES)
    for y in range(height):
        for x in range(width):
            if pixels[y * width + x]:
                frame[x + (y // 8) * width] |= 1 << (y & 7)
    return bytes(frame)


def convert_png(
    path: Path | str,
    threshold: int = 128,
    strict_monochrome: bool = False,
) -> tuple[DecodedPng, bytes]:
    image = read_png(path)
    if image.width != G13_WIDTH or image.height != G13_HEIGHT:
        raise ConversionError(
            f"{path} must be exactly {G13_WIDTH}x{G13_HEIGHT}; "
            f"got {image.width}x{image.height}"
        )
    pixels = png_to_monochrome(image, threshold, strict_monochrome)
    return image, monochrome_to_g13(pixels, image.width, image.height)


def _png_chunk(chunk_type: bytes, payload: bytes) -> bytes:
    crc = zlib.crc32(chunk_type)
    crc = zlib.crc32(payload, crc) & 0xFFFFFFFF
    return (
        struct.pack(">I", len(payload))
        + chunk_type
        + payload
        + struct.pack(">I", crc)
    )


def write_one_bit_png(
    path: Path | str,
    width: int,
    height: int,
    pixels: Sequence[bool],
) -> None:
    """Write a deterministic non-interlaced grayscale 1-bit PNG."""

    if width <= 0 or height <= 0:
        raise ConversionError("PNG dimensions must be positive")
    if len(pixels) != width * height:
        raise ConversionError(
            f"pixel buffer has {len(pixels)} entries; expected {width * height}"
        )

    row_bytes = (width + 7) // 8
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # filter type None
        packed = bytearray(row_bytes)
        for x in range(width):
            if pixels[y * width + x]:
                packed[x // 8] |= 1 << (7 - (x & 7))
        raw.extend(packed)

    ihdr = struct.pack(">IIBBBBB", width, height, 1, 0, 0, 0, 0)
    output = (
        PNG_SIGNATURE
        + _png_chunk(b"IHDR", ihdr)
        + _png_chunk(b"IDAT", zlib.compress(bytes(raw), level=9))
        + _png_chunk(b"IEND", b"")
    )
    destination = Path(path)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(output)


def _identifier(prefix: str, source: Path, index: int) -> str:
    stem = re.sub(r"[^a-zA-Z0-9]+", "_", source.stem).strip("_").lower()
    if not stem:
        stem = f"frame_{index:02d}"
    return f"{prefix}_{stem}"


def render_arrays(
    sources: Sequence[Path],
    frames: Sequence[bytes],
    language: str,
    prefix: str,
) -> str:
    if len(sources) != len(frames) or not frames:
        raise ConversionError("at least one matching source/frame pair is required")

    identifiers = [
        _identifier(prefix, source, index)
        for index, source in enumerate(sources, start=1)
    ]
    if len(set(identifiers)) != len(identifiers):
        raise ConversionError("input filenames produce duplicate C identifiers")

    guard = re.sub(r"[^A-Z0-9]+", "_", prefix.upper()).strip("_")
    lines = [
        "/* Generated by tools/png_to_g13.py. Do not edit manually. */",
        f"/* Native layout: offset=x+(y/8)*{G13_WIDTH}, bit=(y&7), 1=white. */",
    ]
    for source, frame in zip(sources, frames):
        digest = hashlib.sha256(frame).hexdigest()
        lines.append(f"/* {source.name}: sha256(native)={digest} */")

    if language == "c":
        lines.extend(
            [
                f"#ifndef {guard}_H",
                f"#define {guard}_H",
                "",
                "#include <stdint.h>",
                "#if defined(__IMXRT1062__)",
                "#include <avr/pgmspace.h>",
                "#define G13_FRAME_STORAGE PROGMEM",
                "#else",
                "#define G13_FRAME_STORAGE",
                "#endif",
                "",
                f"#define {guard}_WIDTH {G13_WIDTH}",
                f"#define {guard}_HEIGHT {G13_HEIGHT}",
                f"#define {guard}_FRAME_BYTES {G13_FRAME_BYTES}",
                f"#define {guard}_FRAME_COUNT {len(frames)}",
                "",
            ]
        )
        declaration = "static const uint8_t"
    else:
        lines.extend(
            [
                "#pragma once",
                "",
                "#include <stdint.h>",
                "#if defined(__IMXRT1062__)",
                "#include <avr/pgmspace.h>",
                "#define G13_FRAME_STORAGE PROGMEM",
                "#else",
                "#define G13_FRAME_STORAGE",
                "#endif",
                "",
                f"static constexpr uint16_t {prefix}_width = {G13_WIDTH};",
                f"static constexpr uint16_t {prefix}_height = {G13_HEIGHT};",
                f"static constexpr uint16_t {prefix}_frame_bytes = {G13_FRAME_BYTES};",
                f"static constexpr uint8_t {prefix}_count = {len(frames)};",
                "",
            ]
        )
        declaration = "alignas(4) static const uint8_t"

    for identifier, frame in zip(identifiers, frames):
        lines.append(
            f"{declaration} {identifier}[{G13_FRAME_BYTES}] "
            "G13_FRAME_STORAGE = {"
        )
        for offset in range(0, len(frame), 16):
            values = ", ".join(f"0x{value:02X}" for value in frame[offset : offset + 16])
            lines.append(f"  {values},")
        lines.extend(["};", ""])

    lines.append(
        f"static const uint8_t *const {prefix}_frames[{len(frames)}] "
        "G13_FRAME_STORAGE = {"
    )
    for identifier in identifiers:
        lines.append(f"  {identifier},")
    lines.extend(["};", ""])
    lines.extend(["#undef G13_FRAME_STORAGE", ""])
    if language == "c":
        lines.extend([f"#endif  /* {guard}_H */", ""])
    return "\n".join(lines)


def _parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert 160x48 PNG files to native Logitech G13 arrays."
    )
    parser.add_argument("inputs", nargs="+", type=Path, help="ordered PNG frames")
    parser.add_argument("-o", "--output", type=Path, help="header output; default stdout")
    parser.add_argument(
        "--language",
        choices=("c", "cpp"),
        default="cpp",
        help="array declaration style (default: cpp)",
    )
    parser.add_argument(
        "--array-prefix",
        default="g13_startup",
        help="C identifier prefix (default: g13_startup)",
    )
    parser.add_argument(
        "--threshold",
        type=int,
        default=128,
        help="black/white luminance threshold, 0..255 (default: 128)",
    )
    parser.add_argument(
        "--strict-monochrome",
        action="store_true",
        help="reject every pixel that is not opaque pure black or white",
    )
    parser.add_argument(
        "--verify-only",
        action="store_true",
        help="validate and report frames without emitting arrays",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = _parse_args(argv if argv is not None else sys.argv[1:])
    try:
        sources = [path.resolve() for path in args.inputs]
        decoded_and_frames = [
            convert_png(path, args.threshold, args.strict_monochrome)
            for path in sources
        ]
        frames = [frame for _, frame in decoded_and_frames]
        for source, (image, frame) in zip(sources, decoded_and_frames):
            mode = (
                "native grayscale 1-bit"
                if image.is_native_one_bit
                else f"converted color_type={image.color_type} bit_depth={image.bit_depth}"
            )
            print(
                f"{source.name}: {image.width}x{image.height}, {mode}, "
                f"{len(frame)} native bytes",
                file=sys.stderr,
            )

        if args.verify_only:
            return 0

        rendered = render_arrays(sources, frames, args.language, args.array_prefix)
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(rendered, encoding="utf-8", newline="\n")
        else:
            sys.stdout.write(rendered)
        return 0
    except (ConversionError, OSError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
