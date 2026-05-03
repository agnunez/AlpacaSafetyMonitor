#include "WiFiManager.h"
#include <ArduinoJson.h>
#include <esp_wifi.h>

WiFiManagerCustom::WiFiManagerCustom(SettingsManager& settings)
    : _settings(settings)
    , _state(WiFiState::DISCONNECTED)
    , _credCount(0)
    , _buttonPressStart(0)
    , _buttonWasPressed(false)
    , _lastReconnectAttempt(0)
    , _lastLedToggle(0)
    , _ledOn(false)
    , _pendingConnect(false)
    , _pendingConnectTime(0)
{
    memset(_pendingSsid, 0, sizeof(_pendingSsid));
    memset(_pendingPassword, 0, sizeof(_pendingPassword));
}

void WiFiManagerCustom::begin()
{
    // Configure LED pin
    int8_t ledPin = _settings.hardware().wifi_led_pin;
    if (ledPin >= 0) {
        pinMode(ledPin, OUTPUT);
        digitalWrite(ledPin, LOW);
    }

    // Configure boot button pin
    pinMode(WIFI_BOOT_BUTTON_PIN, INPUT_PULLUP);

    // Load WiFi credentials from NVS
    _settings.loadWiFiCredentials(_creds, _credCount);
    _sortCredentialsByPriority();

    if (_credCount == 0) {
        Serial.println(F("# WiFiManager: No stored credentials, starting AP mode"));
        startCaptivePortal();
        return;
    }

    // Attempt to connect to each network in priority order
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);   // we handle reconnect ourselves
    _state = WiFiState::CONNECTING;

    for (uint8_t i = 0; i < _credCount; i++) {
        Serial.printf("# WiFiManager: Trying network '%s' (priority %d)\n",
                      _creds[i].ssid, _creds[i].priority);

        for (int attempt = 0; attempt < 2; attempt++) {
            if (_connectToNetwork(_creds[i].ssid, _creds[i].password)) {
                _state = WiFiState::CONNECTED;
                Serial.printf("# WiFiManager: Connected to '%s', IP: %s\n",
                              _creds[i].ssid,
                              WiFi.localIP().toString().c_str());
                _setupMdns();
                _updateLed();
                return;
            }
            Serial.printf("# WiFiManager: Attempt %d failed for '%s'\n",
                          attempt + 1, _creds[i].ssid);
        }
    }

    // All networks failed, fall back to AP mode
    Serial.println(F("# WiFiManager: All networks failed, starting AP mode"));
    startCaptivePortal();
}

void WiFiManagerCustom::loop()
{
    // --- Boot button long-press detection (GPIO 0) ---
    bool buttonPressed = (digitalRead(WIFI_BOOT_BUTTON_PIN) == LOW);

    if (buttonPressed && !_buttonWasPressed) {
        _buttonPressStart = millis();
        _buttonWasPressed = true;
    } else if (buttonPressed && _buttonWasPressed) {
        if ((millis() - _buttonPressStart) >= WIFI_LONG_PRESS_MS) {
            if (_state != WiFiState::AP_MODE) {
                Serial.println(F("# WiFiManager: Boot button long press - starting AP mode"));
                startCaptivePortal();
            }
            _buttonWasPressed = false;
        }
    } else if (!buttonPressed && _buttonWasPressed) {
        _buttonWasPressed = false;
    }

    // --- Deferred WiFi connect (set by POST /api/wifi/connect) ---
    if (_pendingConnect && millis() - _pendingConnectTime >= 1500) {
        _pendingConnect = false;

        Serial.printf("# WiFiManager: Deferred connect to '%s' (pass len=%d)\n",
                      _pendingSsid, (int)strlen(_pendingPassword));

        // Save credentials FIRST so they persist even if connect fails
        _addCredential(_pendingSsid, _pendingPassword);

        if (_connectToNetwork(_pendingSsid, _pendingPassword)) {
            _state = WiFiState::CONNECTED;
            Serial.printf("# WiFiManager: Connected to '%s', IP: %s\n",
                          _pendingSsid, WiFi.localIP().toString().c_str());
            _dnsServer.stop();
            _setupMdns();
        } else {
            Serial.printf("# WiFiManager: Failed to connect to '%s'\n", _pendingSsid);
            // Restart AP so user can retry
            _state = WiFiState::AP_MODE;
            WiFi.mode(WIFI_AP_STA);
            WiFi.softAP(_settings.hardware().hostname);
            _dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
        }
    }

    // --- DNS handling in AP mode ---
    if (_state == WiFiState::AP_MODE) {
        _dnsServer.processNextRequest();
    }

    // --- Reconnect if disconnected (not in AP mode) ---
    if (_state == WiFiState::CONNECTED && WiFi.status() != WL_CONNECTED) {
        _state = WiFiState::DISCONNECTED;
        Serial.println(F("# WiFiManager: Connection lost"));
    }

    if (_state == WiFiState::DISCONNECTED) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > WIFI_CONNECT_TIMEOUT_MS) {
            _lastReconnectAttempt = now;
            Serial.println(F("# WiFiManager: Attempting reconnect..."));

            for (uint8_t i = 0; i < _credCount; i++) {
                if (_connectToNetwork(_creds[i].ssid, _creds[i].password)) {
                    _state = WiFiState::CONNECTED;
                    Serial.printf("# WiFiManager: Reconnected to '%s'\n", _creds[i].ssid);
                    _setupMdns();
                    break;
                }
            }
        }
    }

    // --- LED update ---
    _updateLed();
}

