#include "RaceChrono.h"

#if defined(ARDUINO_ARCH_NRF52)
namespace {

RaceChronoBleCanHandler *handler = nullptr;

// The protocol implemented in this file is based on
// https://github.com/aollin/racechrono-ble-diy-device
void handle_racechrono_filter_request(
    uint16_t conn_hdl, BLECharacteristic *chr, uint8_t *data, uint16_t len) {
  if (len < 1) {
    // TODO: figure out how to report errors.
    return;
  }

  switch (data[0]) {
    case 1:  // Allow all CAN PIDs.
      if (len == 3) {
        uint16_t updateIntervalMs = data[1] << 8 | data[2];
        handler->allowAllPids(updateIntervalMs);
        return;
      }
      break;

    case 0:  // Deny all CAN PIDs.
      if (len == 1) {
        handler->denyAllPids();
        return;
      }
      break;

    case 2:  // Allow one more CAN PID.
      if (len == 7) {
        uint16_t updateIntervalMs = data[1] << 8 | data[2];
        uint32_t pid = data[3] << 24 | data[4] << 16 | data[5] << 8 | data[6];
        handler->allowPid(pid, updateIntervalMs);
        return;
      }
      break;
  }

  // TODO: figure out how to report errors.
}


class RaceChronoBleAgentNRF52 : public RaceChronoBleAgent {
public:
  RaceChronoBleAgentNRF52();

  // Seems like the length limit for 'bluetoothName' is 19 visible characters.
  void setUp(const char *bluetoothName, RaceChronoBleCanHandler *handler);

  void startAdvertising();

  // Returns true on success, false on failure.
  bool waitForConnection(uint32_t timeoutMs);

  bool isConnected() const;

  void sendCanData(uint32_t pid, const uint8_t *data, uint8_t len);

private:
  // BLEService docs: https://learn.adafruit.com/bluefruit-nrf52-feather-learning-guide/bleservice
  // BLECharacteristic docs: https://learn.adafruit.com/bluefruit-nrf52-feather-learning-guide/blecharacteristic

  BLEService _service;

  // RaceChrono uses two BLE characteristics:
  // 1) 0x02 to request which PIDs to send, and how frequently
  // 2) 0x01 to be notified of data received for those PIDs
  BLECharacteristic _pidRequestsCharacteristic;
  BLECharacteristic _canBusDataCharacteristic;
};

RaceChronoBleAgentNRF52 RaceChronoNRF52Instance;

RaceChronoBleAgentNRF52::RaceChronoBleAgentNRF52() :
  _service(/* uuid= */ 0x00000001000000fd8933990d6f411ff8),
  _pidRequestsCharacteristic(0x02),
  _canBusDataCharacteristic(0x01)
{
}

void RaceChronoBleAgentNRF52::setUp(
    const char *bluetoothName, RaceChronoBleCanHandler *handler) {
  ::handler = handler;

  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.begin();
  Bluefruit.setName(bluetoothName);

  _service.begin();

  _pidRequestsCharacteristic.setProperties(CHR_PROPS_WRITE);
  _pidRequestsCharacteristic.setPermission(SECMODE_NO_ACCESS, SECMODE_OPEN);
  _pidRequestsCharacteristic.setWriteCallback(handle_racechrono_filter_request);
  _pidRequestsCharacteristic.begin();

  _canBusDataCharacteristic.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  _canBusDataCharacteristic.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  _canBusDataCharacteristic.begin();

  Bluefruit.setTxPower(+4);
}

void RaceChronoBleAgentNRF52::startAdvertising() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(_service);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);

  // Fast mode interval: 20 ms, slow mode interval: 152.5 ms.
  Bluefruit.Advertising.setInterval(/* fast= */ 32, /* slow= */ 244); // x0.625 ms

  // Timeout for fast mode is 30 seconds.
  Bluefruit.Advertising.setFastTimeout(30);

  // Start advertising forever.
  Bluefruit.Advertising.start(/* timeout= */ 0);
}

bool RaceChronoBleAgentNRF52::waitForConnection(uint32_t timeoutMs) {
  uint32_t startTimeMs = millis();
  while (!Bluefruit.connected()) {
    if (millis() - startTimeMs >= timeoutMs) {
      return false;
    }
    delay(100);
  }

  return true;
}

bool RaceChronoBleAgentNRF52::isConnected() const {
  return Bluefruit.connected();
}

void RaceChronoBleAgentNRF52::sendCanData(
    uint32_t pid, const uint8_t *data, uint8_t len) {
  if (len > 8) {
    len = 8;
  }

  unsigned char buffer[20];
  buffer[0] = pid & 0xFF;
  buffer[1] = (pid >> 8) & 0xFF;
  buffer[2] = (pid >> 16) & 0xFF;
  buffer[3] = (pid >> 24) & 0xFF;
  memcpy(buffer + 4, data, len);
  _canBusDataCharacteristic.notify(buffer, 4 + len);
}

}  // namespace

RaceChronoBleAgent &RaceChronoBle = RaceChronoNRF52Instance;

#endif  // ARDUINO_ARCH_NRF52
