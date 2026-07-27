/*
==============================================================================
Teensy 4.1 Logitech G13 Adapter Project

by Marcus Cordey

Feel free to use, redistribute and/or edit.

This project contains reverse-engineered Logitech G13 protocol handling,
USB communication, key mapping and LCD support for Teensy 4.1.

The code is intentionally documented to help other developers understand
the protocol and implementation details.

No warranty is provided. Use at your own risk.
==============================================================================
*/

#include <USBHost_t36.h>
#include "G13Config.h"
#include "HIDDumper.h"
#include "USBDeviceInfo.h"
#include "G13Display.h"
#include <Keyboard.h>
#include <string.h>   // für memset, memcpy

const int STATUS_LED_PIN = LED_BUILTIN;

// -----------------------------------------------------------------------------
// SystemStatus
// Purpose:
// Tracks the coarse runtime state used by the onboard status LED.
//
// Details:
// STATUS_BOOTING uses a faster LED blink while USB host enumeration settles.
// STATUS_RUNNING indicates that the normal USB host / HID bridge loop is active.
// -----------------------------------------------------------------------------
enum SystemStatus {
  STATUS_BOOTING,
  STATUS_RUNNING
};

// Global runtime state for the LED heartbeat. It does not affect HID decoding.
SystemStatus systemStatus = STATUS_BOOTING;

// ======================================================
// Monitoring (NEU) - nur wenn HID-Reports ausbleiben
// ======================================================
// Timestamp of the last valid incoming HID report from the G13.
// This is a reverse-engineering aid and a runtime health indicator: as long as
// reports continue to arrive, the USB host side is assumed to be alive.
static uint32_t g13_last_report_ms = 0;

// Latched flag to avoid printing repeated stall messages in the fast loop.
static bool     g13_stall_reported = false;

// Timeout bewusst großzügig, damit kein Fehlalarm bei Idle
// Practical-test note:
// 5000 ms was chosen as a conservative stall threshold. It should not fire
// during normal idle time unless the G13 stops producing HID reports entirely.
static const uint32_t G13_STALL_TIMEOUT_MS = 5000; // 5 Sekunden ohne Report -> Stall

// Grace period used before clearing a previously reported stall. This prevents
// noisy "recovered" logs from short bursts of resumed reports.
static const uint32_t G13_RECOVER_GRACE_MS  = 200;

// -----------------------------------------------------------------------------
// Function: G13_Heartbeat
// Purpose:
// Records that a valid HID IN transfer was received from the Logitech G13.
//
// Input:
// None directly. It is called from the HID transfer callback after a report
// arrives.
//
// Output:
// Updates g13_last_report_ms and clears the stall latch after reports resume.
//
// Notes:
// This function must remain very small because it runs on the HID input path.
// It intentionally performs no USB recovery and does not block.
// -----------------------------------------------------------------------------
static inline void G13_Heartbeat() {
  uint32_t now = millis();
  g13_last_report_ms = now;

  if (g13_stall_reported) {
    static uint32_t recovered_at = 0;
    if (recovered_at == 0) recovered_at = now;

    if (now - recovered_at > G13_RECOVER_GRACE_MS) {
      if (Serial) Serial.println("[OK] G13 reports resumed.");
      g13_stall_reported = false;
      recovered_at = 0;
    }
  }
}

// -----------------------------------------------------------------------------
// Function: G13_Monitor
// Purpose:
// Detects long gaps in incoming G13 HID reports and logs a single diagnostic.
//
// Input:
// Uses the global g13_last_report_ms timestamp maintained by G13_Heartbeat().
//
// Output:
// Emits an error-only Serial message and latches g13_stall_reported.
//
// Assumption:
// A long report gap is treated as a possible USB host/HID stall. This is a
// runtime observation, not a confirmed Logitech protocol feature.
// -----------------------------------------------------------------------------
static inline void G13_Monitor() {
  if (g13_last_report_ms == 0) return; // noch nie einen Report gesehen -> nicht alarmieren

  uint32_t delta = millis() - g13_last_report_ms;
  if (!g13_stall_reported && delta > G13_STALL_TIMEOUT_MS) {
    if (Serial) {
      Serial.println("[ERROR] G13 stall detected: no HID reports.");
      Serial.print("        ms_since_last_report=");
      Serial.println(delta);
    }
    g13_stall_reported = true;
  }
}