void WiFiManagerCustom::startCaptivePortal()
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.setAutoReconnect(false);
    WiFi.softAP(_settings.hardware().hostname);
    Serial.printf("# WiFiManager: AP started, SSID: '%s', IP: 192.168.4.1\n",
                  _settings.hardware().hostname);

    _dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    _state = WiFiState::AP_MODE;
    _updateLed();
}

void WiFiManagerCustom::stopCaptivePortal()
{
    _dnsServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    _state = WiFiState::DISCONNECTED;
    Serial.println(F("# WiFiManager: Captive portal stopped"));
}

void WiFiManagerCustom::registerEndpoints(AsyncWebServer* server)
{
    // GET /api/wifi/scan - scan for available networks (non-blocking)
    server->on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
        int n = WiFi.scanComplete();

        if (n == WIFI_SCAN_FAILED) {
            WiFi.scanNetworks(true);  // async
            request->send(200, "application/json", "{\"networks\":[],\"scanning\":true}");
            return;
        }

        if (n == WIFI_SCAN_RUNNING) {
            request->send(200, "application/json", "{\"networks\":[],\"scanning\":true}");
            return;
        }

        DynamicJsonDocument doc(2048);
        JsonObject root = doc.to<JsonObject>();
        JsonArray arr = root.createNestedArray("networks");

        for (int i = 0; i < n; i++) {
            JsonObject net = arr.createNestedObject();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
            net["open"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
        }
        root["scanning"] = false;
        WiFi.scanDelete();

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // GET /api/wifi/status
    server->on("/api/wifi/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        DynamicJsonDocument doc(512);
        const char* stateStr = "unknown";
        switch (_state) {
            case WiFiState::DISCONNECTED: stateStr = "disconnected"; break;
            case WiFiState::CONNECTING:   stateStr = "connecting"; break;
            case WiFiState::CONNECTED:    stateStr = "connected"; break;
            case WiFiState::AP_MODE:      stateStr = "ap_mode"; break;
        }
        doc["state"] = stateStr;
        doc["ssid"] = WiFi.SSID();
        doc["ip"] = (_state == WiFiState::AP_MODE)
            ? WiFi.softAPIP().toString()
            : WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
        doc["hostname"] = _settings.hardware().hostname;

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // GET /api/wifi/networks - stored networks (no passwords)
    server->on("/api/wifi/networks", HTTP_GET, [this](AsyncWebServerRequest* request) {
        // Use cached credentials to avoid NVS reads on every request
        DynamicJsonDocument doc(1024);
        JsonArray arr = doc.to<JsonArray>();
        for (uint8_t i = 0; i < _credCount; i++) {
            JsonObject net = arr.createNestedObject();
            net["ssid"] = _creds[i].ssid;
            net["priority"] = _creds[i].priority;
        }

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // POST /api/wifi/connect - schedule connection (non-blocking)
    server->on("/api/wifi/connect", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            // Response sent from body handler
        },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            // Guard: ignore if a connect is already pending
            if (_pendingConnect) {
                request->send(200, "application/json",
                    "{\"success\":true,\"message\":\"Connection already in progress\"}");
                return;
            }

            DynamicJsonDocument doc(512);
            DeserializationError err = deserializeJson(doc, data, len);
            if (err) {
                Serial.printf("# WiFiManager: JSON parse error: %s\n", err.c_str());
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            const char* ssid = doc["ssid"];
            const char* password = doc["password"] | "";

            if (!ssid || strlen(ssid) == 0) {
                request->send(400, "application/json", "{\"error\":\"SSID required\"}");
                return;
            }

            Serial.printf("# WiFiManager: Connect scheduled for '%s' (pass len=%d)\n",
                          ssid, (int)strlen(password));

            strlcpy(_pendingSsid, ssid, sizeof(_pendingSsid));
            strlcpy(_pendingPassword, password, sizeof(_pendingPassword));
            _pendingConnect = true;
            _pendingConnectTime = millis();

            request->send(200, "application/json", "{\"success\":true,\"message\":\"Connecting...\"}");
        });

    // POST /api/wifi/networks - save network list
    server->on("/api/wifi/networks", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(1024);
            DeserializationError err = deserializeJson(doc, data, len);
            if (err) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            JsonArray arr = doc.as<JsonArray>();
            if (arr.isNull()) {
                request->send(400, "application/json", "{\"error\":\"Expected JSON array\"}");
                return;
            }

            WiFiCredential creds[MAX_WIFI_NETWORKS];
            uint8_t count = 0;
            for (JsonObject net : arr) {
                if (count >= MAX_WIFI_NETWORKS) break;
                strlcpy(creds[count].ssid, net["ssid"] | "", sizeof(creds[count].ssid));
                strlcpy(creds[count].password, net["password"] | "", sizeof(creds[count].password));
                creds[count].priority = net["priority"] | (int8_t)count;
                count++;
            }

            _settings.saveWiFiCredentials(creds, count);
            _credCount = count;
            memcpy(_creds, creds, sizeof(creds));
            _sortCredentialsByPriority();

            request->send(200, "application/json", "{\"success\":true}");
        });

    // DELETE /api/wifi/networks - remove a stored network
    server->on("/api/wifi/networks", HTTP_DELETE,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(256);
            DeserializationError err = deserializeJson(doc, data, len);
            if (err) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            const char* ssidToRemove = doc["ssid"];
            if (!ssidToRemove || strlen(ssidToRemove) == 0) {
                request->send(400, "application/json", "{\"error\":\"SSID required\"}");
                return;
            }

            WiFiCredential updated[MAX_WIFI_NETWORKS];
            uint8_t newCount = 0;
            bool found = false;

            for (uint8_t i = 0; i < _credCount; i++) {
                if (strcmp(_creds[i].ssid, ssidToRemove) == 0) {
                    found = true;
                    continue;
                }
                updated[newCount++] = _creds[i];
            }

            if (!found) {
                request->send(404, "application/json", "{\"error\":\"Network not found\"}");
                return;
            }

            _settings.saveWiFiCredentials(updated, newCount);
            _credCount = newCount;
            memcpy(_creds, updated, sizeof(updated));
            _sortCredentialsByPriority();

            request->send(200, "application/json", "{\"success\":true}");
        });
}

bool WiFiManagerCustom::isConnected() const
{
    return _state == WiFiState::CONNECTED && WiFi.status() == WL_CONNECTED;
}

WiFiState WiFiManagerCustom::getState() const
{
    return _state;
}

IPAddress WiFiManagerCustom::getIP() const
{
    if (_state == WiFiState::AP_MODE) {
        return WiFi.softAPIP();
    }
    return WiFi.localIP();
}

bool WiFiManagerCustom::_connectToNetwork(const char* ssid, const char* password)
{
    _state = WiFiState::CONNECTING;

    Serial.printf("# WiFiManager: Connecting to '%s' (pass len=%d)\n",
                  ssid, (int)strlen(password));

    // If already connected to this SSID, skip the disconnect/reconnect dance
    if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == ssid) {
        Serial.printf("# WiFiManager: Already connected to '%s'\n", ssid);
        return true;
    }

    // Full STA disconnect and clear at the ESP-IDF level
    WiFi.disconnect(true);  // true = erase STA config from ESP-IDF
    delay(100);

    // Set the right mode: AP_STA if AP is running, STA otherwise
    wifi_mode_t currentMode;
    esp_wifi_get_mode(&currentMode);
    if (currentMode == WIFI_MODE_AP || currentMode == WIFI_MODE_APSTA) {
        WiFi.mode(WIFI_AP_STA);
    } else {
        WiFi.mode(WIFI_STA);
    }
    delay(100);

    // Start the connection
    WiFi.begin(ssid, password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
            wl_status_t st = WiFi.status();
            Serial.printf("# WiFiManager: Timeout connecting to '%s' (status=%d)\n",
                          ssid, (int)st);
            WiFi.disconnect(false);
            return false;
        }
        delay(250);
        // Print a dot every second for progress
        if (((millis() - start) % 1000) < 250) {
            Serial.print(".");
        }
    }
    Serial.println();
    return true;
}

