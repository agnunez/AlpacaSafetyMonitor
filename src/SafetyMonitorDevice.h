#pragma once
#include <Arduino.h>
#include "AlpacaSafetyMonitor.h"
#include "SafetyEngine.h"
#include "DomeController.h"

class SafetyMonitorDevice : public AlpacaSafetyMonitor {
private:
    static uint8_t _n_devices;
    static SafetyMonitorDevice* _device_array[4];
    uint8_t _device_index;

    SafetyEngine&   _engine;
    DomeController& _dome;
    bool _issafe = false;

public:
    SafetyMonitorDevice(SafetyEngine& engine, DomeController& dome);

    bool begin();

    // Called by the main loop after SafetyEngine::update() to propagate state
    void updateSafetyState(bool safe);

    // ASCOM Alpaca getter
    void aGetIsSafe(AsyncWebServerRequest* request);

    // JSON serialization for /setup web page (compatible with jsonFormer)
    void aReadJson(JsonObject& root);
    void aWriteJson(JsonObject& root);
};
