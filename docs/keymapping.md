# Key Mapping

Version `v1.1.0` uses a fixed mapping compiled into the firmware.

| G13 key | Emitted key | Status in source |
|---|---|---|
| G1 | `1` | Established mapping |
| G2 | `2` | Established mapping |
| G3 | `3` | Established mapping |
| G4 | `4` | Established mapping |
| G5 | `5` | Established mapping |
| G6 | `6` | Established mapping |
| G7 | `7` | Established mapping |
| G8 | `k` | Test mapping |
| G9 | `l` | Test mapping |
| G10 | `a` | Established mapping |
| G11 | `w` | Established mapping |
| G12 | `d` | Established mapping |
| G13 | `m` | Test mapping |
| G14 | `n` | Test mapping |
| G15 | `o` | Test mapping |
| G16 | `p` | Test mapping |
| G17 | `s` | Established mapping |
| G18 | `q` | Test mapping |
| G19 | `r` | Test mapping |
| G20 | Space | Established mapping |
| G21 | `u` | Test mapping |
| G22 | `t` | Established mapping |

The verified board configuration uses the **German (Mac)** keyboard layout.
Choosing another Teensy keyboard layout can change the characters produced by
the same key codes.

The documented Teensy USB keyboard profile provides six simultaneous ordinary
key slots. If more than six mapped G-keys are held, the additional G-key remains
pending and is emitted when one of the six active slots becomes free. Releasing
the G13 connection releases all represented keys and clears the pending state.

G1 through G22, simultaneous forward/turn and forward/jump combinations, and
release after a physical G13 disconnect were confirmed on hardware for
`v1.1.0`. See [`hardware-validation.md`](hardware-validation.md) for the exact
test boundary.

The joystick, M1/M2/M3, MR and the four buttons below the LCD are not mapped in
this firmware version.
