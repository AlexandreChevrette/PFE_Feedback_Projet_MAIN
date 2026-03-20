
#include "Arduino.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#ifndef BLUETOOTH_H_
#define BLUETOOTH_H_

#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcd1234-ab12-ab12-ab12-abcdef123456"




class Bluetooth {
public:
    bool deviceConnected = false; 
    Bluetooth() = default;
    void setup();
    void send(const String& data);

private:
    BLEServer*         m_Server         = nullptr;
    BLEService*        m_Service        = nullptr;
    BLECharacteristic* m_Characteristic = nullptr;
};


#endif