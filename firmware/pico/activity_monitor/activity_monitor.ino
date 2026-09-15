/*
 * zte-pico-tool — H3601P Activity Monitor v0.2.0
 *
 * Passive timing capture. GP2/GP3 are INPUT ONLY; the Pico never drives
 * the ZTE target pins. Edge timestamps are captured with GPIO interrupts.
 * This version measures timing and gives UART baud-rate heuristics; it does
 * not decode or transmit UART data yet.
 *
 * Wiring:
 *   ZTE pad 2 -> Pico GP2
 *   ZTE pad 3 -> Pico GP3
 *   ZTE pad 4 -> Pico GND
 *   ZTE pad 1 -> DO NOT CONNECT
 */

#include <Arduino.h>

static const uint8_t PAD2_PIN = 2;
static const uint8_t PAD3_PIN = 3;
static const uint16_t MAX_EDGES = 4096;
static const uint32_t MAX_DELTA_US = 5000;

struct Edge { uint32_t timestamp; uint8_t level; };

volatile Edge pad2_edges[MAX_EDGES];
volatile Edge pad3_edges[MAX_EDGES];
volatile uint16_t pad2_count = 0;
volatile uint16_t pad3_count = 0;
volatile bool capture_active = false;

void onPad2Edge() {
  if (!capture_active) return;
  uint16_t i = pad2_count;
  if (i < MAX_EDGES) {
    pad2_edges[i].timestamp = micros();
    pad2_edges[i].level = digitalRead(PAD2_PIN) ? 1 : 0;
    pad2_count = i + 1;
  }
}

void onPad3Edge() {
  if (!capture_active) return;
  uint16_t i = pad3_count;
  if (i < MAX_EDGES) {
    pad3_edges[i].timestamp = micros();
    pad3_edges[i].level = digitalRead(PAD3_PIN) ? 1 : 0;
    pad3_count = i + 1;
  }
}

uint16_t countOf(volatile uint16_t &v) {
  noInterrupts();
  uint16_t n = v;
  interrupts();
  return n;
}

void resetCapture() {
  noInterrupts();
  pad2_count = 0;
  pad3_count = 0;
  capture_active = false;
  interrupts();
}

void printBaudHeuristics(volatile Edge *edges, uint16_t count) {
  const uint32_t baudRates[] = {1200,2400,4800,9600,19200,38400,57600,115200,230400,460800};
  const size_t baudCount = sizeof(baudRates) / sizeof(baudRates[0]);

  Serial.println("Candidate UART baud rates (heuristic):");
  for (size_t b = 0; b < baudCount; ++b) {
    double bitUs = 1000000.0 / baudRates[b];
    uint32_t matches = 0, samples = 0;
    for (uint16_t i = 1; i < count; ++i) {
      uint32_t d = edges[i].timestamp - edges[i - 1].timestamp;
      if (d < 2 || d > MAX_DELTA_US) continue;
      uint32_t multiple = (uint32_t)((d / bitUs) + 0.5);
      if (multiple < 1) multiple = 1;
      double expected = multiple * bitUs;
      double error = ((double)d - expected) / expected;
      samples++;
      if (error >= -0.15 && error <= 0.15) matches++;
    }
    if (samples > 20) {
      double score = 100.0 * matches / samples;
      if (score >= 70.0) {
        Serial.print("  "); Serial.print(baudRates[b]);
        Serial.print(" baud -> "); Serial.print(score, 1);
        Serial.println("% timing matches");
      }
    }
  }
}