// ---------- G13-Report-Decoder (Schicht 2 + 3) ----------
// Per-G-key press state cache. Indexes 1..22 correspond to G1..G22.
// The cache prevents repeated Keyboard.press()/release() calls while a key is
// held and allows multiple simultaneous keys to be tracked independently.
static bool g_pressed[32] = {false};  // Merker je G-Taste (1..22)

// -----------------------------------------------------------------------------
// Function: handle_g13_report
// Purpose:
// Decodes the 8-byte Logitech G13 HID report and maps selected G-keys to USB
// keyboard events presented to the Mac by the Teensy device side.
//
// Input:
// d   - pointer to a raw HID report received through USBHost_t36
// len - report length; known working reports are at least 8 bytes
//
// Output:
// Calls Keyboard.press() and Keyboard.release() when mapped key states change.
//
// Confirmed report structure:
// Byte 0 is expected to be report ID/header 0x01 for the reports handled here.
// Byte 3 contains G1..G8 as bit 0..7.
// Byte 4 contains G9..G16 as bit 0..7.
// Byte 5 contains G17..G22 as bit 0..5.
//
// Reverse-engineering notes:
// G17/G20/G22 were first observed as exact values 0x81/0x88/0xA0. The
// reference project g13-master confirms byte 5 as a bitfield: bit 0 = G17,
// bit 3 = G20, bit 5 = G22. Bit 7 appears to be LIGHT_STATE/status and is
// intentionally ignored as a key.
//
// Limitations:
// Joystick, M-keys, MR, L-keys and other status bits are not decoded in this
// function yet. Existing stable HID behavior has priority over expanding scope.
// -----------------------------------------------------------------------------
// Übersetzer: nimmt einen 8-Byte-Report und macht Keyboard-Events daraus
void handle_g13_report(const uint8_t *d, uint32_t len) {
  if (len < 8) return;

  // Wir filtern nur "unsere" bekannten Reports (Header 01 7f 7f)
  if (d[0] != 0x01) return;

  uint8_t b3 = d[3];
  uint8_t b4 = d[4];
  uint8_t b5 = d[5];

  // Local helper:
  // Converts a decoded G-key state into a USB keyboard state transition.
  // It only emits a Keyboard event when the state changes, which keeps the
  // USB device side stable during held keys and during key combinations.
  auto updateKey = [&](int gnum, bool nowPressed, uint8_t keycode) {
    bool before = g_pressed[gnum];
    if (nowPressed && !before) {
      g_pressed[gnum] = true;
      Keyboard.press(keycode);
    }
    else if (!nowPressed && before) {
      g_pressed[gnum] = false;
      Keyboard.release(keycode);
    }
  };

  // Key mapping policy:
  // "bestehende Belegung" marks mappings that were already known to work.
  // "Testbelegung" marks temporary unique letters used to verify newly decoded
  // keys without changing the broader keyboard bridge architecture.

  // Byte 3: G1..G8
  updateKey(1,  b3 & 0x01, '1');  // G1  -> '1'  (bestehende Belegung)
  updateKey(2,  b3 & 0x02, '2');  // G2  -> '2'  (bestehende Belegung)
  updateKey(3,  b3 & 0x04, '3');  // G3  -> '3'  (bestehende Belegung)
  updateKey(4,  b3 & 0x08, '4');  // G4  -> '4'  (bestehende Belegung)
  updateKey(5,  b3 & 0x10, '5');  // G5  -> '5'  (bestehende Belegung)
  updateKey(6,  b3 & 0x20, '6');  // G6  -> '6'  (bestehende Belegung)
  updateKey(7,  b3 & 0x40, '7');  // G7  -> '7'  (bestehende Belegung)
  updateKey(8,  b3 & 0x80, 'k');  // G8  -> 'k'  (Testbelegung)

  // Byte 4: G9..G16
  updateKey(9,  b4 & 0x01, 'l');  // G9  -> 'l'  (Testbelegung)
  updateKey(10, b4 & 0x02, 'a');  // G10 -> 'a'  (bestehende Belegung)
  updateKey(11, b4 & 0x04, 'w');  // G11 -> 'w'  (bestehende Belegung)
  updateKey(12, b4 & 0x08, 'd');  // G12 -> 'd'  (bestehende Belegung)
  updateKey(13, b4 & 0x10, 'm');  // G13 -> 'm'  (Testbelegung)
  updateKey(14, b4 & 0x20, 'n');  // G14 -> 'n'  (Testbelegung)
  updateKey(15, b4 & 0x40, 'o');  // G15 -> 'o'  (Testbelegung)
  updateKey(16, b4 & 0x80, 'p');  // G16 -> 'p'  (Testbelegung)

  // Byte 5: Bits 0..5 = G17..G22, Bit 7 = Statusbit (LIGHT_STATE)
  bool g17_now = b5 & 0x01;
  bool g18_now = b5 & 0x02;
  bool g19_now = b5 & 0x04;
  bool g20_now = b5 & 0x08;
  bool g21_now = b5 & 0x10;
  bool g22_now = b5 & 0x20;

  updateKey(17, g17_now, 's');   // G17 -> 's'          (bestehende Belegung)
  updateKey(18, g18_now, 'q');   // G18 -> 'q'          (Testbelegung)
  updateKey(19, g19_now, 'r');   // G19 -> 'r'          (Testbelegung)
  updateKey(20, g20_now, ' ');   // G20 -> Leertaste    (bestehende Belegung)
  updateKey(21, g21_now, 'u');   // G21 -> 'u'          (Testbelegung)
  updateKey(22, g22_now, 't');   // G22 -> 't'          (bestehende Belegung)
}