void WiFiManagerCustom::_addCredential(const char* ssid, const char* password)
{
    // Check if SSID already exists, update password if so
    for (uint8_t i = 0; i < _credCount; i++) {
        if (strcmp(_creds[i].ssid, ssid) == 0) {
            strlcpy(_creds[i].password, password, sizeof(_creds[i].password));
            _settings.saveWiFiCredentials(_creds, _credCount);
            Serial.printf("# WiFiManager: Updated credentials for '%s'\n", ssid);
            return;
        }
    }

    // Add new credential
    if (_credCount < MAX_WIFI_NETWORKS) {
        strlcpy(_creds[_credCount].ssid, ssid, sizeof(_creds[_credCount].ssid));
        strlcpy(_creds[_credCount].password, password, sizeof(_creds[_credCount].password));
        _creds[_credCount].priority = _credCount;
        _credCount++;
    } else {
        uint8_t slot = MAX_WIFI_NETWORKS - 1;
        strlcpy(_creds[slot].ssid, ssid, sizeof(_creds[slot].ssid));
        strlcpy(_creds[slot].password, password, sizeof(_creds[slot].password));
        _creds[slot].priority = slot;
    }

    _settings.saveWiFiCredentials(_creds, _credCount);
    Serial.printf("# WiFiManager: Saved credentials for '%s' (total: %d)\n", ssid, _credCount);
}

