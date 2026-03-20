
#include "Arduino.h"
#include "BluetoothComm.h"


class ServerCallbacks : public BLEServerCallbacks {
public:
    explicit ServerCallbacks(Bluetooth* bt) : m_bt(bt) {}

    void onConnect(BLEServer* pServer) override {
        m_bt->deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer) override {
        m_bt->deviceConnected = false;
        pServer->startAdvertising();
    }

private:
    Bluetooth* m_bt;
};

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pChar) override {
        String value = pChar->getValue().c_str();
        if (value.length() > 0) {
            Serial.print("Received: ");
            Serial.println(value);
            pChar->setValue(("Echo: " + value).c_str());
            pChar->notify();
        }
    }
};

// --- Bluetooth setup ---

void Bluetooth::setup() {
    BLEDevice::init("ESP32-BLE");
    BLEDevice::setMTU(185); // send byte limit to 185 bytes
    m_Server = BLEDevice::createServer();
    m_Server->setCallbacks(new ServerCallbacks(this));

    m_Service = m_Server->createService(SERVICE_UUID);
    m_Characteristic = m_Service->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ  |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    m_Characteristic->addDescriptor(new BLE2902());
    m_Characteristic->setCallbacks(new CharacteristicCallbacks());
    m_Characteristic->setValue("Hello from ESP32");

    m_Service->start();
    BLEDevice::startAdvertising();
    Serial.println("BLE ready, waiting for connections...");
}

void Bluetooth::send(const String& data) {
    if (deviceConnected) {
        m_Characteristic->setValue(data.c_str());
        m_Characteristic->notify();
    }
}