// ---------- USB-Host-Struktur (Schicht 1) ----------
// USBHost_t36 objects:
// The Teensy 4.1 acts as a USB host for the physical Logitech G13. At the same
// time it acts as a USB keyboard device toward macOS through Keyboard.h.
//
// The HID parser objects enumerate HID interfaces/collections. The first custom
// listener (g13bridge) performs the G13-to-keyboard translation; the additional
// dump controllers are retained for diagnostics and future attached HID devices.
USBHost myusb;
USBHub  hub1(myusb);
USBHub  hub2(myusb);
USBDeviceInfo dinfo(myusb); // nur Infos, claimt nichts
USBHIDParser hid1(myusb);
USBHIDParser hid2(myusb);
USBHIDParser hid3(myusb);
USBHIDParser hid4(myusb);
USBHIDParser hid5(myusb);

// Wir bauen einen eigenen HID-Listener auf Basis von HIDDumpController:
// -----------------------------------------------------------------------------
// Class: G13KeyboardBridge
// Purpose:
// Bridges the claimed G13 HID collection into the project-specific report
// decoder while preserving the proven HIDDumpController claim/parsing path.
//
// USB behavior:
// claim_collection() delegates to HIDDumpController first. Only after a claim
// succeeds is the optional LCD module allowed to attach to the same parser, and
// only when LCD support is enabled and the parser exposes an OUT endpoint.
//
// Safety note:
// The HID input callback remains the critical path. LCD support is guarded so
// display transfers do not run unless the compile-time and runtime checks pass.
// -----------------------------------------------------------------------------
class G13KeyboardBridge : public HIDDumpController {
public:
  G13KeyboardBridge(USBHost &host, uint32_t index = 0, uint32_t usage = 0)
    : HIDDumpController(host, index, usage) {}

protected:
  // ---------------------------------------------------------------------------
  // Function: claim_collection
  // Purpose:
  // Lets USBHost_t36 decide whether this HID collection belongs to this bridge.
  //
  // Input:
  // driver   - USBHIDParser instance managing the interface
  // dev      - USB device descriptor/state
  // topusage - HID top-level usage discovered from the report descriptor
  //
  // Output:
  // Returns the claim result from HIDDumpController. Optional LCD attachment is
  // a side effect only when G13_LCD_ENABLE is enabled.
  // ---------------------------------------------------------------------------
  virtual hidclaim_t claim_collection(USBHIDParser *driver, Device_t *dev, uint32_t topusage) override {
    hidclaim_t claim = HIDDumpController::claim_collection(driver, dev, topusage);

#if G13_LCD_ENABLE
    if (claim != CLAIM_NO && lcdCanAttachTo(driver)) {
      lcdAttach(driver);
    }
#endif

    return claim;
  }

  // ---------------------------------------------------------------------------
  // Function: disconnect_collection
  // Purpose:
  // Releases per-device HID state and notifies the optional LCD module.
  //
  // Notes:
  // The LCD detach hook is compiled out when G13_LCD_ENABLE is 0. The base
  // class cleanup remains the authoritative HID disconnect handling.
  // ---------------------------------------------------------------------------
  virtual void disconnect_collection(Device_t *dev) override {
#if G13_LCD_ENABLE
    if (dev && dev->idVendor == 0x046d && dev->idProduct == 0xc21c) {
      lcdDetach();
    }
#endif

    HIDDumpController::disconnect_collection(dev);
  }

