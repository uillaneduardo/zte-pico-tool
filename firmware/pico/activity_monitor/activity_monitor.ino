/*
 * zte-pico-tool — H3601P UART Activity Monitor
 *
 * Firmware version: 0.1.0
 *
 * PURPOSE
 * -------
 * Passive digital activity monitor for the first hardware investigation
 * of the ZTE H3601P UART test pads.
 *
 * GP2 and GP3 are INPUT ONLY. The firmware never drives the target pins.
 * GP2/GP3 are sampled and transition counts are reported over USB CDC.
 *
 * WIRING
 * -------
 * ZTE pad 2 -> Pico GP2
 * ZTE pad 3 -> Pico GP3
 * ZTE pad 4 -> Pico GND
 * ZTE pad 1 -> DO NOT CONNECT
 *
 * SAFETY
 * ------
 * - Do not connect ZTE pad 1 until its function is confirmed.
 * - Do not connect Pico VBUS/3V3 to the ZTE board.
 * - This firmware is intentionally read-only with respect to the target.
 * - Confirm target GPIO voltage before connecting signals to the Pico.
 *
 * SERIAL OUTPUT
 * -------------
 * Connect the Pico by USB and open its USB serial port at any baud rate
 * (USB CDC ignores the UART baud setting).
 *
 * Commands:
 *   ?      show help
 *   s      show current state
 *   r      reset counters
 *   m      monitor for 5 seconds
 *   c      continuous monitor (press any key to stop)
 *
 * The transition counter is intentionally simple. It is a diagnostic aid,
 * not yet a logic analyzer or UART decoder.
 */

#include <Arduino.h>

static const uint8_t PAD2_PIN = 2;
static const uint8_t PAD3_PIN = 3;
static const uint32_t SAMPLE_INTERVAL_US = 2;

volatile uint32_t pad2_rising = 0;
volatile uint32_t pad2_falling = 0;
volatile uint32_t pad3_rising = 0;
volatile uint32_t pad3_falling = 0;

uint8_t last_state = 0;

void resetCounters() {
  noInterrupts();
  pad2_rising = 0;
  pad2_falling = 0;
  pad3_rising = 0;
  pad3_falling = 0;
  interrupts();
}

void samplePins() {
  uint8_t state = (digitalRead(PAD2_PIN) ? 0x01 : 0x00) |
                  (digitalRead(PAD3_PIN) ? 0x02 : 0x00);

  uint8_t changed = state ^ last_state;

  if (changed & 0x01) {
    if (state & 0x01) pad2_rising++;
    else pad2_falling++;
  }

  if (changed & 0x02) {
    if (state & 0x02) pad3_rising++;
    else pad3_falling++;
  }

  last_state = state;
}

void printState() {
  noInterrupts();
  uint32_t p2r = pad2_rising;
  uint32_t p2f = pad2_falling;
  uint32_t p3r = pad3_rising;
  uint32_t p3f = pad3_falling;
  interrupts();

  Serial.println();
  Serial.println("=== ZTE Pico Activity Monitor 0.1.0 ===");
  Serial.print("GP2 / ZTE pad 2: ");
  Serial.print(digitalRead(PAD2_PIN) ? "HIGH" : "LOW");
  Serial.print(" | rising=");
  Serial.print(p2r);
  Serial.print(" falling=");
  Serial.println(p2f);

  Serial.print("GP3 / ZTE pad 3: ");
  Serial.print(digitalRead(PAD3_PIN) ? "HIGH" : "LOW");
  Serial.print(" | rising=");
  Serial.print(p3r);
  Serial.print(" falling=");
  Serial.println(p3f);
}

void monitorFor(uint32_t durationMs, bool continuous) {
  resetCounters();
  last_state = (digitalRead(PAD2_PIN) ? 0x01 : 0x00) |
               (digitalRead(PAD3_PIN) ? 0x02 : 0x00);

  uint32_t start = millis();
  uint32_t lastReport = start;

  Serial.println();
  Serial.println(continuous
    ? "Monitoring continuously. Press any key to stop..."
    : "Monitoring for 5 seconds...");

  while (continuous || (millis() - start < durationMs)) {
    uint32_t begin = micros();
    samplePins();
    while ((uint32_t)(micros() - begin) < SAMPLE_INTERVAL_US) {
      tight_loop_contents();
    }

    if (millis() - lastReport >= 1000) {
      lastReport = millis();
      printState();
    }

    if (continuous && Serial.available()) {
      while (Serial.available()) Serial.read();
      break;
    }
  }

  Serial.println();
  Serial.println("--- Capture result ---");
  printState();
}

void printHelp() {
  Serial.println();
  Serial.println("zte-pico-tool / Activity Monitor v0.1.0");
  Serial.println("Passive monitor for H3601P pads 2 and 3.");
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  ?  help");
  Serial.println("  s  show current state/counters");
  Serial.println("  r  reset counters");
  Serial.println("  m  monitor for 5 seconds");
  Serial.println("  c  continuous monitor; press key to stop");
  Serial.println();
  Serial.println("Wiring: pad2->GP2, pad3->GP3, pad4->GND.");
  Serial.println("Pad1 MUST remain disconnected.");
}

void setup() {
  pinMode(PAD2_PIN, INPUT);
  pinMode(PAD3_PIN, INPUT);

  Serial.begin(115200);
  delay(1000);

  last_state = (digitalRead(PAD2_PIN) ? 0x01 : 0x00) |
               (digitalRead(PAD3_PIN) ? 0x02 : 0x00);

  Serial.println();
  Serial.println("zte-pico-tool / H3601P Activity Monitor v0.1.0");
  Serial.println("Passive mode: GP2/GP3 are inputs only.");
  Serial.println("Pad 1 is intentionally unused.");
  printHelp();
}

void loop() {
  if (!Serial.available()) {
    return;
  }

  char command = Serial.read();

  switch (command) {
    case '?':
      printHelp();
      break;
    case 's':
      printState();
      break;
    case 'r':
      resetCounters();
      Serial.println("Counters reset.");
      break;
    case 'm':
      monitorFor(5000, false);
      break;
    case 'c':
      monitorFor(0, true);
      break;
    case '\n':
    case '\r':
      break;
    default:
      Serial.println("Unknown command. Press ? for help.");
      break;
  }
}
