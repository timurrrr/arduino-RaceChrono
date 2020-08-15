#include <RaceChrono.h>

namespace {

RaceChronoBleCanHandler *handler = nullptr;

void handle_racechrono_filter_request(
    uint16_t conn_hdl, BLECharacteristic *chr, uint8_t *data, uint16_t len) {
  if (len < 1) {
    // TODO: figure out how to report errors.
    return;
  }

  switch (data[0]) {
    case 1:  // Allow all CAN PIDs.
      if (len == 3) {
        uint16_t notifyIntervalMs = data[1] << 8 | data[2];
        handler->allowAllPids(notifyIntervalMs);
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
        uint16_t notifyIntervalMs = data[1] << 8 | data[2];
        uint32_t pid = data[3] << 24 | data[4] << 16 | data[5] << 8 | data[6];
        handler->allowPid(pid, notifyIntervalMs);
        return;
      }
      break;
  }

  // TODO: figure out how to report errors.
}

}  // namespace

RaceChronoBleAgent RaceChronoBle;

RaceChronoBleAgent::RaceChronoBleAgent() :
  _service(/* uuid= */ 0x00000001000000fd8933990d6f411ff8),
  _pidRequestsCharacteristic(0x02),
  _canBusDataCharacteristic(0x01)
{
}

void RaceChronoBleAgent::setUp(
    const char *bluetoothName, RaceChronoBleCanHandler *handler) {
  ::handler = handler;

  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.begin();
  Bluefruit.setName(bluetoothName);

  _service.begin();

  _pidRequestsCharacteristic.setProperties(CHR_PROPS_WRITE);
  _pidRequestsCharacteristic.setPermission(SECMODE_NO_ACCESS, SECMODE_OPEN);
  _pidRequestsCharacteristic.setWriteCallback(handle_racechrono_filter_request);
  // TODO: use setWriteAuthorizeCallback() to limit read access to only the last authorized device.
  _pidRequestsCharacteristic.begin();

  _canBusDataCharacteristic.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  _canBusDataCharacteristic.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  // TODO: use setReadAuthorizeCallback() to limit read access to only the last
  // authorized device?
  _canBusDataCharacteristic.begin();

  Bluefruit.setTxPower(+4);
}

void RaceChronoBleAgent::startAdvertising() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(_service);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(/* fast= */ 32, /* slow= */ 244); // x0.625 ms
  Bluefruit.Advertising.setFastTimeout(30); // in seconds
  Bluefruit.Advertising.start(/* timeout= */ 0);  // 0 = Don't stop advertising
}

bool RaceChronoBleAgent::waitForConnection(uint32_t timeoutMs) {
  uint32_t startTimeMs = millis();
  while (!Bluefruit.connected()) {
    if (millis() - startTimeMs >= timeoutMs) {
      return false;
    }
    delay(100);
  }

  return true;
}

bool RaceChronoBleAgent::isConnected() {
  return Bluefruit.connected();
}

void RaceChronoBleAgent::sendCanData(
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
