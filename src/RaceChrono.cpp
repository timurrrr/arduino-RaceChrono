#include "RaceChrono.h"

#define MAIN_SERVICE_NAME           (uint16_t) 0x1ff8
#define PID_CHARACTERISTIC          (uint16_t) 0x02
#define CANBUS_CHARACTERISTIC       (uint16_t) 0x01

namespace {

RaceChronoBleCanHandler *handler = nullptr;

#if defined(ARDUINO_ARCH_NRF52)
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
#elif defined(ARDUINO_ARCH_ESP32)
class handle_racechrono_filter_request: public NimBLECharacteristicCallbacks {

  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string data = pCharacteristic->getValue();
    int len = pCharacteristic->getValue().length();
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
  }
};
#endif

}  // namespace

RaceChronoBleAgent RaceChronoBle;

#if defined(ARDUINO_ARCH_NRF52)
RaceChronoBleAgent::RaceChronoBleAgent() :
  _service(/* uuid= */ MAIN_SERVICE_NAME),
  _pidRequestsCharacteristic(PID_CHARACTERISTIC),
  _canBusDataCharacteristic(CANBUS_CHARACTERISTIC)
{
}
#elif defined(ARDUINO_ARCH_ESP32)
RaceChronoBleAgent::RaceChronoBleAgent() :
  BTName(),
  pServer(),
  pMainService(),
  _pidRequestsCharacteristic(),
  _canBusDataCharacteristic()
{
}

static handle_racechrono_filter_request cbhandle_racechrono_filter_request;
#endif

#if defined(ARDUINO_ARCH_NRF52)
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
  _pidRequestsCharacteristic.begin();

  _canBusDataCharacteristic.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  _canBusDataCharacteristic.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  _canBusDataCharacteristic.begin();

  Bluefruit.setTxPower(+4);
}
#elif defined(ARDUINO_ARCH_ESP32)
void RaceChronoBleAgent::setUp(
    const char *bluetoothName, RaceChronoBleCanHandler *handler) {
  ::handler = handler;
  BTName = bluetoothName;
  BLEDevice::init(BTName);
  BLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  pServer = BLEDevice::createServer();
  pMainService = pServer->createService(MAIN_SERVICE_NAME);

  _pidRequestsCharacteristic = pMainService->createCharacteristic( 
                PID_CHARACTERISTIC, 
                NIMBLE_PROPERTY::WRITE
                );
  _pidRequestsCharacteristic->setCallbacks(&cbhandle_racechrono_filter_request);

  _canBusDataCharacteristic = pMainService->createCharacteristic( 
                CANBUS_CHARACTERISTIC, 
                NIMBLE_PROPERTY::READ |
                NIMBLE_PROPERTY::NOTIFY
                );
  pMainService->start();
  
}
#endif


#if defined(ARDUINO_ARCH_NRF52)
void RaceChronoBleAgent::startAdvertising() {
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

bool RaceChronoBleAgent::isConnected() const {
  return Bluefruit.connected();
}

#elif defined(ARDUINO_ARCH_ESP32)

void RaceChronoBleAgent::startAdvertising() {
  
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName(BTName);
  pAdvertising->setMinInterval(32);
  pAdvertising->setMaxInterval(160);
  pAdvertising->addServiceUUID(pMainService->getUUID());
  pAdvertising->setScanResponse(false);
  pAdvertising->start();
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

bool RaceChronoBleAgent::isConnected() const {
  int32_t connectedCount;
  connectedCount = pServer->getConnectedCount();
  return (connectedCount > 0);
}
#endif


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
#if defined(ARDUINO_ARCH_NRF52)
  _canBusDataCharacteristic.notify(buffer, 4 + len);
#elif defined(ARDUINO_ARCH_ESP32)
  _canBusDataCharacteristic->setValue(buffer, 4 + len);
  _canBusDataCharacteristic->notify();
#endif
}
