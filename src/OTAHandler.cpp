#include "OTAHandler.h"
#include <Ticker.h>

// ESP32 firmware magic byte
static constexpr uint8_t ESP32_MAGIC_BYTE = 0xE9;

// Ticker for delayed restart after OTA
static Ticker restartTicker;

static void doRestart()
{
    Serial.println(F("# OTAHandler: Restarting..."));
    ESP.restart();
}

void OTAHandler::registerEndpoints(AsyncWebServer* server)
{
    // GET /api/ota/version - return current firmware version
    server->on("/api/ota/version", HTTP_GET, [](AsyncWebServerRequest* request) {
        char json[64];
        snprintf(json, sizeof(json), "{\"version\":\"%s\"}", FW_VERSION);
        request->send(200, "application/json", json);
    });

    // POST /api/ota/upload - multipart firmware upload
    server->on("/api/ota/upload", HTTP_POST,
        // Request handler (called when upload completes)
        [](AsyncWebServerRequest* request) {
            if (Update.hasError()) {
                char errMsg[128];
                snprintf(errMsg, sizeof(errMsg),
                         "{\"success\":false,\"error\":\"Update failed: %s\"}",
                         Update.errorString());
                request->send(500, "application/json", errMsg);
            } else {
                request->send(200, "application/json",
                              "{\"success\":true,\"message\":\"Update successful, restarting...\"}");
                // Schedule restart after 1 second to allow response to be sent
                restartTicker.once_ms(1000, doRestart);
            }
        },
        // Upload handler (called for each chunk)
        [](AsyncWebServerRequest* request, const String& filename, size_t index,
           uint8_t* data, size_t len, bool final) {

            if (index == 0) {
                // First chunk: validate magic byte
                Serial.printf("# OTAHandler: Upload started: '%s'\n", filename.c_str());

                if (len == 0) {
                    Serial.println(F("# OTAHandler: Empty upload"));
                    Update.abort();
                    return;
                }

                if (data[0] != ESP32_MAGIC_BYTE) {
                    Serial.printf("# OTAHandler: Invalid magic byte 0x%02X (expected 0x%02X)\n",
                                  data[0], ESP32_MAGIC_BYTE);
                    Update.abort();
                    return;
                }

                // Begin update with max sketch size
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Serial.printf("# OTAHandler: Update.begin() failed: %s\n",
                                  Update.errorString());
                    return;
                }

                Serial.println(F("# OTAHandler: Update started"));
            }

            // Stream data to Update library
            if (Update.isRunning()) {
                if (Update.write(data, len) != len) {
                    Serial.printf("# OTAHandler: Write failed: %s\n",
                                  Update.errorString());
                    Update.abort();
                    return;
                }
            }

            if (final) {
                if (Update.end(true)) {
                    Serial.printf("# OTAHandler: Upload complete, %u bytes\n",
                                  index + len);
                } else {
                    Serial.printf("# OTAHandler: Update.end() failed: %s\n",
                                  Update.errorString());
                }
            }
        });
}

const char* OTAHandler::currentVersion() const
{
    return FW_VERSION;
}
