/*
 * zte-pico-tool — H3601P Passive UART Capture v0.3.0
 *
 * Passive UART byte capture using Arduino-Pico SerialPIO RX only.
 * The Pico never transmits to the ZTE target pins.
 *
 * Default hypothesis under test:
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
static const size_t CAPTURE_BUFFER_SIZE = 8192;

// RX-only PIO UARTs. NOPIN explicitly disables the TX side.
SerialPIO pad2Uart(NOPIN, PAD2_PIN, 256);
SerialPIO pad3Uart(NOPIN, PAD3_PIN, 256);

struct CaptureBuffer {
  uint8_t data[CAPTURE_BUFFER_SIZE];
  size_t count = 0;
  uint32_t dropped = 0;
};

CaptureBuffer pad2Capture;
CaptureBuffer pad3Capture;

void clearCapture(CaptureBuffer &capture) {
  capture.count = 0;
  capture.dropped = 0;
}

void drainSerial(SerialPIO &uart, CaptureBuffer &capture) {
  while (uart.available()) {
    int value = uart.read();
    if (value < 0) return;

    if (capture.count < CAPTURE_BUFFER_SIZE) {
      capture.data[capture.count++] = static_cast<uint8_t>(value);
    } else {
      capture.dropped++;
    }
  }
}

void flushSerial(SerialPIO &uart) {
  while (uart.available()) uart.read();
}

void printEscapedAscii(const CaptureBuffer &capture) {
  Serial.println("ASCII/escaped:");
  for (size_t i = 0; i < capture.count; ++i) {
    uint8_t c = capture.data[i];
    if (c >= 32 && c <= 126) Serial.write(c);
    else if (c == '\n') Serial.print("\\n\n");
    else if (c == '\r') Serial.print("\\r");
    else if (c == '\t') Serial.print("\\t");
    else Serial.printf("\\x%02X", c);
  }
  Serial.println();
}

void printHex(const CaptureBuffer &capture) {
  Serial.println("HEX:");
  for (size_t i = 0; i < capture.count; i += 16) {
    Serial.printf("%04X  ", static_cast<unsigned>(i));
    size_t lineEnd = min(i + 16, capture.count);
    for (size_t j = i; j < lineEnd; ++j) Serial.printf("%02X ", capture.data[j]);
    Serial.println();
  }
}

void printCapture(const char *label, const CaptureBuffer &capture) {
  Serial.println();
  Serial.print("--- "); Serial.print(label); Serial.println(" ---");
  Serial.print("Bytes: "); Serial.println(capture.count);
  Serial.print("Dropped: "); Serial.println(capture.dropped);
  if (capture.count == 0) {
    Serial.println("No decoded bytes captured.");
    return;
  }
  printEscapedAscii(capture);
  printHex(capture);
}

bool captureStopRequested() {
  if (!Serial.available()) return false;

  int command = Serial.read();
  if (command == 'x' || command == 'X' || command == 3) {
    while (Serial.available()) Serial.read();
    return true;
  }

  return false;
}

void captureOne(const char *label, SerialPIO &uart, CaptureBuffer &capture) {
  clearCapture(capture);
  flushSerial(uart);

  Serial.println();
  Serial.print("Starting passive UART capture on "); Serial.print(label);
  Serial.print(" at "); Serial.print(UART_BAUD);
  Serial.println(" 8N1 for 5 seconds...");
  Serial.println("Power-cycle the ZTE during the capture if you want boot output.");
  Serial.println("Pico RX only: no data is transmitted to the ZTE.");

  uint32_t start = millis();
  while (millis() - start < 5000) {
    drainSerial(uart, capture);
    if (captureStopRequested()) break;
    tight_loop_contents();
  }

  drainSerial(uart, capture);
  printCapture(label, capture);
}

void captureBoth() {
  clearCapture(pad2Capture);
  clearCapture(pad3Capture);
  flushSerial(pad2Uart);
  flushSerial(pad3Uart);

  Serial.println();
  Serial.println("Starting passive UART capture on GP2 and GP3 at 115200 8N1 for 5 seconds...");
  Serial.println("Power-cycle the ZTE during the capture if you want boot output.");
  Serial.println("Both PIO UARTs are RX-only; the Pico never transmits to the ZTE.");

  uint32_t start = millis();
  while (millis() - start < 5000) {
    drainSerial(pad2Uart, pad2Capture);
    drainSerial(pad3Uart, pad3Capture);
    if (captureStopRequested()) break;
    tight_loop_contents();
  }

  drainSerial(pad2Uart, pad2Capture);
  drainSerial(pad3Uart, pad3Capture);
  printCapture("GP2 / ZTE pad 2", pad2Capture);
  printCapture("GP3 / ZTE pad 3", pad3Capture);
}

void captureContinuous() {
  clearCapture(pad2Capture);
  clearCapture(pad3Capture);
  flushSerial(pad2Uart);
  flushSerial(pad3Uart);

  Serial.println();
  Serial.println("Starting CONTINUOUS passive UART capture on GP2 and GP3.");
  Serial.println("Configuration: 115200 8N1, RX-only.");
  Serial.println("Stop with 'x' or Ctrl-C.");
  Serial.println("A serial EOF/disconnect also ends the capture when the USB connection is lost.");
  Serial.println("No data is transmitted to the ZTE.");
  Serial.println();

  uint32_t lastStatus = millis();

  while (true) {
    drainSerial(pad2Uart, pad2Capture);
    drainSerial(pad3Uart, pad3Capture);

    // On USB serial terminals, EOF is normally represented by closing the
    // connection rather than by a byte. If the USB CDC connection disappears,
    // leave the capture loop without transmitting anything to the target.
    if (!Serial) break;
    if (captureStopRequested()) break;

    // Do not continuously dump the captured buffer: USB output would interfere
    // with acquisition. Print a lightweight progress line every 5 seconds.
    if (millis() - lastStatus >= 5000) {
      Serial.print("[capture] GP2 bytes=");
      Serial.print(pad2Capture.count);
      Serial.print(" dropped=");
      Serial.print(pad2Capture.dropped);
      Serial.print(" | GP3 bytes=");
      Serial.print(pad3Capture.count);
      Serial.print(" dropped=");
      Serial.println(pad3Capture.dropped);
      lastStatus = millis();
    }

    tight_loop_contents();
  }

  drainSerial(pad2Uart, pad2Capture);
  drainSerial(pad3Uart, pad3Capture);

  Serial.println();
  Serial.println("=== Continuous capture stopped ===");
  printCapture("GP2 / ZTE pad 2", pad2Capture);
  printCapture("GP3 / ZTE pad 3", pad3Capture);
}

void printState() {
  Serial.println();
  Serial.println("=== ZTE Pico Passive UART Capture 0.3.0 ===");
  Serial.print("GP2 / ZTE pad 2 level: "); Serial.println(digitalRead(PAD2_PIN) ? "HIGH" : "LOW");
  Serial.print("GP3 / ZTE pad 3 level: "); Serial.println(digitalRead(PAD3_PIN) ? "HIGH" : "LOW");
  Serial.print("Configuration: "); Serial.print(UART_BAUD); Serial.println(" baud, 8N1, RX-only");
}

void printHelp() {
  Serial.println();
  Serial.println("zte-pico-tool / Passive UART Capture v0.3.0");
  Serial.println("Passive byte decoding using SerialPIO RX-only at 115200 8N1.");
  Serial.println("Commands:");
  Serial.println("  ?  help");
  Serial.println("  s  current GPIO/UART state");
  Serial.println("  2  capture GP2 / ZTE pad 2 for 5 seconds");
  Serial.println("  3  capture GP3 / ZTE pad 3 for 5 seconds");
  Serial.println("  b  capture GP2 and GP3 simultaneously for 5 seconds");
  Serial.println("  c  continuous capture until x/Ctrl-C/USB EOF");
  Serial.println("  x  stop an active capture early");
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
  Serial.println("\nzte-pico-tool / H3601P Passive UART Capture v0.3.0");
  Serial.println("GP2 and GP3 are monitored with PIO-based RX-only UARTs.");
  printHelp();
}

void loop() {
  if (!Serial.available()) return;
  char command = Serial.read();
  switch (command) {
    case '?': printHelp(); break;
    case 's': printState(); break;
    case '2': captureOne("GP2 / ZTE pad 2", pad2Uart, pad2Capture); break;
    case '3': captureOne("GP3 / ZTE pad 3", pad3Uart, pad3Capture); break;
    case 'b': captureBoth(); break;
    case 'c': captureContinuous(); break;
    case 'x': case 'X': case '\n': case '\r': break;
    default: Serial.println("Unknown command. Press ? for help."); break;
  }
}
