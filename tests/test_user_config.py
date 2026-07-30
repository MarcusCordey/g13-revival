#!/usr/bin/env python3
"""Host-side compile tests for the public G13 user configuration."""

from __future__ import annotations

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
from typing import Iterable


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
FIRMWARE_DIR = REPOSITORY_ROOT / "firmware" / "g13_marie_v1_0_0"


class UserConfigCompileTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.cxx = shutil.which("c++")
        if cls.cxx is None:
            raise unittest.SkipTest("a host C++ compiler is required")

        cls.stub_directory = tempfile.TemporaryDirectory(
            prefix="g13-arduino-stub-"
        )
        stub_path = Path(cls.stub_directory.name) / "Arduino.h"
        stub_path.write_text(
            """\
#pragma once

#include <stdint.h>

#define KEY_A     (4 | 0xF000)
#define KEY_D     (7 | 0xF000)
#define KEY_K     (14 | 0xF000)
#define KEY_L     (15 | 0xF000)
#define KEY_M     (16 | 0xF000)
#define KEY_N     (17 | 0xF000)
#define KEY_O     (18 | 0xF000)
#define KEY_P     (19 | 0xF000)
#define KEY_Q     (20 | 0xF000)
#define KEY_R     (21 | 0xF000)
#define KEY_S     (22 | 0xF000)
#define KEY_T     (23 | 0xF000)
#define KEY_U     (24 | 0xF000)
#define KEY_W     (26 | 0xF000)
#define KEY_1     (30 | 0xF000)
#define KEY_2     (31 | 0xF000)
#define KEY_3     (32 | 0xF000)
#define KEY_4     (33 | 0xF000)
#define KEY_5     (34 | 0xF000)
#define KEY_6     (35 | 0xF000)
#define KEY_7     (36 | 0xF000)
#define KEY_TAB   (43 | 0xF000)
#define KEY_SPACE (44 | 0xF000)
#define KEY_F1    (58 | 0xF000)
#define KEY_F24   (115 | 0xF000)
""",
            encoding="utf-8",
        )

    @classmethod
    def tearDownClass(cls) -> None:
        cls.stub_directory.cleanup()

    def compile_config(
        self,
        *,
        definitions: Iterable[str] = (),
        assertions: Iterable[str] = (),
    ) -> subprocess.CompletedProcess[str]:
        source_lines = ['#include "G13Config.h"', ""]
        source_lines.extend(
            f'static_assert({expression}, "{expression}");'
            for expression in assertions
        )
        source_lines.extend(("", "int main() { return 0; }", ""))

        command = [
            self.cxx,
            "-x",
            "c++",
            "-std=c++17",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsyntax-only",
            f"-I{self.stub_directory.name}",
            f"-I{FIRMWARE_DIR}",
        ]
        command.extend(f"-D{definition}" for definition in definitions)
        command.append("-")

        return subprocess.run(
            command,
            input="\n".join(source_lines),
            capture_output=True,
            check=False,
            text=True,
        )

    def assert_compiles(
        self,
        *,
        definitions: Iterable[str] = (),
        assertions: Iterable[str] = (),
    ) -> None:
        result = self.compile_config(
            definitions=definitions,
            assertions=assertions,
        )
        if result.returncode != 0:
            self.fail(
                "configuration probe did not compile:\n"
                f"{result.stdout}{result.stderr}"
            )

    def test_defaults_preserve_v1_1_keymap_and_presentation(self) -> None:
        self.assert_compiles(
            assertions=(
                "G13_KEY_G1 == KEY_1",
                "G13_KEY_G2 == KEY_2",
                "G13_KEY_G3 == KEY_3",
                "G13_KEY_G4 == KEY_4",
                "G13_KEY_G5 == KEY_5",
                "G13_KEY_G6 == KEY_6",
                "G13_KEY_G7 == KEY_7",
                "G13_KEY_G8 == KEY_K",
                "G13_KEY_G9 == KEY_L",
                "G13_KEY_G10 == KEY_A",
                "G13_KEY_G11 == KEY_W",
                "G13_KEY_G12 == KEY_D",
                "G13_KEY_G13 == KEY_M",
                "G13_KEY_G14 == KEY_N",
                "G13_KEY_G15 == KEY_O",
                "G13_KEY_G16 == KEY_P",
                "G13_KEY_G17 == KEY_S",
                "G13_KEY_G18 == KEY_Q",
                "G13_KEY_G19 == KEY_R",
                "G13_KEY_G20 == KEY_SPACE",
                "G13_KEY_G21 == KEY_U",
                "G13_KEY_G22 == KEY_T",
                "G13_BACKLIGHT_RED == 0",
                "G13_BACKLIGHT_GREEN == 0",
                "G13_BACKLIGHT_BLUE == 255",
                "G13_LCD_THEME == G13_LCD_THEME_MARIE_LATTE",
                "G13_LCD_ANIMATION_ENABLE == 1",
                "G13_LCD_PERMANENT_FRAME_ENABLE == 1",
                "G13_LCD_ANIMATION_REPEAT == 0",
                "G13_LCD_STATIC_FALLBACK_ENABLE == 1",
                "G13_LCD_ANIMATION_FRAME_MS == 700",
                "G13_LATTE_OVERCLOCK_MS == 1200",
                "G13_READY_HOLD_MS == 2000",
            )
        )

    def test_theme_selection_derives_one_unambiguous_startup_path(self) -> None:
        cases = (
            (
                "marie_latte",
                "G13_LCD_THEME_MARIE_LATTE",
                (
                    "G13_INTERNAL_LCD_ANIMATION_ENABLE == 1",
                    "G13_INTERNAL_LCD_PERMANENT_FRAME_ENABLE == 1",
                    "G13_INTERNAL_LCD_STATIC_FALLBACK_ENABLE == 0",
                ),
            ),
            (
                "static",
                "G13_LCD_THEME_STATIC",
                (
                    "G13_INTERNAL_LCD_ANIMATION_ENABLE == 0",
                    "G13_INTERNAL_LCD_PERMANENT_FRAME_ENABLE == 0",
                    "G13_INTERNAL_LCD_STATIC_FALLBACK_ENABLE == 1",
                ),
            ),
            (
                "none",
                "G13_LCD_THEME_NONE",
                (
                    "G13_INTERNAL_LCD_ANIMATION_ENABLE == 0",
                    "G13_INTERNAL_LCD_PERMANENT_FRAME_ENABLE == 0",
                    "G13_INTERNAL_LCD_STATIC_FALLBACK_ENABLE == 0",
                ),
            ),
        )

        for name, theme, assertions in cases:
            with self.subTest(theme=name):
                self.assert_compiles(
                    definitions=(f"G13_LCD_THEME={theme}",),
                    assertions=assertions,
                )

    def test_valid_overrides_compile_at_documented_boundaries(self) -> None:
        definitions = (
            "G13_KEY_G1=KEY_F1",
            "G13_KEY_G2=KEY_F24",
            "G13_KEY_G22=KEY_TAB",
            "G13_BACKLIGHT_RED=12",
            "G13_BACKLIGHT_GREEN=34",
            "G13_BACKLIGHT_BLUE=56",
            "G13_LCD_ANIMATION_ENABLE=0",
            "G13_LCD_PERMANENT_FRAME_ENABLE=0",
            "G13_LCD_ANIMATION_REPEAT=1",
            "G13_LCD_STATIC_FALLBACK_ENABLE=0",
            "G13_LCD_ANIMATION_FRAME_MS=1",
            "G13_LATTE_OVERCLOCK_MS=60000",
            "G13_READY_HOLD_MS=0",
        )
        assertions = (
            "G13_KEY_G1 == KEY_F1",
            "G13_KEY_G2 == KEY_F24",
            "G13_KEY_G22 == KEY_TAB",
            "G13_BACKLIGHT_RED == 12",
            "G13_BACKLIGHT_GREEN == 34",
            "G13_BACKLIGHT_BLUE == 56",
            "G13_LCD_ANIMATION_ENABLE == 0",
            "G13_LCD_PERMANENT_FRAME_ENABLE == 0",
            "G13_LCD_ANIMATION_REPEAT == 1",
            "G13_LCD_STATIC_FALLBACK_ENABLE == 0",
            "G13_LCD_ANIMATION_FRAME_MS == 1",
            "G13_LATTE_OVERCLOCK_MS == 60000",
            "G13_READY_HOLD_MS == 0",
        )
        self.assert_compiles(definitions=definitions, assertions=assertions)

    def test_invalid_values_report_the_targeted_configuration_error(self) -> None:
        cases = (
            (
                "rgb",
                "G13_BACKLIGHT_RED=256",
                "G13_BACKLIGHT_RED must be between 0 and 255",
            ),
            (
                "theme",
                "G13_LCD_THEME=99",
                "G13_LCD_THEME must be G13_LCD_THEME_MARIE_LATTE, "
                "G13_LCD_THEME_STATIC or G13_LCD_THEME_NONE",
            ),
            (
                "boolean",
                "G13_LCD_ANIMATION_ENABLE=2",
                "G13_LCD_ANIMATION_ENABLE must be 0 or 1",
            ),
            (
                "timing",
                "G13_READY_HOLD_MS=60001",
                "G13_READY_HOLD_MS must be between 0 and 60000 milliseconds",
            ),
            (
                "misspelled_key",
                "G13_KEY_G1=KEY_SPCAE",
                "G13_KEY_G1 must be a supported normal-key Teensy "
                "KEY_* constant",
            ),
            (
                "character_literal",
                "G13_KEY_G1='w'",
                "G13_KEY_G1 must be a supported normal-key Teensy "
                "KEY_* constant",
            ),
            (
                "unsupported_usage_gap",
                "G13_KEY_G1=0xF066",
                "G13_KEY_G1 must be a supported normal-key Teensy "
                "KEY_* constant",
            ),
        )

        for name, definition, expected_error in cases:
            with self.subTest(value=name):
                result = self.compile_config(definitions=(definition,))
                diagnostics = f"{result.stdout}{result.stderr}"
                self.assertNotEqual(
                    result.returncode,
                    0,
                    msg=f"invalid configuration unexpectedly compiled:\n{diagnostics}",
                )
                self.assertIn(expected_error, diagnostics)


if __name__ == "__main__":
    unittest.main()
