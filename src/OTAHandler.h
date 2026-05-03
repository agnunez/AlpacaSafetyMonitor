#pragma once

#include <Arduino.h>
#include <Update.h>
#include <ESPAsyncWebServer.h>
#include "version.h"

class OTAHandler {
public:
    void registerEndpoints(AsyncWebServer* server);
    const char* currentVersion() const;
};