void printCaptureSummary(uint8_t pin, volatile Edge *edges, uint16_t count) {
  Serial.print("\n--- GP"); Serial.print(pin); Serial.println(" timing capture ---");
  Serial.print("Edges: "); Serial.println(count);
  if (count < 2) { Serial.println("Insufficient edges for timing analysis."); return; }

  uint32_t minDelta = 0xFFFFFFFFUL, maxDelta = 0;
  uint64_t sum = 0; uint32_t valid = 0;
  uint16_t histogram[251] = {0};

  for (uint16_t i = 1; i < count; ++i) {
    uint32_t d = edges[i].timestamp - edges[i - 1].timestamp;
    if (d == 0 || d > MAX_DELTA_US) continue;
    if (d < minDelta) minDelta = d;
    if (d > maxDelta) maxDelta = d;
    sum += d; valid++;
    if (d <= 250 && histogram[d] < 65535) histogram[d]++;
  }

  if (!valid) { Serial.println("No usable edge intervals found."); return; }
  Serial.print("Min interval: "); Serial.print(minDelta); Serial.println(" us");
  Serial.print("Max interval (<=5000 us): "); Serial.print(maxDelta); Serial.println(" us");
  Serial.print("Average interval: "); Serial.print((double)sum / valid, 2); Serial.println(" us");

  Serial.println("Repeated short intervals (1..250 us):");
  uint8_t printed = 0;
  for (uint16_t us = 1; us <= 250; ++us) {
    if (histogram[us] >= 2) {
      Serial.print("  "); Serial.print(us); Serial.print(" us: "); Serial.println(histogram[us]);
      if (++printed >= 20) break;
    }
  }
  if (!printed) Serial.println("  none");
  printBaudHeuristics(edges, count);
}

void printState() {
  Serial.println("\n=== ZTE Pico Activity Monitor 0.2.0 ===");
  Serial.print("GP2 / ZTE pad 2: "); Serial.print(digitalRead(PAD2_PIN) ? "HIGH" : "LOW");
  Serial.print(" | edges="); Serial.println(countOf(pad2_count));
  Serial.print("GP3 / ZTE pad 3: "); Serial.print(digitalRead(PAD3_PIN) ? "HIGH" : "LOW");
  Serial.print(" | edges="); Serial.println(countOf(pad3_count));
}

void capture(uint32_t durationMs, bool continuous) {
  resetCapture();
  attachInterrupt(digitalPinToInterrupt(PAD2_PIN), onPad2Edge, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PAD3_PIN), onPad3Edge, CHANGE);
  noInterrupts(); capture_active = true; interrupts();

  uint32_t start = millis(), lastReport = start;
  Serial.println(continuous ? "\nTiming capture continuously. Press any key to stop..." : "\nTiming capture for 5 seconds...");
  Serial.println("Passive mode: Pico pins remain inputs.");

  while (continuous || millis() - start < durationMs) {
    if (millis() - lastReport >= 1000) { lastReport = millis(); printState(); }
    if (continuous && Serial.available()) { while (Serial.available()) Serial.read(); break; }
    if (countOf(pad2_count) >= MAX_EDGES && countOf(pad3_count) >= MAX_EDGES) break;
    tight_loop_contents();
  }

  noInterrupts(); capture_active = false; interrupts();
  detachInterrupt(digitalPinToInterrupt(PAD2_PIN));
  detachInterrupt(digitalPinToInterrupt(PAD3_PIN));

  uint16_t p2 = countOf(pad2_count), p3 = countOf(pad3_count);
  Serial.println("\n=== Capture result ===");
  printCaptureSummary(2, pad2_edges, p2);
  printCaptureSummary(3, pad3_edges, p3);
}

void printHelp() {
  Serial.println("\nzte-pico-tool / Activity Monitor v0.2.0");
  Serial.println("Passive timing capture for H3601P pads 2 and 3.");
  Serial.println("Commands:");
  Serial.println("  ?  help");
  Serial.println("  s  current state/capture counts");
  Serial.println("  r  reset capture buffers");
  Serial.println("  m  capture timing for 5 seconds");
  Serial.println("  c  continuous timing capture; press key to stop");
  Serial.println("Wiring: pad2->GP2, pad3->GP3, pad4->GND; pad1 disconnected.");
  Serial.println("v0.2.0 measures timing only; it does not decode or transmit UART.");
}

void setup() {
  pinMode(PAD2_PIN, INPUT); pinMode(PAD3_PIN, INPUT);
  Serial.begin(115200); delay(1000);
  Serial.println("\nzte-pico-tool / H3601P Activity Monitor v0.2.0");
  Serial.println("Passive timing mode: GP2/GP3 are inputs only.");
  printHelp();
}

void loop() {
  if (!Serial.available()) return;
  char command = Serial.read();
  switch (command) {
    case '?': printHelp(); break;
    case 's': printState(); break;
    case 'r': resetCapture(); Serial.println("Capture buffers reset."); break;
    case 'm': capture(5000, false); break;
    case 'c': capture(0, true); break;
    case '\n': case '\r': break;
    default: Serial.println("Unknown command. Press ? for help."); break;
  }
}
