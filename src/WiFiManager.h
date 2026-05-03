#pragma once

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include "SettingsManager.h"
#include "Config.h"

enum class WiFiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    AP_MODE
};

class WiFiManagerCustom {
public:
    explicit WiFiManagerCustom(SettingsManager& settings);

    void begin();
    void loop();

    void startCaptivePortal();
    void stopCaptivePortal();

    void registerEndpoints(AsyncWebServer* server);

    bool isConnected() const;
    WiFiState getState() const;
    IPAddress getIP() const;

private:
    SettingsManager& _settings;
    DNSServer _dnsServer;
    WiFiState _state;

    WiFiCredential _creds[MAX_WIFI_NETWORKS];
    uint8_t _credCount;

    // Boot button long-press detection
    unsigned long _buttonPressStart;
    bool _buttonWasPressed;

    // Reconnect timing
    unsigned long _lastReconnectAttempt;

    // LED blinking
    unsigned long _lastLedToggle;
    bool _ledOn;

    // Deferred connect (set by POST handler, executed in loop)
    bool _pendingConnect;
    unsigned long _pendingConnectTime;
    char _pendingSsid[33];
    char _pendingPassword[65];

    bool _connectToNetwork(const char* ssid, const char* password);
    void _addCredential(const char* ssid, const char* password);
    void _sortCredentialsByPriority();
    void _updateLed();
    void _setupMdns();
};
