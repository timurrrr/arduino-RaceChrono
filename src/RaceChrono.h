#ifndef __RACECHRONO_H
#define __RACECHRONO_H
#include <Arduino.h>

#if defined(ARDUINO_ARCH_NRF52)
  #include <bluefruit.h>
#elif defined(ARDUINO_ARCH_ESP32)
  #include <NimBLEDevice.h>
#endif

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
  RaceChronoBleAgent();

  // Seems like the length limit for 'bluetoothName' is 19 visible characters.
  void setUp(const char *bluetoothName, RaceChronoBleCanHandler *handler);

  void startAdvertising();

  // Returns true on success, false on failure.
  bool waitForConnection(uint32_t timeoutMs);

  bool isConnected() const;

  void sendCanData(uint32_t pid, const uint8_t *data, uint8_t len);

private:
  // The protocol implemented in this file is based on
  // https://github.com/aollin/racechrono-ble-diy-device

  // BLEService docs: https://learn.adafruit.com/bluefruit-nrf52-feather-learning-guide/bleservice
  // BLECharacteristic docs: https://learn.adafruit.com/bluefruit-nrf52-feather-learning-guide/blecharacteristic
  #if defined(ARDUINO_ARCH_NRF52)
  BLEService _service;

  // RaceChrono uses two BLE characteristics:
  // 1) 0x02 to request which PIDs to send, and how frequently
  // 2) 0x01 to be notified of data received for those PIDs
  
  BLECharacteristic _pidRequestsCharacteristic;
  BLECharacteristic _canBusDataCharacteristic;
  #elif defined(ARDUINO_ARCH_ESP32)
  BLEServer* pServer;
  BLEService* pMainService;
  std::string BTName;
  NimBLECharacteristic* _pidRequestsCharacteristic;
  NimBLECharacteristic* _canBusDataCharacteristic;
  #endif
};

extern RaceChronoBleAgent RaceChronoBle;

#endif  // __RACECHRONO_H
