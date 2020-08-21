#include <RaceChrono.h>

// We're ignoring the requested PIDs and notify intervals here for simplicity.
class IgnoreRaceChronoCommands : public RaceChronoBleCanHandler {
  void allowAllPids(uint16_t updateIntervalMs) {}
  void denyAllPids() {}
  void allowPid(uint32_t pid, uint16_t updateIntervalMs) {}
} raceChronoHandler;

// Forward declaration to help put code in a natural reading order.
void waitForConnection();

void setup() {
  uint32_t startTimeMs = millis();
  Serial.begin(115200);
  while (!Serial && millis() - startTimeMs < 1000) {
  }

  Serial.println("Setting up BLE...");
  RaceChronoBle.setUp("BLE CAN device demo", &raceChronoHandler);
  RaceChronoBle.startAdvertising();

  Serial.println("BLE is set up, waiting for an incoming connection.");
  waitForConnection();
}

void waitForConnection() {
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

  Serial.println("Connected.");
}

void loop() {
  if (!RaceChronoBle.isConnected()) {
    Serial.println("RaceChrono disconnected! Waiting for a new connection.");
    waitForConnection();
  }

  for (uint16_t pid = 0; pid < 256; pid++) {
    // For this simple demo, just sent PID repeated 8 times as data.
    uint8_t data[8] = { pid, pid, pid, pid, pid, pid, pid, pid };
    uint8_t len = 8;
    RaceChronoBle.sendCanData(pid, data, len);
  }
}
