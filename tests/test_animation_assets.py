#!/usr/bin/env python3

from __future__ import annotations

import contextlib
import hashlib
import io
import re
import sys
import tempfile
import unittest
from pathlib import Path


REPOSITORY = Path(__file__).resolve().parents[1]
TOOLS = REPOSITORY / "tools"
sys.path.insert(0, str(TOOLS))

from png_to_g13 import (  # noqa: E402
    ConversionError,
    DecodedPng,
    G13_NATIVE_FRAME_BYTES,
    G13_NATIVE_HEIGHT,
    G13_LAST_PAGE_PADDING_MASK,
    G13_VISIBLE_HEIGHT,
    G13_VISIBLE_WIDTH,
    convert_png,
    monochrome_to_g13,
    png_to_monochrome,
    read_png,
    render_arrays,
    write_one_bit_png,
)
from generate_startup_animation import main as generate_animation  # noqa: E402


FRAME_DIRECTORY = REPOSITORY / "assets/startup-animation/frames"
PREVIEW_DIRECTORY = REPOSITORY / "assets/startup-animation/preview/frames"
CONTACT_SHEET = (
    REPOSITORY
    / "assets/startup-animation/preview/startup_animation_contact_sheet_4x.png"
)
ANIMATION_FILENAMES = (
    "frame_01_marie_inside.png",
    "frame_02_marie_wakes.png",
    "frame_03_latte_arrives.png",
    "frame_04_marie_drinks.png",
    "frame_05_overclock_chaos.png",
    "frame_06_latte_overclock.png",
    "frame_07_marie_stable.png",
    "frame_08_ready_for_azeroth.png",
)
PERMANENT_FILENAME = "permanent_m2_inside_powered_by_marie.png"
HEADER = (
    REPOSITORY
    / "firmware/g13_marie_v1_0_0/G13StartupAnimationFrames.h"
)


