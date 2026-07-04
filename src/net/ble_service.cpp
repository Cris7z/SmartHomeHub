#include "ble_service.h"

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include "../app/controls.h"
#include "../app/event_log.h"
#include "../app/hub_state.h"
#include "../io/mic_processing.h"

namespace {
constexpr char BLE_DEVICE_NAME[] = "SmartHomeHub";
constexpr char BLE_SERVICE_UUID[] = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char BLE_STATE_UUID[] = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char BLE_COMMAND_UUID[] = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";

BLECharacteristic *stateCharacteristic = nullptr;
uint32_t lastBleNotifyMs = 0;

String boolJson(bool value) {
  return value ? "true" : "false";
}

String stateJson() {
  String json;
  json.reserve(860);
  json += "{";
  json += "\"tempC\":" + String(state.tempC, 1) + ",";
  json += "\"humidity\":" + String(state.humidity, 0) + ",";
  json += "\"presence\":" + boolJson(state.presence) + ",";
  json += "\"soundTriggered\":" + boolJson(state.soundTriggered) + ",";
  json += "\"micLevel\":" + String((long)state.micLevel) + ",";
  json += "\"micBaseline\":" + String((long)state.micBaseline) + ",";
  json += "\"micThreshold\":" + String((long)state.micThreshold) + ",";
  json += "\"micPercent\":" + String(noisePercentFor(state.micLevel, state.micThreshold)) + ",";
  json += "\"irReceived\":" + boolJson(state.irReceived) + ",";
  json += "\"securityArmed\":" + boolJson(state.securityArmed) + ",";
  json += "\"alarm\":" + boolJson(state.alarm) + ",";
  json += "\"forceSecurity\":" + boolJson(state.forceSecurity) + ",";
  json += "\"manualLamp\":" + boolJson(state.manualLamp) + ",";
  json += "\"lampOverride\":" + boolJson(state.lampOverride) + ",";
  json += "\"manualAc\":" + boolJson(state.manualAc) + ",";
  json += "\"acCooling\":" + boolJson(state.acCooling) + ",";
  json += "\"irTestActive\":" + boolJson(state.irTestActive) + ",";
  json += "\"displayPage\":" + String(state.displayPage) + ",";
  json += "\"aiRiskScore\":" + String(state.aiRiskScore) + ",";
  json += "\"aiRisk\":\"" + String(state.aiRiskText) + "\",";
  json += "\"bleClientConnected\":" + boolJson(state.bleClientConnected) + ",";
  json += "\"timeReady\":" + boolJson(state.timeReady) + ",";
  json += "\"time\":\"" + String(state.timeText) + "\",";
  json += "\"date\":\"" + String(state.dateText) + "\",";
  json += "\"weatherReady\":" + boolJson(state.weatherReady) + ",";
  json += "\"locationReady\":" + boolJson(state.locationReady) + ",";
  json += "\"location\":\"" + String(state.locationText) + "\",";
  json += "\"timezone\":\"" + String(state.timezoneText) + "\",";
  json += "\"outdoorTempC\":" + String(state.outdoorTempC, 1) + ",";
  json += "\"weather\":\"" + String(state.weatherText) + "\",";
  json += "\"windKph\":" + String(state.windKph, 1) + ",";
  json += "\"sunrise\":\"" + String(state.sunriseText) + "\",";
  json += "\"sunset\":\"" + String(state.sunsetText) + "\",";
  json += "\"events\":[";
  for (uint8_t i = 0; i < state.eventLogCount; i++) {
    if (i) json += ",";
    json += "\"" + String(eventLogLine(i)) + "\"";
  }
  json += "]";
  json += "}";
  return json;
}

bool runCommand(const String &command) {
  Serial.printf("[BLE] RX command: '%s'\n", command.c_str());
  if (command == "security") {
    applyHubCommand(HubCommand::ToggleSecurity, "BLE");
  } else if (command == "lamp") {
    applyHubCommand(HubCommand::ToggleLamp, "BLE");
  } else if (command == "ac") {
    applyHubCommand(HubCommand::ToggleAc, "BLE");
  } else if (command == "mode" || command == "page" || command == "buzzer") {
    applyHubCommand(HubCommand::NextDisplayPage, "BLE");
  } else if (command == "clear") {
    applyHubCommand(HubCommand::ClearAlarmSecurity, "BLE");
  } else {
    Serial.println("[BLE] Unknown command");
    return false;
  }
  return true;
}

class HubServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *) override {
    state.bleClientConnected = true;
  }

  void onDisconnect(BLEServer *server) override {
    state.bleClientConnected = false;
    server->getAdvertising()->start();
  }
};

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String command = characteristic->getValue().c_str();
    command.trim();
    command.toLowerCase();
    runCommand(command);
  }
};
}

void setupBleService() {
  BLEDevice::init(BLE_DEVICE_NAME);
  BLEServer *bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new HubServerCallbacks());
  BLEService *service = bleServer->createService(BLE_SERVICE_UUID);

  stateCharacteristic = service->createCharacteristic(
    BLE_STATE_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  stateCharacteristic->addDescriptor(new BLE2902());
  stateCharacteristic->setValue(stateJson().c_str());

  BLECharacteristic *commandCharacteristic = service->createCharacteristic(
    BLE_COMMAND_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  commandCharacteristic->setCallbacks(new CommandCallbacks());

  service->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  BLEAdvertisementData advertisementData;
  BLEAdvertisementData scanResponseData;
  advertisementData.setFlags(0x06);
  advertisementData.setCompleteServices(BLEUUID(BLE_SERVICE_UUID));
  scanResponseData.setName(BLE_DEVICE_NAME);
  advertising->setAdvertisementData(advertisementData);
  advertising->setScanResponseData(scanResponseData);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.printf("BLE: %s advertising\n", BLE_DEVICE_NAME);
}

void updateBleService() {
  if (!stateCharacteristic || millis() - lastBleNotifyMs < 2000) return;

  lastBleNotifyMs = millis();
  const String json = stateJson();
  stateCharacteristic->setValue(json.c_str());
  if (state.bleClientConnected) {
    stateCharacteristic->notify();
  }
}
