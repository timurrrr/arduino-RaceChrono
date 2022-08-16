#ifndef __RACECHRONO_H
#define __RACECHRONO_H

#include "RaceChronoPidMap.h"

class RaceChronoBleCanHandler {
public:
  virtual ~RaceChronoBleCanHandler() {}

  virtual void allowAllPids(uint16_t updateIntervalMs);
  virtual void denyAllPids();
  virtual void allowPid(uint32_t pid, uint16_t updateIntervalMs);

  void handlePidRequest(const uint8_t *data, uint16_t len);
};

class RaceChronoBleAgent {
public:
  // Seems like the length limit for 'bluetoothName' is 19 visible characters.
  virtual void setUp(
      const char *bluetoothName, RaceChronoBleCanHandler *handler) = 0;

  virtual void startAdvertising() = 0;

  // Returns true on success, false on failure.
  bool waitForConnection(uint32_t timeoutMs);

  virtual bool isConnected() const = 0;

  virtual void sendCanData(uint32_t pid, const uint8_t *data, uint8_t len) = 0;

protected:
  static const uint16_t RACECHRONO_SERVICE_UUID = 0x1ff8;
  static const uint16_t PID_CHARACTERISTIC_UUID = 0x2;
  static const uint16_t CAN_BUS_CHARACTERISTIC_UUID = 0x1;
};

#if defined(ARDUINO_ARCH_NRF52)
#endif  // ARDUINO_ARCH_NRF52

extern RaceChronoBleAgent &RaceChronoBle;

#endif  // __RACECHRONO_H
