
#include "Arduino.h"
#include "BluetoothComm.h"
#include "FeedbackControl.h"

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

// receive data
class CharacteristicCallbacks : public BLECharacteristicCallbacks {
    public:
        explicit CharacteristicCallbacks(FeedbackControl* fc) : m_fc(fc) {}

        void onWrite(BLECharacteristic* pChar) override {
            Serial.println("Received Data");
            String value = pChar->getValue().c_str();
            if (value.length() == 0) return;

            // ── SET_SETPOINTS:v0,v1,v2 ────────────────────────────────────────
            if (value.startsWith("SET_SETPOINTS:")) {
                String csv = value.substring(14);
                size_t motorIndex = 1;
                int start = 0;
                int comma = csv.indexOf(',');

                while (comma != -1 && motorIndex < numberOfChannels+1) {
                    float val = csv.substring(start, comma).toFloat();
                    m_fc->updateSetpoint(motorIndex++, val);
                    start = comma + 1;
                    comma = csv.indexOf(',', start);
                }
                // last value (no trailing comma)
                if (motorIndex < numberOfChannels+1) {
                    m_fc->updateSetpoint(motorIndex, csv.substring(start).toFloat());
                }
            }

            // ── SET_PID:p,i,d ─────────────────────────────────────────────────
            else if (value.startsWith("SET_PID:")) {
                String csv = value.substring(8);
                float gains[3] = {0, 0, 0};
                int idx = 0, start = 0;
                int comma = csv.indexOf(',');

                while (comma != -1 && idx < 3) {
                    gains[idx++] = csv.substring(start, comma).toFloat();
                    start = comma + 1;
                    comma = csv.indexOf(',', start);
                }
                if (idx < 3) gains[idx] = csv.substring(start).toFloat();

                m_fc->setProportional(gains[0]);
                m_fc->setIntegral(gains[1]);
                m_fc->setDerivative(gains[2]);
            }

        }

    private:
        FeedbackControl* m_fc;
};

// --- Bluetooth setup ---

void Bluetooth::setup(FeedbackControl* p_feedbackControl) {
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
    m_Characteristic->setCallbacks(new CharacteristicCallbacks(p_feedbackControl));
    m_Characteristic->setValue("Hello from ESP32");

    m_Service->start();
    BLEDevice::startAdvertising();
    Serial.println("BLE ready, waiting for connections...");
}

void Bluetooth::send(const String& data) {
    if (!deviceConnected) return;

    unsigned long now = millis();
    if (now - m_lastSendTime < 10) return;  // 100Hz max
    m_lastSendTime = now;

    m_Characteristic->setValue(data.c_str());
    m_Characteristic->notify();
}