class AnimationAssetTests(unittest.TestCase):
    def frame_paths(self) -> list[Path]:
        return [
            FRAME_DIRECTORY / filename
            for filename in ANIMATION_FILENAMES + (PERMANENT_FILENAME,)
        ]

    def test_all_sources_are_visible_one_bit_160x43(self) -> None:
        paths = self.frame_paths()
        self.assertEqual(len(paths), 9)
        self.assertEqual(sorted(FRAME_DIRECTORY.glob("*.png")), sorted(paths))
        native_hashes: set[str] = set()
        for path in paths:
            image, frame = convert_png(path, strict_monochrome=True)
            self.assertEqual(
                (image.width, image.height),
                (G13_VISIBLE_WIDTH, G13_VISIBLE_HEIGHT),
            )
            self.assertTrue(image.is_one_bit_grayscale)
            self.assertEqual(len(frame), G13_NATIVE_FRAME_BYTES)
            native_hashes.add(hashlib.sha256(frame).hexdigest())
        self.assertEqual(len(native_hashes), 9)

    def test_visible_y42_maps_to_bit_two_of_the_last_native_page(self) -> None:
        pixels = [False] * (G13_VISIBLE_WIDTH * G13_VISIBLE_HEIGHT)
        known_pixels = ((0, 0), (0, 7), (0, 8), (159, 42))
        for x, y in known_pixels:
            pixels[y * G13_VISIBLE_WIDTH + x] = True

        frame = monochrome_to_g13(pixels)
        self.assertEqual(frame[0], 0x81)
        self.assertEqual(frame[160], 0x01)
        self.assertEqual(frame[959], 0x04)
        self.assertEqual(sum(value.bit_count() for value in frame), 4)

    def test_all_native_frames_keep_last_page_padding_bits_clear(self) -> None:
        for path in self.frame_paths():
            _, frame = convert_png(path, strict_monochrome=True)
            self.assertEqual(len(frame), G13_NATIVE_FRAME_BYTES)
            for value in frame[-G13_VISIBLE_WIDTH:]:
                self.assertEqual(value & G13_LAST_PAGE_PADDING_MASK, 0)

    def test_full_visible_canvas_fills_only_visible_native_bits(self) -> None:
        pixels = [True] * (G13_VISIBLE_WIDTH * G13_VISIBLE_HEIGHT)
        frame = monochrome_to_g13(pixels)
        self.assertEqual(frame[:5 * G13_VISIBLE_WIDTH], b"\xFF" * 800)
        self.assertEqual(frame[5 * G13_VISIBLE_WIDTH:], b"\x07" * 160)

    def test_checked_in_motifs_keep_visible_bottom_safety_row_clear(self) -> None:
        for path in self.frame_paths():
            image = read_png(path)
            pixels = png_to_monochrome(image, strict_monochrome=True)
            safety_row = pixels[
                (G13_VISIBLE_HEIGHT - 1) * G13_VISIBLE_WIDTH:
                G13_VISIBLE_HEIGHT * G13_VISIBLE_WIDTH
            ]
            self.assertFalse(any(safety_row), path.name)

    def test_one_bit_png_round_trip(self) -> None:
        pixels = [
            ((x + y) % 11 == 0)
            for y in range(G13_VISIBLE_HEIGHT)
            for x in range(G13_VISIBLE_WIDTH)
        ]
        expected = monochrome_to_g13(pixels)
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "roundtrip.png"
            write_one_bit_png(
                path,
                G13_VISIBLE_WIDTH,
                G13_VISIBLE_HEIGHT,
                pixels,
            )
            image, actual = convert_png(path, strict_monochrome=True)
            self.assertTrue(image.is_one_bit_grayscale)
            self.assertEqual(actual, expected)

    def test_visible_source_coordinates_at_y43_are_rejected(self) -> None:
        height = G13_VISIBLE_HEIGHT + 1
        pixels = [False] * (G13_VISIBLE_WIDTH * height)
        pixels[G13_VISIBLE_HEIGHT * G13_VISIBLE_WIDTH] = True
        with self.assertRaisesRegex(
            ConversionError,
            r"160x43 visible pixels.*160x48 with empty padding",
        ):
            monochrome_to_g13(pixels, G13_VISIBLE_WIDTH, height)

    def test_160x48_compatibility_requires_empty_padding_rows(self) -> None:
        visible_pixels = [False] * (
            G13_VISIBLE_WIDTH * G13_VISIBLE_HEIGHT
        )
        visible_pixels[42 * G13_VISIBLE_WIDTH + 19] = True
        expected = monochrome_to_g13(visible_pixels)

        compatibility_pixels = [False] * (
            G13_VISIBLE_WIDTH * G13_NATIVE_HEIGHT
        )
        compatibility_pixels[42 * G13_VISIBLE_WIDTH + 19] = True
        self.assertEqual(
            monochrome_to_g13(
                compatibility_pixels,
                G13_VISIBLE_WIDTH,
                G13_NATIVE_HEIGHT,
            ),
            expected,
        )

        compatibility_pixels[43 * G13_VISIBLE_WIDTH + 23] = True
        with self.assertRaisesRegex(
            ConversionError,
            r"padding pixel \(23,43\) is set; rows 43 through 47 must be empty",
        ):
            monochrome_to_g13(
                compatibility_pixels,
                G13_VISIBLE_WIDTH,
                G13_NATIVE_HEIGHT,
            )

    def test_160x48_compatibility_png_requires_empty_padding_rows(self) -> None:
        pixels = [False] * (G13_VISIBLE_WIDTH * G13_NATIVE_HEIGHT)
        pixels[42 * G13_VISIBLE_WIDTH] = True
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "compatibility.png"
            write_one_bit_png(
                path,
                G13_VISIBLE_WIDTH,
                G13_NATIVE_HEIGHT,
                pixels,
            )
            _, frame = convert_png(path, strict_monochrome=True)
            self.assertEqual(frame[5 * G13_VISIBLE_WIDTH], 0x04)

            pixels[43 * G13_VISIBLE_WIDTH] = True
            write_one_bit_png(
                path,
                G13_VISIBLE_WIDTH,
                G13_NATIVE_HEIGHT,
                pixels,
            )
            with self.assertRaisesRegex(
                ConversionError,
                r"padding pixel \(0,43\) is set",
            ):
                convert_png(path, strict_monochrome=True)

    def test_invalid_dimensions_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "wrong-size.png"
            write_one_bit_png(
                path,
                159,
                G13_VISIBLE_HEIGHT,
                [False] * (159 * G13_VISIBLE_HEIGHT),
            )
            with self.assertRaisesRegex(
                ConversionError,
                r"exactly 160x43 visible pixels",
            ):
                convert_png(path)

    def test_non_monochrome_pixels_are_thresholded_to_one_bit(self) -> None:
        image = DecodedPng(
            width=4,
            height=1,
            bit_depth=8,
            color_type=2,
            rgba=bytes(
                (
                    0, 0, 0, 255,
                    127, 127, 127, 255,
                    128, 128, 128, 255,
                    255, 255, 255, 255,
                )
            ),
        )
        self.assertFalse(image.is_one_bit_grayscale)
        self.assertEqual(
            png_to_monochrome(image, threshold=128),
            (False, False, True, True),
        )

    def test_generated_header_matches_png_order_and_bytes(self) -> None:
        paths = self.frame_paths()
        frames = [convert_png(path, strict_monochrome=True)[1] for path in paths]
        expected = render_arrays(paths, frames, "cpp", "g13_startup_animation")
        self.assertEqual(HEADER.read_text(encoding="utf-8"), expected)
        self.assertIn(
            "g13_startup_animation_visible_width = 160;",
            expected,
        )
        self.assertIn(
            "g13_startup_animation_visible_height = 43;",
            expected,
        )
        self.assertIn(
            "g13_startup_animation_native_height = 48;",
            expected,
        )
        self.assertIn(
            "g13_startup_animation_frame_bytes = 960;",
            expected,
        )

        arrays = re.findall(
            r"alignas\(4\) static const uint8_t .*?\[960\] "
            r"G13_FRAME_STORAGE = \{(.*?)\};",
            expected,
            re.DOTALL,
        )
        self.assertEqual(len(arrays), 9)
        for body in arrays:
            self.assertEqual(len(re.findall(r"0x[0-9A-F]{2}", body)), 960)

    def test_array_renderer_rejects_wrong_size_or_padding_bits(self) -> None:
        source = [Path("frame.png")]
        for size in (G13_NATIVE_FRAME_BYTES - 1, G13_NATIVE_FRAME_BYTES + 1):
            with self.subTest(size=size), self.assertRaisesRegex(
                ConversionError,
                rf"native frame 1 has {size} bytes; expected 960",
            ):
                render_arrays(source, [bytes(size)], "cpp", "test_frame")

        invalid_padding = bytearray(G13_NATIVE_FRAME_BYTES)
        invalid_padding[-G13_VISIBLE_WIDTH] = 0x08
        with self.assertRaisesRegex(
            ConversionError,
            r"native frame 1 has non-zero padding bits in rows 43 through 47",
        ):
            render_arrays(
                source,
                [bytes(invalid_padding)],
                "cpp",
                "test_frame",
            )

    def test_contact_sheet_is_one_bit_and_integer_scaled(self) -> None:
        image = read_png(CONTACT_SHEET)
        self.assertTrue(image.is_one_bit_grayscale)
        self.assertEqual(image.width, 1984)
        self.assertEqual(image.height, 652)

    def test_individual_previews_are_exact_four_times_scales(self) -> None:
        expected_preview_paths = []
        for source_path in self.frame_paths():
            preview_path = PREVIEW_DIRECTORY / f"{source_path.stem}_4x.png"
            expected_preview_paths.append(preview_path)
            source = png_to_monochrome(
                read_png(source_path),
                strict_monochrome=True,
            )
            preview_image = read_png(preview_path)
            self.assertTrue(preview_image.is_one_bit_grayscale)
            self.assertEqual(
                (preview_image.width, preview_image.height),
                (G13_VISIBLE_WIDTH * 4, G13_VISIBLE_HEIGHT * 4),
            )
            preview = png_to_monochrome(
                preview_image,
                strict_monochrome=True,
            )
            expected = tuple(
                source[(y // 4) * G13_VISIBLE_WIDTH + (x // 4)]
                for y in range(G13_VISIBLE_HEIGHT * 4)
                for x in range(G13_VISIBLE_WIDTH * 4)
            )
            self.assertEqual(preview, expected)

        self.assertEqual(
            sorted(PREVIEW_DIRECTORY.glob("*.png")),
            sorted(expected_preview_paths),
        )

    def test_generator_reproduces_all_checked_in_assets(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            output_directory = root / "frames"
            preview_directory = root / "preview"
            contact_sheet = root / "contact.png"
            with contextlib.redirect_stdout(io.StringIO()):
                result = generate_animation(
                    [
                        "--output-dir",
                        str(output_directory),
                        "--preview-dir",
                        str(preview_directory),
                        "--contact-sheet",
                        str(contact_sheet),
                    ]
                )
            self.assertEqual(result, 0)

            for source_path in self.frame_paths():
                self.assertEqual(
                    (output_directory / source_path.name).read_bytes(),
                    source_path.read_bytes(),
                )
                preview_name = f"{source_path.stem}_4x.png"
                self.assertEqual(
                    (preview_directory / preview_name).read_bytes(),
                    (PREVIEW_DIRECTORY / preview_name).read_bytes(),
                )
            self.assertEqual(contact_sheet.read_bytes(), CONTACT_SHEET.read_bytes())

    def test_c_and_cpp_output_are_reproducible(self) -> None:
        paths = self.frame_paths()[:2]
        frames = [convert_png(path, strict_monochrome=True)[1] for path in paths]
        for language in ("c", "cpp"):
            first = render_arrays(paths, frames, language, "test_frames")
            second = render_arrays(paths, frames, language, "test_frames")
            self.assertEqual(first, second)


if __name__ == "__main__":
    unittest.main()
