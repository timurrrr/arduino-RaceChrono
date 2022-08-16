#include <Arduino.h>
#include "RaceChrono.h"

void RaceChronoBleCanHandler::handlePidRequest(
    const uint8_t *data, uint16_t len) {
  // The protocol implemented in this file is based on
  // https://github.com/aollin/racechrono-ble-diy-device

  if (len < 1) {
    // TODO: figure out how to report errors.
    return;
  }

  switch (data[0]) {
    case 1:  // Allow all CAN PIDs.
      if (len == 3) {
        uint16_t updateIntervalMs = data[1] << 8 | data[2];
        allowAllPids(updateIntervalMs);
        return;
      }
      break;

    case 0:  // Deny all CAN PIDs.
      if (len == 1) {
        denyAllPids();
        return;
      }
      break;

    case 2:  // Allow one more CAN PID.
      if (len == 7) {
        uint16_t updateIntervalMs = data[1] << 8 | data[2];
        uint32_t pid = data[3] << 24 | data[4] << 16 | data[5] << 8 | data[6];
        allowPid(pid, updateIntervalMs);
        return;
      }
      break;
  }

  // TODO: figure out how to report errors.
}

bool RaceChronoBleAgent::waitForConnection(uint32_t timeoutMs) {
  uint32_t startTimeMs = millis();
  while (!isConnected()) {
    if (millis() - startTimeMs >= timeoutMs) {
      return false;
    }
    delay(100);
  }

  return true;
}