  // Diese Funktion wird bei jedem eingehenden HID-Report aufgerufen
  // ---------------------------------------------------------------------------
  // Function: hid_process_in_data
  // Purpose:
  // Entry point for incoming HID IN transfers from USBHost_t36.
  //
  // Input:
  // transfer - USBHost_t36 transfer descriptor containing the report buffer
  //
  // Output:
  // Updates the HID heartbeat, optionally dumps diagnostics through the base
  // class, then forwards the raw report to handle_g13_report().
  //
  // Critical path:
  // This method must not block. It is the path that keeps keyboard input stable.
  // ---------------------------------------------------------------------------
  virtual bool hid_process_in_data(const Transfer_t *transfer) override {
    // NEU: Heartbeat -> wir wissen sicher: ein Report kam an
    G13_Heartbeat();

    // Erst die normale Dump-Logik laufen lassen (wie das Original-Beispiel)
    HIDDumpController::hid_process_in_data(transfer);

    const uint8_t *data = (const uint8_t *)transfer->buffer;
    uint32_t len = transfer->length;

    // Unsere Übersetzer-Routine aufrufen
    handle_g13_report(data, len);

    return true;
  }
};

// Ein G13-Bridge-Listener + ggf. weitere Dumper (falls Du andere Geräte anschließt)
G13KeyboardBridge g13bridge(myusb, 1);
HIDDumpController hdc2(myusb, 2);
HIDDumpController hdc3(myusb, 3);
HIDDumpController hdc4(myusb, 4);
HIDDumpController hdc5(myusb, 5);

// Arrays zur Anzeige (Infos zu Verbindungen, wie im Original)
// USBDriver registry:
// Used only to detect connection state changes and print event-style
// diagnostics. These objects do not perform the G13 key mapping themselves.
USBDriver *drivers[] = {&hub1, &hub2, &hid1, &hid2, &hid3, &hid4, &hid5};
#define CNT_DEVICES (sizeof(drivers)/sizeof(drivers[0]))
const char * driver_names[CNT_DEVICES] = {"Hub1", "Hub2", "HID1" , "HID2", "HID3", "HID4", "HID5"};
bool driver_active[CNT_DEVICES] = {false, false, false, false, false, false, false};

// HID-Input-Devices:
// HID listener registry:
// g13bridge is the active translator. hdc2..hdc5 remain diagnostic listeners
// for additional HID collections/devices and for preserving the known-working
// USBHost_t36 example structure.
USBHIDInput *hiddrivers[] = {&g13bridge, &hdc2, &hdc3, &hdc4, &hdc5};
#define CNT_HIDDEVICES (sizeof(hiddrivers)/sizeof(hiddrivers[0]))
const char * hid_driver_names[CNT_HIDDEVICES] = {"G13", "hdc2", "hdc3", "hdc4", "hdc5"};
bool hid_driver_active[CNT_HIDDEVICES] = {false, false, false, false, false};

// Kleine Herzschlag-Funktion (für Debug)
// -----------------------------------------------------------------------------
// Function: heartbeat
// Purpose:
// Drives the onboard LED as a coarse visible status indicator.
//
// Input:
// Uses global systemStatus to select a boot or running blink interval.
//
// Output:
// Toggles STATUS_LED_PIN. It does not touch USB or HID state.
// -----------------------------------------------------------------------------
void heartbeat() {
  static uint32_t lastToggle = 0;
  static bool ledState = false;

  uint32_t now = millis();
  uint32_t interval;

  if (systemStatus == STATUS_BOOTING) {
    interval = 200;   // Schnell blinken beim Hochfahren
  } else {
    interval = 1000;  // Ruhiger Puls im Normalbetrieb
  }

  if (now - lastToggle >= interval) {
    lastToggle = now;
    ledState = !ledState;
    digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
  }
}

