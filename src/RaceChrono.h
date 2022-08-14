#ifndef __RACECHRONO_H
#define __RACECHRONO_H

#include "RaceChronoPidMap.h"

class RaceChronoBleCanHandler {
public:
  virtual ~RaceChronoBleCanHandler() {}

  virtual void allowAllPids(uint16_t updateIntervalMs);
  virtual void denyAllPids();
  virtual void allowPid(uint32_t pid, uint16_t updateIntervalMs);
};

class RaceChronoBleAgent {
public:
  // Seems like the length limit for 'bluetoothName' is 19 visible characters.
  virtual void setUp(
      const char *bluetoothName, RaceChronoBleCanHandler *handler) = 0;

  virtual void startAdvertising() = 0;

  // Returns true on success, false on failure.
  virtual bool waitForConnection(uint32_t timeoutMs) = 0;

  virtual bool isConnected() const = 0;

  virtual void sendCanData(uint32_t pid, const uint8_t *data, uint8_t len) = 0;
};

#if defined(ARDUINO_ARCH_NRF52)
#endif  // ARDUINO_ARCH_NRF52

extern RaceChronoBleAgent &RaceChronoBle;

#endif  // __RACECHRONO_H