void WiFiManagerCustom::_sortCredentialsByPriority()
{
    for (uint8_t i = 1; i < _credCount; i++) {
        WiFiCredential key = _creds[i];
        int8_t j = i - 1;
        while (j >= 0 && _creds[j].priority > key.priority) {
            _creds[j + 1] = _creds[j];
            j--;
        }
        _creds[j + 1] = key;
    }
}

void WiFiManagerCustom::_updateLed()
{
    int8_t ledPin = _settings.hardware().wifi_led_pin;
    if (ledPin < 0) return;

    switch (_state) {
        case WiFiState::CONNECTED:
            digitalWrite(ledPin, HIGH);
            _ledOn = true;
            break;

        case WiFiState::AP_MODE: {
            unsigned long now = millis();
            if (now - _lastLedToggle >= 200) {
                _lastLedToggle = now;
                _ledOn = !_ledOn;
                digitalWrite(ledPin, _ledOn ? HIGH : LOW);
            }
            break;
        }

        case WiFiState::CONNECTING: {
            unsigned long now = millis();
            if (now - _lastLedToggle >= 500) {
                _lastLedToggle = now;
                _ledOn = !_ledOn;
                digitalWrite(ledPin, _ledOn ? HIGH : LOW);
            }
            break;
        }

        case WiFiState::DISCONNECTED:
        default:
            digitalWrite(ledPin, LOW);
            _ledOn = false;
            break;
    }
}

void WiFiManagerCustom::_setupMdns()
{
    MDNS.end();
    if (MDNS.begin(_settings.hardware().hostname)) {
        Serial.printf("# WiFiManager: mDNS started as '%s.local'\n",
                      _settings.hardware().hostname);
    } else {
        Serial.println(F("# WiFiManager: mDNS start failed"));
    }
}