// ---------- setup / loop ----------
// -----------------------------------------------------------------------------
// Function: setup
// Purpose:
// Initializes USB serial diagnostics, the Teensy USB keyboard device side and
// the USB host stack used to enumerate the Logitech G13.
//
// Sequence:
// 1. Bring up Serial for diagnostics.
// 2. Start Keyboard device emulation toward macOS.
// 3. Start USBHost_t36 for the physical G13.
// 4. Run a warm-up window so hubs and the G13 can enumerate before normal loop.
//
// Limitation:
// The short delay() calls in the historical warm-up block are inherited from the
// stable baseline. They are not part of the report decoder and should be treated
// cautiously if future HID-stability work revisits startup timing.
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000) {
    // warten, bis USB-Serial bereit ist (nur für Debug)
  }

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  systemStatus = STATUS_BOOTING;

  Keyboard.begin();
  myusb.begin();

  // Warming-Up-Phase: Hub + G13 dürfen in Ruhe hochfahren
  const uint32_t warmup_ms = 4000;  // z.B. 4 Sekunden
  uint32_t start = millis();
  while (millis() - start < warmup_ms) {
    myusb.Task();
    heartbeat();
  }

  // Noch ein paar Zyklen, damit Enumeration & HID-Parser wirklich durch sind
  for (int i = 0; i < 10; i++) {
    myusb.Task();
    delay(10);
  }

  systemStatus = STATUS_RUNNING;
}

// -----------------------------------------------------------------------------
// Function: loop
// Purpose:
// Main cooperative service loop.
//
// Order:
// myusb.Task() services USB host enumeration and transfers first. The remaining
// steps handle optional diagnostics, connection state logs, LED heartbeat, LCD
// service and stall monitoring.
//
// Important:
// The loop avoids cyclic fast Serial output except for inherited diagnostic
// toggles and event-style connection messages. HID input itself is processed by
// USBHost_t36 callbacks, not by polling a custom buffer here.
// -----------------------------------------------------------------------------
void loop() {
  myusb.Task();

  // Serielle Steuerung (wie im Original, optional)
  if (Serial.available()) {
    int ch = Serial.read();
    while (Serial.read() != -1) ;
    if (ch == 'r' || (ch == 'R')) {
      if (HIDDumpController::show_raw_data) {
        HIDDumpController::show_raw_data = false;
        HIDDumpController::show_formated_data = true;
      } else {
        HIDDumpController::show_raw_data = true;
      }
    } else if (ch == 'C' || (ch == 'c')) {
      if (HIDDumpController::changed_data_only) {
        HIDDumpController::changed_data_only = false;
      } else {
        HIDDumpController::changed_data_only = true;
      }
    } else {
      if (HIDDumpController::show_formated_data) {
        HIDDumpController::show_formated_data = false;
        HIDDumpController::show_raw_data = true;
      } else {
        HIDDumpController::show_formated_data = true;
      }
    }
  }

  // Geräte-Connect/Disconnect anzeigen (wie im Original)
  for (uint8_t i = 0; i < CNT_DEVICES; i++) {
    if (*drivers[i] != driver_active[i]) {
      if (driver_active[i]) {
        Serial.printf("*** Device %s - disconnected ***\n", driver_names[i]);
        driver_active[i] = false;
      } else {
        Serial.printf("*** Device %s %x:%x - connected ***\n", driver_names[i],
                      drivers[i]->idVendor(), drivers[i]->idProduct());
        driver_active[i] = true;

        const uint8_t *psz = drivers[i]->manufacturer();
        if (psz && *psz) Serial.printf("  manufacturer: %s\n", psz);
        psz = drivers[i]->product();
        if (psz && *psz) Serial.printf("  product: %s\n", psz);
        psz = drivers[i]->serialNumber();
        if (psz && *psz) Serial.printf("  Serial: %s\n", psz);
      }
    }
  }

  for (uint8_t i = 0; i < CNT_HIDDEVICES; i++) {
    if (*hiddrivers[i] != hid_driver_active[i]) {
      if (hid_driver_active[i]) {
        hid_driver_active[i] = false;
      } else {
        Serial.printf("*** HID Device %s %x:%x - connected ***\n", hid_driver_names[i],
                      hiddrivers[i]->idVendor(), hiddrivers[i]->idProduct());
        hid_driver_active[i] = true;

        const uint8_t *psz = hiddrivers[i]->manufacturer();
        if (psz && *psz) Serial.printf("  manufacturer: %s\n", psz);
        psz = hiddrivers[i]->product();
        if (psz && *psz) Serial.printf("  product: %s\n", psz);
        psz = hiddrivers[i]->serialNumber();
        if (psz && *psz) Serial.printf("  Serial: %s\n", psz);
      }
    }
  }

  heartbeat();

  updateDisplay();

  // NEU: Stall-Detektion (nur Fehlerausgabe)
  G13_Monitor();
}
