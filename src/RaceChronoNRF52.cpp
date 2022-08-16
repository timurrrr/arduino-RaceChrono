#if defined(ARDUINO_ARCH_NRF52)

#include <bluefruit.h>
#include "RaceChrono.h"

namespace {

// TODO: Instead of using a global variable, figure out how to pass a callback
// to setWriteCallback() in a way that we can reference a field of
// RaceChronoBleAgentNRF52.
RaceChronoBleCanHandler *handler = nullptr;

void handle_racechrono_filter_request(
    uint16_t conn_hdl, BLECharacteristic *chr, uint8_t *data, uint16_t len) {
  handler->handlePidRequest(data, len);
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
  _service(RACECHRONO_SERVICE_UUID),
  _pidRequestsCharacteristic(PID_CHARACTERISTIC_UUID),
  _canBusDataCharacteristic(CAN_BUS_CHARACTERISTIC_UUID)
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
  // The numbers specified on the call are multiplied by (0.625 ms) units.
  Bluefruit.Advertising.setInterval(/* fast= */ 32, /* slow= */ 244);

  // Timeout for fast mode is 30 seconds.
  Bluefruit.Advertising.setFastTimeout(30);

  // Start advertising forever.
  Bluefruit.Advertising.start(/* timeout= */ 0);
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
