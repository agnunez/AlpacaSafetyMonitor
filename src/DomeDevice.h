#pragma once
#include <Arduino.h>
#include "AlpacaDome.h"
#include "DomeController.h"

class DomeDevice : public AlpacaDome {
private:
    static uint8_t _n_devices;
    static DomeDevice* _device_array[4];
    uint8_t _device_index;

    DomeController& _ctrl;

public:
    explicit DomeDevice(DomeController& ctrl);

    bool begin();

    // Capability getters (constants for shutter-only roll-off roof)
    void aGetCanSetShutter   (AsyncWebServerRequest *request) override;
    void aGetCanSetAzimuth   (AsyncWebServerRequest *request) override;
    void aGetCanSetAltitude  (AsyncWebServerRequest *request) override;
    void aGetCanSetPark      (AsyncWebServerRequest *request) override;
    void aGetCanPark         (AsyncWebServerRequest *request) override;
    void aGetCanFindHome     (AsyncWebServerRequest *request) override;
    void aGetCanSlave        (AsyncWebServerRequest *request) override;
    void aGetCanSyncAzimuth  (AsyncWebServerRequest *request) override;

    // State getters
    void aGetSlewing         (AsyncWebServerRequest *request) override;
    void aGetSlaved          (AsyncWebServerRequest *request) override;
    void aPutSlaved          (AsyncWebServerRequest *request) override;
    void aGetShutterStatus   (AsyncWebServerRequest *request) override;
    void aGetAzimuth         (AsyncWebServerRequest *request) override;
    void aGetAltitude        (AsyncWebServerRequest *request) override;
    void aGetAtHome          (AsyncWebServerRequest *request) override;
    void aGetAtPark          (AsyncWebServerRequest *request) override;

    // Commands
    void aPutOpenShutter     (AsyncWebServerRequest *request) override;
    void aPutCloseShutter    (AsyncWebServerRequest *request) override;
    void aPutAbortSlew       (AsyncWebServerRequest *request) override;
    void aPutSlewToAzimuth   (AsyncWebServerRequest *request) override;
    void aPutSlewToAltitude  (AsyncWebServerRequest *request) override;
    void aPutPark            (AsyncWebServerRequest *request) override;
    void aPutSetPark         (AsyncWebServerRequest *request) override;
    void aPutFindHome        (AsyncWebServerRequest *request) override;
    void aPutSyncToAzimuth   (AsyncWebServerRequest *request) override;

    // Custom Actions for safety override
    void aPutAction(AsyncWebServerRequest *request) override;

    // Custom setup page
    void _setSetupPage() override;

    // jsonFormer integration
    void aReadJson(JsonObject& root) override;
    void aWriteJson(JsonObject& root) override;
};
