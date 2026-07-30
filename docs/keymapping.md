# Key Mapping

Version `v1.2.0` keeps the established default mapping but makes every G-key
individually configurable in
[`G13UserConfig.h`](../firmware/g13_marie_v1_0_0/G13UserConfig.h). Normal users
do not need to edit the sketch or internal firmware headers.

| G13 key | User-config macro | Default Teensy keycode | Default result | Status of default |
|---|---|---|---|---|
| G1 | `G13_KEY_G1` | `KEY_1` | `1` | Established mapping |
| G2 | `G13_KEY_G2` | `KEY_2` | `2` | Established mapping |
| G3 | `G13_KEY_G3` | `KEY_3` | `3` | Established mapping |
| G4 | `G13_KEY_G4` | `KEY_4` | `4` | Established mapping |
| G5 | `G13_KEY_G5` | `KEY_5` | `5` | Established mapping |
| G6 | `G13_KEY_G6` | `KEY_6` | `6` | Established mapping |
| G7 | `G13_KEY_G7` | `KEY_7` | `7` | Established mapping |
| G8 | `G13_KEY_G8` | `KEY_K` | `k` | Test mapping |
| G9 | `G13_KEY_G9` | `KEY_L` | `l` | Test mapping |
| G10 | `G13_KEY_G10` | `KEY_A` | `a` | Established mapping |
| G11 | `G13_KEY_G11` | `KEY_W` | `w` | Established mapping |
| G12 | `G13_KEY_G12` | `KEY_D` | `d` | Established mapping |
| G13 | `G13_KEY_G13` | `KEY_M` | `m` | Test mapping |
| G14 | `G13_KEY_G14` | `KEY_N` | `n` | Test mapping |
| G15 | `G13_KEY_G15` | `KEY_O` | `o` | Test mapping |
| G16 | `G13_KEY_G16` | `KEY_P` | `p` | Test mapping |
| G17 | `G13_KEY_G17` | `KEY_S` | `s` | Established mapping |
| G18 | `G13_KEY_G18` | `KEY_Q` | `q` | Test mapping |
| G19 | `G13_KEY_G19` | `KEY_R` | `r` | Test mapping |
| G20 | `G13_KEY_G20` | `KEY_SPACE` | Space | Established mapping |
| G21 | `G13_KEY_G21` | `KEY_U` | `u` | Test mapping |
| G22 | `G13_KEY_G22` | `KEY_T` | `t` | Established mapping |

The documented board configuration uses the **German (Mac)** keyboard layout.
The `KEY_*` values are direct Teensy USB-HID key positions. The operating
system's active keyboard layout interprets those positions, so a different
layout can produce a different character, particularly for layout-dependent
positions such as Y and Z. The table above describes the result with the
documented German (Mac) setup.

Use supported Teensy `KEY_*` constants consistently. Do not mix quoted ASCII
character literals with `KEY_*` values in this mapping. Supported normal keys
are `KEY_A` through `KEY_MENU` and `KEY_F13` through `KEY_F24`; modifier-only,
media and system codes are intentionally rejected. Examples, validation details
and the complete edit/compile/upload procedure are in
[User configuration](user-configuration.md).

The documented Teensy USB keyboard profile provides six simultaneous ordinary
key slots. If more than six mapped G-keys are held, the additional G-key remains
pending and is emitted when one of the six active slots becomes free. Releasing
the G13 connection releases all represented keys and clears the pending state.
Mapping two G-keys to the same `KEY_*` value is supported; the firmware keeps
that keyboard key in one represented slot and keeps it pressed until both G-keys
have been released.

G1 through G22, simultaneous forward/turn and forward/jump combinations, and
release after a physical G13 disconnect were confirmed on hardware for the
`v1.1.0` default mapping. Automated `v1.2.0` tests confirm that its defaults are
the same, but `v1.2.0` and custom mappings were not physically tested in this
development step. See [Hardware validation](hardware-validation.md).

The joystick, M1/M2/M3, MR and the four buttons below the LCD are not mapped in
this firmware version.
