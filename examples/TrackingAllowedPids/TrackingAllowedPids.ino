#include <RaceChrono.h>

// In this example, we use RaceChronoPidMap to keep track of the requested PIDs
// and update intervals. You can use any POD type in as an "extra" in this map,
// but in this example we don't actually need any extras.
// If you do use an extra, keep it small for better performance.
using NoExtra = struct {};
RaceChronoPidMap<NoExtra> pidMap;

void dumpMapToSerial() {
  Serial.println("Current state of the PID map:");

  uint16_t updateIntervalForAllEntries;
  bool areAllPidsAllowed =
      pidMap.areAllPidsAllowed(&updateIntervalForAllEntries);
  if (areAllPidsAllowed) {
    Serial.print("  All PIDs are allowed, update interval: ");
    Serial.print(updateIntervalForAllEntries);
    Serial.println(" ms.");
    Serial.println("");
  }

  if (pidMap.isEmpty()) {
    if (areAllPidsAllowed) {
      // This condition doesn't happen in this example, but will happen if you
      // query the map for incoming data in the "allow all" mode.
      Serial.println("  Map is empty.");
      Serial.println("");
    } else {
      Serial.println("  No PIDs are allowed.");
      Serial.println("");
    }
    return;
  }

  struct {
    void operator() (const void *entry) {
      uint32_t pid = pidMap.getPid(entry);
      uint16_t updateIntervalMs = pidMap.getUpdateIntervalMs(entry);

      Serial.print("  ");
      Serial.print(pid);
      Serial.print(" (0x");
      Serial.print(pid, HEX);
      Serial.print("), update interval: ");
      Serial.print(updateIntervalMs);
      Serial.println(" ms.");
    }
  } dumpEntry;
  pidMap.forEach(dumpEntry);

  Serial.println("");
}

class UpdateMapOnRaceChronoCommands : public RaceChronoBleCanHandler {
public:
  void allowAllPids(uint16_t updateIntervalMs) {
    Serial.print("Command: ALLOW ALL PIDS, update interval: ");
    Serial.print(updateIntervalMs);
    Serial.println(" ms.");

    pidMap.allowAllPids(updateIntervalMs);

    dumpMapToSerial();
  }

  void denyAllPids() {
    Serial.println("Command: DENY ALL PIDS");

    pidMap.reset();

    dumpMapToSerial();
  }

  void allowPid(uint32_t pid, uint16_t updateIntervalMs) {
    Serial.print("Command: ALLOW PID ");
    Serial.print(pid);
    Serial.print(" (0x");
    Serial.print(pid, HEX);
    Serial.print("), update interval: ");
    Serial.print(updateIntervalMs);
    Serial.println(" ms");

    if (!pidMap.allowOnePid(pid, updateIntervalMs)) {
      Serial.println("WARNING: unable to handle this request!");
    }

    dumpMapToSerial();
  }

  void handleDisconnect() {
    Serial.println("Resetting the map.");

    pidMap.reset();

    dumpMapToSerial();
  }
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
  bool lastPrintHadNewline = false;
  while (!RaceChronoBle.waitForConnection(1000)) {
    Serial.print(".");
    if ((++iteration) % 10 == 0) {
      lastPrintHadNewline = true;
      Serial.println();
    } else {
      lastPrintHadNewline = false;
    }
  }

  if (!lastPrintHadNewline) {
    Serial.println();
  }

  Serial.println("Connected");
}

void loop() {
  if (!RaceChronoBle.isConnected()) {
    Serial.println("RaceChrono disconnected!");
    raceChronoHandler.handleDisconnect();

    Serial.println("Waiting for a new connection.");
    waitForConnection();
  }
}
