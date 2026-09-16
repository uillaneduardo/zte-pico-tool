/*
 * zte-pico-tool — H3601P Passive UART Capture v0.5.0
 *
 * Passive UART byte capture using Arduino-Pico SerialPIO RX only.
 * The Pico never transmits to the ZTE target pins.
 *
 * v0.5.0 adds chunked USB capture frames with per-channel sequence
 * numbers so the host can detect missing frames during acquisition.
 *
 * Target UART hypothesis:
 *   ZTE pad 2 -> Pico GP2 -> 115200 8N1 RX
 *   ZTE pad 3 -> Pico GP3 -> 115200 8N1 RX
 *
 * Wiring:
 *   ZTE pad 2 -> Pico GP2
 *   ZTE pad 3 -> Pico GP3
 *   ZTE pad 4 -> Pico GND
 *   ZTE pad 1 -> DO NOT CONNECT
 */

#include <Arduino.h>
#include <SerialPIO.h>

static const uint8_t PAD2_PIN = 2;
static const uint8_t PAD3_PIN = 3;
static const uint32_t UART_BAUD = 115200;
static const uint16_t STREAM_CHUNK_SIZE = 256;

SerialPIO pad2Uart(NOPIN, PAD2_PIN, 256);
SerialPIO pad3Uart(NOPIN, PAD3_PIN, 256);

static const uint8_t FRAME_MAGIC[4] = {0x5A, 0x54, 0x45, 0x31}; // ZTE1
static const uint8_t CHANNEL_GP2 = 2;
static const uint8_t CHANNEL_GP3 = 3;

uint32_t gp2Sequence = 0;
uint32_t gp3Sequence = 0;

bool captureStopRequested() {
  while (Serial.available()) {
    int command = Serial.read();
    if (command == 'x' || command == 'X' || command == 3) {
      while (Serial.available()) Serial.read();
      return true;
    }
  }
  return false;
}

void flushSerial(SerialPIO &uart) {
  while (uart.available()) uart.read();
}

void writeUint32LE(uint32_t value) {
  Serial.write(static_cast<uint8_t>(value & 0xFF));
  Serial.write(static_cast<uint8_t>((value >> 8) & 0xFF));
  Serial.write(static_cast<uint8_t>((value >> 16) & 0xFF));
  Serial.write(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void writeFrame(uint8_t channel, uint32_t sequence, uint8_t *data, uint16_t length) {
  Serial.write(FRAME_MAGIC, sizeof(FRAME_MAGIC));
  Serial.write(channel);
  writeUint32LE(sequence);
  Serial.write(static_cast<uint8_t>(length & 0xFF));
  Serial.write(static_cast<uint8_t>((length >> 8) & 0xFF));
  Serial.write(data, length);
}

uint16_t drainToBuffer(SerialPIO &uart, uint8_t *buffer, uint16_t capacity) {
  uint16_t count = 0;
  while (uart.available() && count < capacity) {
    int value = uart.read();
    if (value < 0) break;
    buffer[count++] = static_cast<uint8_t>(value);
  }
  return count;
}

void streamContinuous() {
  uint8_t gp2Buffer[STREAM_CHUNK_SIZE];
  uint8_t gp3Buffer[STREAM_CHUNK_SIZE];

  gp2Sequence = 0;
  gp3Sequence = 0;

  flushSerial(pad2Uart);
  flushSerial(pad3Uart);

  Serial.println("ZTE-CAPTURE-V2");

  while (true) {
    if (!Serial) break;
    if (captureStopRequested()) break;

    uint16_t gp2Count = drainToBuffer(pad2Uart, gp2Buffer, STREAM_CHUNK_SIZE);
    if (gp2Count > 0) {
      writeFrame(CHANNEL_GP2, gp2Sequence++, gp2Buffer, gp2Count);
    }

    uint16_t gp3Count = drainToBuffer(pad3Uart, gp3Buffer, STREAM_CHUNK_SIZE);
    if (gp3Count > 0) {
      writeFrame(CHANNEL_GP3, gp3Sequence++, gp3Buffer, gp3Count);
    }

    tight_loop_contents();
  }

  flushSerial(pad2Uart);
  flushSerial(pad3Uart);
}

void printState() {
  Serial.println();
  Serial.println("=== ZTE Pico Passive UART Capture 0.5.0 ===");
  Serial.print("GP2 / ZTE pad 2 level: "); Serial.println(digitalRead(PAD2_PIN) ? "HIGH" : "LOW");
  Serial.print("GP3 / ZTE pad 3 level: "); Serial.println(digitalRead(PAD3_PIN) ? "HIGH" : "LOW");
  Serial.print("Configuration: "); Serial.print(UART_BAUD); Serial.println(" baud, 8N1, RX-only");
}

void printHelp() {
  Serial.println();
  Serial.println("zte-pico-tool / Passive UART Capture v0.5.0");
  Serial.println("Passive byte decoding using SerialPIO RX-only at 115200 8N1.");
  Serial.println("Commands:");
  Serial.println("  ?  help");
  Serial.println("  s  current GPIO/UART state");
  Serial.println("  c  continuous binary capture stream until x/Ctrl-C/USB EOF");
  Serial.println("  x  stop an active capture");
  Serial.println("Capture protocol: ZTE-CAPTURE-V2 + sequenced binary records.");
  Serial.println("Wiring: pad2->GP2, pad3->GP3, pad4->GND; pad1 disconnected.");
  Serial.println("Safety: both PIO UARTs are RX-only; the Pico does not transmit to the ZTE.");
}

void setup() {
  pinMode(PAD2_PIN, INPUT);
  pinMode(PAD3_PIN, INPUT);
  Serial.begin(115200);
  pad2Uart.begin(UART_BAUD);
  pad3Uart.begin(UART_BAUD);
  delay(1000);
  Serial.println("\nzte-pico-tool / H3601P Passive UART Capture v0.5.0");
  Serial.println("GP2 and GP3 are monitored with PIO-based RX-only UARTs.");
  printHelp();
}

void loop() {
  if (!Serial.available()) return;

  char command = Serial.read();
  switch (command) {
    case '?': printHelp(); break;
    case 's': printState(); break;
    case 'c': streamContinuous(); break;
    case 'x': case 'X': case '\n': case '\r': break;
    default: Serial.println("Unknown command. Press ? for help."); break;
  }
}
