#include <RaceChrono.h>

class PrintRaceChronoCommands : public RaceChronoBleCanHandler {
  void allowAllPids(uint16_t updateIntervalMs) {
    Serial.print("ALLOW ALL PIDS, update interval: ");
    Serial.print(updateIntervalMs);
    Serial.println(" ms");
  }

  void denyAllPids() {
    Serial.println("DENY ALL PIDS");
  }

  void allowPid(uint32_t pid, uint16_t updateIntervalMs) {
    Serial.print("ALLOW PID ");
    Serial.print(pid);
    Serial.print(" (0x");
    Serial.print(pid, HEX);
    Serial.print("), update interval: ");
    Serial.print(updateIntervalMs);
    Serial.println(" ms");
  }
} raceChronoHandler;

// Forward declaration to help put code in a natural reading order.
void wait_for_incoming_connection();

void setup() {
  uint32_t startTimeMs = millis();
  Serial.begin(115200);
  while (!Serial && millis() - startTimeMs < 1000) {
  }

  Serial.println("Setting up BLE...");
  RaceChronoBle.setUp("BLE CAN device demo", &raceChronoHandler);
  RaceChronoBle.startAdvertising();

  Serial.println("BLE is set up, waiting for an incoming connection.");
  wait_for_incoming_connection();
}

void wait_for_incoming_connection() {
  uint32_t iteration = 0;
  bool lastLineHadNewline = false;
  while (!RaceChronoBle.waitForConnection(1000)) {
    Serial.print(".");
    if ((++iteration) % 10 == 0) {
      lastLineHadNewline = true;
      Serial.println();
    } else {
      lastLineHadNewline = false;
    }
  }

  if (!lastLineHadNewline) {
    Serial.println();
  }

  Serial.println("Connected");
}

void loop() {
  if (!RaceChronoBle.isConnected()) {
    Serial.println("RaceChrono disconnected! Waiting for a new connection.");
    wait_for_incoming_connection();
  }
}
