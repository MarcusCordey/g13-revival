#!/usr/bin/env python3
"""Generate deterministic 160x48 monochrome G13 startup-animation PNGs."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Sequence

from png_to_g13 import write_one_bit_png


WIDTH = 160
HEIGHT = 48


FONT = {
    " ": ("00000",) * 7,
    "!": ("00100", "00100", "00100", "00100", "00100", "00000", "00100"),
    ".": ("00000", "00000", "00000", "00000", "00000", "00110", "00110"),
    "0": ("01110", "10001", "10011", "10101", "11001", "10001", "01110"),
    "1": ("00100", "01100", "00100", "00100", "00100", "00100", "01110"),
    "2": ("01110", "10001", "00001", "00010", "00100", "01000", "11111"),
    "3": ("11110", "00001", "00001", "01110", "00001", "00001", "11110"),
    "4": ("00010", "00110", "01010", "10010", "11111", "00010", "00010"),
    "5": ("11111", "10000", "10000", "11110", "00001", "00001", "11110"),
    "6": ("01110", "10000", "10000", "11110", "10001", "10001", "01110"),
    "7": ("11111", "00001", "00010", "00100", "01000", "01000", "01000"),
    "8": ("01110", "10001", "10001", "01110", "10001", "10001", "01110"),
    "9": ("01110", "10001", "10001", "01111", "00001", "00001", "01110"),
    "A": ("01110", "10001", "10001", "11111", "10001", "10001", "10001"),
    "B": ("11110", "10001", "10001", "11110", "10001", "10001", "11110"),
    "C": ("01111", "10000", "10000", "10000", "10000", "10000", "01111"),
    "D": ("11110", "10001", "10001", "10001", "10001", "10001", "11110"),
    "E": ("11111", "10000", "10000", "11110", "10000", "10000", "11111"),
    "F": ("11111", "10000", "10000", "11110", "10000", "10000", "10000"),
    "G": ("01111", "10000", "10000", "10111", "10001", "10001", "01111"),
    "H": ("10001", "10001", "10001", "11111", "10001", "10001", "10001"),
    "I": ("11111", "00100", "00100", "00100", "00100", "00100", "11111"),
    "J": ("00111", "00010", "00010", "00010", "10010", "10010", "01100"),
    "K": ("10001", "10010", "10100", "11000", "10100", "10010", "10001"),
    "L": ("10000", "10000", "10000", "10000", "10000", "10000", "11111"),
    "M": ("10001", "11011", "10101", "10101", "10001", "10001", "10001"),
    "N": ("10001", "11001", "10101", "10011", "10001", "10001", "10001"),
    "O": ("01110", "10001", "10001", "10001", "10001", "10001", "01110"),
    "P": ("11110", "10001", "10001", "11110", "10000", "10000", "10000"),
    "Q": ("01110", "10001", "10001", "10001", "10101", "10010", "01101"),
    "R": ("11110", "10001", "10001", "11110", "10100", "10010", "10001"),
    "S": ("01111", "10000", "10000", "01110", "00001", "00001", "11110"),
    "T": ("11111", "00100", "00100", "00100", "00100", "00100", "00100"),
    "U": ("10001", "10001", "10001", "10001", "10001", "10001", "01110"),
    "V": ("10001", "10001", "10001", "10001", "10001", "01010", "00100"),
    "W": ("10001", "10001", "10001", "10101", "10101", "10101", "01010"),
    "X": ("10001", "10001", "01010", "00100", "01010", "10001", "10001"),
    "Y": ("10001", "10001", "01010", "00100", "00100", "00100", "00100"),
    "Z": ("11111", "00001", "00010", "00100", "01000", "10000", "11111"),
}

LOWER_FONT = {
    "a": ("00000", "01110", "00001", "01111", "10001", "01111", "00000"),
    "b": ("10000", "10000", "10110", "11001", "10001", "11110", "00000"),
    "d": ("00001", "00001", "01101", "10011", "10001", "01111", "00000"),
    "e": ("00000", "01110", "10001", "11111", "10000", "01111", "00000"),
    "i": ("00100", "00000", "01100", "00100", "00100", "01110", "00000"),
    "n": ("00000", "10110", "11001", "10001", "10001", "10001", "00000"),
    "o": ("00000", "01110", "10001", "10001", "10001", "01110", "00000"),
    "r": ("00000", "10110", "11001", "10000", "10000", "10000", "00000"),
    "s": ("00000", "01111", "10000", "01110", "00001", "11110", "00000"),
    "w": ("00000", "10001", "10001", "10101", "10101", "01010", "00000"),
    "y": ("00000", "10001", "10001", "01111", "00001", "01110", "00000"),
}


@dataclass
class Canvas:
    width: int = WIDTH
    height: int = HEIGHT

    def __post_init__(self) -> None:
        self.pixels = [False] * (self.width * self.height)

    def set(self, x: int, y: int, value: bool = True) -> None:
        if 0 <= x < self.width and 0 <= y < self.height:
            self.pixels[y * self.width + x] = value

    def rect(self, x: int, y: int, width: int, height: int, fill: bool = True) -> None:
        if width <= 0 or height <= 0:
            return
        if fill:
            for py in range(y, y + height):
                for px in range(x, x + width):
                    self.set(px, py)
            return
        for px in range(x, x + width):
            self.set(px, y)
            self.set(px, y + height - 1)
        for py in range(y, y + height):
            self.set(x, py)
            self.set(x + width - 1, py)

    def clear_rect(self, x: int, y: int, width: int, height: int) -> None:
        for py in range(y, y + height):
            for px in range(x, x + width):
                self.set(px, py, False)

    def line(self, x0: int, y0: int, x1: int, y1: int, thickness: int = 1) -> None:
        dx = abs(x1 - x0)
        sx = 1 if x0 < x1 else -1
        dy = -abs(y1 - y0)
        sy = 1 if y0 < y1 else -1
        error = dx + dy
        while True:
            radius = thickness // 2
            self.rect(x0 - radius, y0 - radius, thickness, thickness)
            if x0 == x1 and y0 == y1:
                break
            doubled = 2 * error
            if doubled >= dy:
                error += dy
                x0 += sx
            if doubled <= dx:
                error += dx
                y0 += sy

    def text(self, x: int, y: int, text: str, scale: int = 1) -> None:
        cursor = x
        for character in text.upper():
            glyph = FONT.get(character)
            if glyph is None:
                raise ValueError(f"unsupported font character {character!r}")
            for row, pattern in enumerate(glyph):
                for column, bit in enumerate(pattern):
                    if bit == "1":
                        self.rect(
                            cursor + column * scale,
                            y + row * scale,
                            scale,
                            scale,
                        )
            cursor += 6 * scale

    def centered_text(self, y: int, text: str, scale: int = 1) -> None:
        width = text_width(text, scale)
        self.text((self.width - width) // 2, y, text, scale)

    def text_case(self, x: int, y: int, text: str, scale: int = 1) -> None:
        cursor = x
        for character in text:
            glyph = LOWER_FONT.get(character, FONT.get(character))
            if glyph is None:
                raise ValueError(f"unsupported case-sensitive font character {character!r}")
            for row, pattern in enumerate(glyph):
                for column, bit in enumerate(pattern):
                    if bit == "1":
                        self.rect(
                            cursor + column * scale,
                            y + row * scale,
                            scale,
                            scale,
                        )
            cursor += 6 * scale


def text_width(text: str, scale: int = 1) -> int:
    return max(0, len(text) * 6 * scale - scale)


def draw_border(canvas: Canvas) -> None:
    canvas.line(5, 3, 154, 3, 1)
    canvas.line(5, 44, 154, 44, 1)
    canvas.rect(2, 1, 4, 5)
    canvas.rect(154, 1, 4, 5)
    canvas.rect(2, 42, 4, 5)
    canvas.rect(154, 42, 4, 5)


def draw_chip_body(
    canvas: Canvas,
    left: int,
    top: int,
    width: int = 44,
    height: int = 30,
) -> None:
    canvas.rect(left, top, width, height, fill=False)
    canvas.rect(left + 1, top + 1, width - 2, height - 2, fill=False)

    for y in (top + 5, top + 11, top + 17, top + 23):
        canvas.rect(left - 5, y, 5, 2)
        canvas.rect(left + width, y, 5, 2)
    for x in (left + 6, left + 14, left + 22, left + 30, left + 38):
        canvas.rect(x, top - 4, 2, 4)
        canvas.rect(x, top + height, 2, 4)


def draw_chip_face(
    canvas: Canvas,
    left: int,
    top: int,
    eye_mode: str = "normal",
    look_right: bool = False,
) -> None:
    if eye_mode == "wide":
        eye_width, eye_height = 11, 12
        eye_y = top + 6
        eye_x = (left + 6, left + 27)
        pupil_width, pupil_height = 4, 5
    elif eye_mode == "crazy":
        eye_width, eye_height = 10, 11
        eye_y = top + 6
        eye_x = (left + 7, left + 27)
        pupil_width, pupil_height = 3, 4
    else:
        eye_width, eye_height = 8, 9
        eye_y = top + 7
        eye_x = (left + 8, left + 28)
        pupil_width, pupil_height = 3, 4

    for index, x in enumerate(eye_x):
        canvas.rect(x, eye_y, eye_width, eye_height)
        pupil_x = x + eye_width - pupil_width - 1 if look_right else x + 2
        if eye_mode == "crazy" and index == 1:
            pupil_x = x + eye_width - pupil_width - 1
        canvas.clear_rect(pupil_x, eye_y + 2, pupil_width, pupil_height)

    mouth_y = top + 23
    canvas.line(left + 13, mouth_y, left + 17, mouth_y + 3, 2)
    canvas.line(left + 17, mouth_y + 3, left + 27, mouth_y + 3, 2)
    canvas.line(left + 27, mouth_y + 3, left + 31, mouth_y, 2)


def draw_marie_chip(
    canvas: Canvas,
    left: int,
    top: int,
    eye_mode: str = "normal",
    look_right: bool = False,
) -> None:
    draw_chip_body(canvas, left, top)
    draw_chip_face(canvas, left, top, eye_mode, look_right)


def draw_cup(
    canvas: Canvas,
    x: int,
    y: int,
    steam: bool = True,
    empty: bool = False,
) -> None:
    canvas.line(x, y + 5, x + 24, y + 5, 2)
    canvas.line(x + 1, y + 6, x + 3, y + 18, 2)
    canvas.line(x + 3, y + 18, x + 20, y + 18, 2)
    canvas.line(x + 20, y + 18, x + 23, y + 6, 2)
    canvas.line(x + 24, y + 8, x + 29, y + 8, 2)
    canvas.line(x + 29, y + 8, x + 32, y + 11, 2)
    canvas.line(x + 32, y + 11, x + 29, y + 15, 2)
    canvas.line(x + 29, y + 15, x + 23, y + 15, 2)
    if not empty:
        canvas.line(x + 4, y + 8, x + 20, y + 8, 2)
    if steam:
        for steam_x, offset in ((x + 8, 0), (x + 17, 2)):
            canvas.line(steam_x, y + 2, steam_x - 2, y - 1 + offset, 2)
            canvas.line(steam_x - 2, y - 1 + offset, steam_x + 1, y - 4 + offset, 2)


def draw_ordered_traces(canvas: Canvas, left: int, top: int) -> None:
    for source_y, target_y in ((top + 5, 8), (top + 12, 20), (top + 23, 38)):
        canvas.line(left - 5, source_y, 40, source_y, 2)
        canvas.line(40, source_y, 30, target_y, 2)
        canvas.line(30, target_y, 19, target_y, 2)
        canvas.rect(14, target_y - 2, 5, 5)
        right = left + 49
        canvas.line(right, source_y, 120, source_y, 2)
        canvas.line(120, source_y, 130, target_y, 2)
        canvas.line(130, target_y, 141, target_y, 2)
        canvas.rect(141, target_y - 2, 5, 5)


def frame_marie_inside() -> Canvas:
    canvas = Canvas()
    draw_border(canvas)
    canvas.centered_text(17, "MARIE INSIDE", 2)
    return canvas


def frame_marie_wakes() -> Canvas:
    canvas = Canvas()
    draw_marie_chip(canvas, 58, 9)
    canvas.line(57, 4, 53, 1, 2)
    canvas.line(80, 4, 80, 0, 2)
    canvas.line(103, 4, 107, 1, 2)
    return canvas


def frame_latte_arrives() -> Canvas:
    canvas = Canvas()
    draw_marie_chip(canvas, 24, 9, look_right=True)
    draw_cup(canvas, 119, 21)
    canvas.line(153, 25, 158, 25, 2)
    canvas.line(151, 31, 158, 31, 2)
    canvas.line(153, 37, 158, 37, 2)
    return canvas


def frame_marie_drinks() -> Canvas:
    canvas = Canvas()
    draw_marie_chip(canvas, 24, 9, eye_mode="wide", look_right=True)
    draw_cup(canvas, 111, 21)
    canvas.line(111, 29, 99, 29, 2)
    canvas.line(99, 29, 89, 22, 2)
    canvas.line(89, 22, 73, 22, 2)
    canvas.rect(92, 25, 5, 5)
    canvas.rect(80, 20, 5, 5)
    return canvas


def frame_overclock_chaos() -> Canvas:
    canvas = Canvas()
    draw_marie_chip(canvas, 58, 9, eye_mode="crazy")

    left_paths = (
        ((53, 14), (43, 14), (36, 8), (24, 8)),
        ((53, 24), (42, 24), (33, 31), (19, 31)),
    )
    right_paths = (
        ((107, 17), (118, 17), (126, 10), (140, 10)),
        ((107, 29), (119, 29), (128, 37), (143, 37)),
    )
    for points in left_paths + right_paths:
        for start, end in zip(points, points[1:]):
            canvas.line(*start, *end, 2)

    for points in (
        ((11, 3), (18, 11), (13, 11), (22, 21)),
        ((148, 2), (141, 11), (147, 11), (139, 21)),
        ((8, 28), (16, 34), (11, 35), (20, 45)),
        ((151, 27), (143, 34), (149, 35), (140, 45)),
    ):
        for start, end in zip(points, points[1:]):
            canvas.line(*start, *end, 2)
    return canvas


def frame_latte_overclock() -> Canvas:
    canvas = Canvas()
    canvas.centered_text(3, "LATTE", 2)
    canvas.centered_text(25, "OVERCLOCK!", 2)
    canvas.line(17, 3, 11, 11, 2)
    canvas.line(11, 11, 17, 11, 2)
    canvas.line(17, 11, 10, 21, 2)
    canvas.line(143, 3, 149, 11, 2)
    canvas.line(149, 11, 143, 11, 2)
    canvas.line(143, 11, 150, 21, 2)
    return canvas


def frame_marie_stable() -> Canvas:
    canvas = Canvas()
    draw_ordered_traces(canvas, 58, 9)
    draw_marie_chip(canvas, 58, 9)
    return canvas


def frame_ready_for_azeroth() -> Canvas:
    canvas = Canvas()
    canvas.centered_text(6, "READY FOR", 2)
    canvas.centered_text(26, "AZEROTH", 2)
    canvas.line(20, 22, 140, 22, 1)
    canvas.rect(14, 20, 5, 5)
    canvas.rect(141, 20, 5, 5)
    return canvas


def draw_superscript_two(canvas: Canvas, x: int, y: int, scale: int = 2) -> None:
    glyph = ("110", "001", "010", "100", "111")
    for row, pattern in enumerate(glyph):
        for column, bit in enumerate(pattern):
            if bit == "1":
                canvas.rect(x + column * scale, y + row * scale, scale, scale)


def permanent_signature() -> Canvas:
    canvas = Canvas()

    # Original clipped-corner technology badge, deliberately not an oval or
    # swoosh associated with an existing "inside" brand.
    canvas.line(8, 2, 68, 2, 2)
    canvas.line(68, 2, 76, 10, 2)
    canvas.line(76, 10, 76, 39, 2)
    canvas.line(76, 39, 69, 46, 2)
    canvas.line(69, 46, 8, 46, 2)
    canvas.line(8, 46, 2, 40, 2)
    canvas.line(2, 40, 2, 9, 2)
    canvas.line(2, 9, 8, 2, 2)

    canvas.text(22, 7, "M", 4)
    draw_superscript_two(canvas, 45, 4, 2)
    canvas.text_case(20, 37, "inside", 1)

    canvas.text_case(93, 8, "Powered by", 1)
    canvas.text_case(93, 25, "Marie", 2)
    return canvas


@dataclass(frozen=True)
class FrameSpec:
    filename: str
    label: str
    builder: Callable[[], Canvas]


ANIMATION_FRAME_SPECS = (
    FrameSpec("frame_01_marie_inside.png", "01 MARIE INSIDE", frame_marie_inside),
    FrameSpec("frame_02_marie_wakes.png", "02 MARIE WAKES", frame_marie_wakes),
    FrameSpec("frame_03_latte_arrives.png", "03 LATTE ARRIVES", frame_latte_arrives),
    FrameSpec("frame_04_marie_drinks.png", "04 MARIE DRINKS", frame_marie_drinks),
    FrameSpec("frame_05_overclock_chaos.png", "05 OVERCLOCK CHAOS", frame_overclock_chaos),
    FrameSpec("frame_06_latte_overclock.png", "06 LATTE OVERCLOCK", frame_latte_overclock),
    FrameSpec("frame_07_marie_stable.png", "07 MARIE STABLE", frame_marie_stable),
    FrameSpec(
        "frame_08_ready_for_azeroth.png",
        "08 READY FOR AZEROTH",
        frame_ready_for_azeroth,
    ),
)

PERMANENT_FRAME_SPEC = FrameSpec(
    "permanent_m2_inside_powered_by_marie.png",
    "PERMANENT SIGNATURE",
    permanent_signature,
)


def scale_canvas(source: Canvas, scale: int) -> Canvas:
    scaled = Canvas(source.width * scale, source.height * scale)
    for y in range(source.height):
        for x in range(source.width):
            if source.pixels[y * source.width + x]:
                scaled.rect(x * scale, y * scale, scale, scale)
    return scaled


def build_contact_sheet(
    frames: Sequence[tuple[FrameSpec, Canvas]],
    scale: int = 4,
) -> Canvas:
    columns = 3
    rows = (len(frames) + columns - 1) // columns
    panel_width = WIDTH * scale
    panel_height = HEIGHT * scale
    label_height = 24
    padding = 16
    sheet = Canvas(
        width=columns * panel_width + (columns + 1) * padding,
        height=rows * (label_height + panel_height) + (rows + 1) * padding,
    )

    for index, (spec, frame) in enumerate(frames):
        column = index % columns
        row = index // columns
        origin_x = padding + column * (panel_width + padding)
        origin_y = padding + row * (label_height + panel_height + padding)
        sheet.text(origin_x, origin_y, spec.label, 2)
        image_y = origin_y + label_height
        sheet.rect(origin_x - 2, image_y - 2, panel_width + 4, panel_height + 4, False)
        for y in range(HEIGHT):
            for x in range(WIDTH):
                if frame.pixels[y * WIDTH + x]:
                    sheet.rect(
                        origin_x + x * scale,
                        image_y + y * scale,
                        scale,
                        scale,
                    )
    return sheet


def _parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate the deterministic G13 startup-animation source PNGs."
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("assets/startup-animation/frames"),
        help="frame output directory",
    )
    parser.add_argument(
        "--preview-dir",
        type=Path,
        default=Path("assets/startup-animation/preview/frames"),
        help="individual integer-scaled preview directory",
    )
    parser.add_argument(
        "--contact-sheet",
        type=Path,
        default=Path(
            "assets/startup-animation/preview/"
            "startup_animation_contact_sheet_4x.png"
        ),
        help="contact-sheet output path",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = _parse_args(argv if argv is not None else [])
    frames: list[tuple[FrameSpec, Canvas]] = []
    for spec in ANIMATION_FRAME_SPECS + (PERMANENT_FRAME_SPEC,):
        canvas = spec.builder()
        destination = args.output_dir / spec.filename
        write_one_bit_png(destination, WIDTH, HEIGHT, canvas.pixels)
        preview = scale_canvas(canvas, 4)
        preview_destination = args.preview_dir / f"{Path(spec.filename).stem}_4x.png"
        write_one_bit_png(
            preview_destination,
            preview.width,
            preview.height,
            preview.pixels,
        )
        frames.append((spec, canvas))
        print(destination)
        print(preview_destination)

    contact = build_contact_sheet(frames)
    write_one_bit_png(
        args.contact_sheet,
        contact.width,
        contact.height,
        contact.pixels,
    )
    print(args.contact_sheet)
    return 0


if __name__ == "__main__":
    import sys

    raise SystemExit(main(sys.argv[1:]))
