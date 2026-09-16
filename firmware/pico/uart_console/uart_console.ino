/*
 * zte-pico-tool — H3601P Interactive UART Console v0.1.0
 *
 * Controlled UART console bridge.
 *
 * Wiring hypothesis:
 *   ZTE pad 2 -> Pico GP2 (ZTE TX / Pico RX)
 *   ZTE pad 3 -> Pico GP3 (ZTE RX / Pico TX)
 *   ZTE pad 4 -> Pico GND
 *   ZTE pad 1 -> DO NOT CONNECT
 *
 * Safety:
 *   TX is DISABLED by default. The console starts in RX-only mode.
 *   The host must explicitly send ARM before bytes can be transmitted
 *   to the ZTE. This is intentional: accidental keystrokes must not
 *   reach the target.
 *
 * The host console can use the command '1' during the bootloader's
 * documented countdown. The Pico forwards that byte only after TX is
 * explicitly armed by the host.
 */

#include <Arduino.h>
#include <SerialPIO.h>

static const uint8_t ZTE_RX_PIN = 2; // ZTE TX -> Pico RX
static const uint8_t ZTE_TX_PIN = 3; // Pico TX -> ZTE RX
static const uint32_t UART_BAUD = 115200;

SerialPIO zteUart(ZTE_TX_PIN, ZTE_RX_PIN, 256);
bool txArmed = false;

void printStatus() {
  Serial.println();
  Serial.println("=== ZTE Pico Interactive UART Console v0.1.0 ===");
  Serial.println("UART: 115200 8N1");
  Serial.println("RX: ZTE pad 2 -> GP2");
  Serial.println("TX: GP3 -> ZTE pad 3");
  Serial.print("TX state: ");
  Serial.println(txArmed ? "ARMED" : "BLOCKED");
  Serial.println("Commands:");
  Serial.println("  ?       help");
  Serial.println("  s       status");
  Serial.println("  a       arm TX");
  Serial.println("  d       disarm TX");
  Serial.println("  1       transmit ASCII '1' (only when armed)");
  Serial.println("  x       disarm TX and stop console");
}

void setup() {
  pinMode(ZTE_RX_PIN, INPUT);
  pinMode(ZTE_TX_PIN, OUTPUT);
  digitalWrite(ZTE_TX_PIN, HIGH);

  Serial.begin(115200);
  zteUart.begin(UART_BAUD);

  delay(1000);
  Serial.println("\nzte-pico-tool / H3601P Interactive UART Console v0.1.0");
  Serial.println("TX starts BLOCKED. Use 'a' to explicitly arm transmission.");
  printStatus();
}

void loop() {
  while (zteUart.available()) {
    int value = zteUart.read();
    if (value >= 0) Serial.write((uint8_t)value);
  }

  while (Serial.available()) {
    int value = Serial.read();
    if (value < 0) return;

    switch (value) {
      case '?':
        printStatus();
        break;
      case 's':
      case 'S':
        printStatus();
        break;
      case 'a':
      case 'A':
        txArmed = true;
        Serial.println("\n[TX ARMED]");
        break;
      case 'd':
      case 'D':
        txArmed = false;
        Serial.println("\n[TX DISARMED]");
        break;
      case '1':
        if (txArmed) {
          zteUart.write((uint8_t)'1');
          Serial.println("\n[TX] 0x31 '1'");
        } else {
          Serial.println("\n[TX BLOCKED] '1' was not transmitted. Arm TX with 'a'.");
        }
        break;
      case 'x':
      case 'X':
        txArmed = false;
        Serial.println("\n[CONSOLE STOPPED / TX DISARMED]");
        return;
      case '\n':
      case '\r':
        break;
      default:
        if (txArmed) {
          zteUart.write((uint8_t)value);
          Serial.write((uint8_t)value);
        } else {
          Serial.println("\n[TX BLOCKED] Arm TX with 'a' before sending target bytes.");
        }
        break;
    }
  }

  tight_loop_contents();
}
