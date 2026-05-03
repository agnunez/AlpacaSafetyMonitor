#include "DomeDevice.h"

uint8_t DomeDevice::_n_devices = 0;
DomeDevice* DomeDevice::_device_array[4] = { nullptr, nullptr, nullptr, nullptr };

DomeDevice::DomeDevice(DomeController& ctrl)
    : AlpacaDome()
    , _ctrl(ctrl)
{
    _device_index = _n_devices++;
    strlcpy(_device_name, "RollOff Roof", sizeof(_device_name));
    strlcpy(_device_desc, "ASCOM Alpaca Dome (roll-off roof, shutter only)", sizeof(_device_desc));
}

bool DomeDevice::begin()
{
    _device_array[_device_index] = this;
    // Use the persistent UID from NVS
    strlcpy(_device_uid, _ctrl.uniqueId(), sizeof(_device_uid));
    return true;
}

// ----- Capabilities -----
void DomeDevice::aGetCanSetShutter (AsyncWebServerRequest *r) { _alpacaServer->respond(r, true); }
void DomeDevice::aGetCanSetAzimuth (AsyncWebServerRequest *r) { _alpacaServer->respond(r, false); }
void DomeDevice::aGetCanSetAltitude(AsyncWebServerRequest *r) { _alpacaServer->respond(r, false); }
void DomeDevice::aGetCanSetPark    (AsyncWebServerRequest *r) { _alpacaServer->respond(r, false); }
void DomeDevice::aGetCanPark       (AsyncWebServerRequest *r) { _alpacaServer->respond(r, false); }
void DomeDevice::aGetCanFindHome   (AsyncWebServerRequest *r) { _alpacaServer->respond(r, false); }
void DomeDevice::aGetCanSlave      (AsyncWebServerRequest *r) { _alpacaServer->respond(r, false); }
void DomeDevice::aGetCanSyncAzimuth(AsyncWebServerRequest *r) { _alpacaServer->respond(r, false); }

// ----- State -----
void DomeDevice::aGetSlewing(AsyncWebServerRequest *r) {
    // Per spec: roll-off roof with shutter only never reports Slewing
    _alpacaServer->respond(r, false);
}

void DomeDevice::aGetSlaved(AsyncWebServerRequest *r) {
    _alpacaServer->respond(r, false);
}

void DomeDevice::aPutSlaved(AsyncWebServerRequest *r) {
    _alpacaServer->respond(r, nullptr, NotImplemented, "Slaving not supported");
}

void DomeDevice::aGetShutterStatus(AsyncWebServerRequest *r) {
    int status = (int)_ctrl.shutterStatus();
    _alpacaServer->respond(r, status);
}

void DomeDevice::aGetAzimuth(AsyncWebServerRequest *r) {
    _alpacaServer->respond(r, nullptr, NotImplemented, "Azimuth not implemented");
}

void DomeDevice::aGetAltitude(AsyncWebServerRequest *r) {
    _alpacaServer->respond(r, nullptr, NotImplemented, "Altitude not implemented");
}

void DomeDevice::aGetAtHome(AsyncWebServerRequest *r) {
    _alpacaServer->respond(r, false);
}

void DomeDevice::aGetAtPark(AsyncWebServerRequest *r) {
    _alpacaServer->respond(r, false);
}

// ----- Commands -----
void DomeDevice::aPutOpenShutter(AsyncWebServerRequest *r) {
    DomeCommandResult res = _ctrl.openShutter();
    if (res == DomeCommandResult::BlockedBySafety) {
        _alpacaServer->respond(r, nullptr, InvalidOperationException,
                               "Unsafe conditions; opening blocked");
    } else if (res == DomeCommandResult::HardwareNotConfigured) {
        _alpacaServer->respond(r, nullptr, InvalidOperationException,
                               "Dome hardware not configured");
    } else {
        _alpacaServer->respond(r, nullptr);
    }
}

void DomeDevice::aPutCloseShutter(AsyncWebServerRequest *r) {
    DomeCommandResult res = _ctrl.closeShutter();
    if (res == DomeCommandResult::HardwareNotConfigured) {
        _alpacaServer->respond(r, nullptr, InvalidOperationException,
                               "Dome hardware not configured");
    } else {
        _alpacaServer->respond(r, nullptr);
    }
}

void DomeDevice::aPutAbortSlew(AsyncWebServerRequest *r) {
    _ctrl.abortSlew();
    _alpacaServer->respond(r, nullptr);
}

void DomeDevice::aPutSlewToAzimuth(AsyncWebServerRequest *r) {
    _alpacaServer->respond(r, nullptr, NotImplemented, "SlewToAzimuth not implemented");
}

void DomeDevice::aPutSlewToAltitude(AsyncWebServerRequest *r) {
    _alpacaServer->respond(r, nullptr, NotImplemented, "SlewToAltitude not implemented");
}

void DomeDevice::aPutPark(AsyncWebServerRequest *r) {
    _alpacaServer->respond(r, nullptr, NotImplemented, "Park not implemented");
}

void DomeDevice::aPutSetPark(AsyncWebServerRequest *r) {
    _alpacaServer->respond(r, nullptr, NotImplemented, "SetPark not implemented");
}

void DomeDevice::aPutFindHome(AsyncWebServerRequest *r) {
    _alpacaServer->respond(r, nullptr, NotImplemented, "FindHome not implemented");
}

void DomeDevice::aPutSyncToAzimuth(AsyncWebServerRequest *r) {
    _alpacaServer->respond(r, nullptr, NotImplemented, "SyncToAzimuth not implemented");
}

// ----- Custom Actions -----
void DomeDevice::aPutAction(AsyncWebServerRequest *r) {
    char actionBuf[64] = "";
    char paramsBuf[64] = "";
    _alpacaServer->getParam(r, "Action",     actionBuf, sizeof(actionBuf));
    _alpacaServer->getParam(r, "Parameters", paramsBuf, sizeof(paramsBuf));

    if (strcasecmp(actionBuf, "SetSafetyOverride") == 0) {
        bool v = (strcasecmp(paramsBuf, "true") == 0);
        _ctrl.setSafetyOverride(v);
        _alpacaServer->respond(r, v ? "true" : "false");
    } else if (strcasecmp(actionBuf, "GetSafetyOverride") == 0) {
        _alpacaServer->respond(r, _ctrl.safetyOverride() ? "true" : "false");
    } else {
        _alpacaServer->respond(r, nullptr, ActionNotImplementedException,
                               "Action not implemented");
    }
}

// ----- Custom setup page (live JSON debug view) -----
void DomeDevice::_setSetupPage() {
    // Standalone HTML setup page at /setup/v1/dome/0/setup
    char url[64];
    snprintf(url, sizeof(url), "/setup/v1/%s/%d/setup", _device_type, _device_number);

    _alpacaServer->getServerTCP()->on(url, HTTP_GET, [this](AsyncWebServerRequest *request) {
        const char* page =
        "<!doctype html><html><head><meta charset='utf-8'>"
        "<title>Dome Setup</title>"
        "<style>body{font-family:monospace;background:#1a2230;color:#e0e6ed;padding:1em;}"
        "h2{color:#7eb6ff}.kv{margin:0.3em 0}b{color:#9ed1ff}"
        "button{background:#2c4060;color:#fff;border:1px solid #4a6890;padding:0.4em 1em;margin:0.2em;cursor:pointer}"
        "button:hover{background:#3a548b}</style></head><body>"
        "<h2>RollOff Roof &mdash; Live Debug</h2>"
        "<div id='out'>loading...</div>"
        "<div style='margin-top:1em'>"
        "<button onclick='cmd(\"openshutter\")'>Open</button>"
        "<button onclick='cmd(\"closeshutter\")'>Close</button>"
        "<button onclick='cmd(\"abortslew\")'>Abort</button>"
        "</div>"
        "<script>"
        "let txn=1;"
        "async function poll(){"
        "  try{"
        "    const s=await(await fetch('/api/v1/dome/0/shutterstatus?ClientID=1&ClientTransactionID='+(txn++))).json();"
        "    const ovr=await(await fetch('/api/dome')).json();"
        "    const names=['Open','Closed','Opening','Closing','Error'];"
        "    document.getElementById('out').innerHTML="
        "      '<div class=kv>State: <b>'+names[s.Value]+'</b> ('+s.Value+')</div>'+"
        "      '<div class=kv>Endstop Open: <b>'+ovr.endstop_open+'</b></div>'+"
        "      '<div class=kv>Endstop Closed: <b>'+ovr.endstop_closed+'</b></div>'+"
        "      '<div class=kv>Safety Override: <b>'+ovr.safety_override+'</b></div>'+"
        "      '<div class=kv>Move timeout (ms): <b>'+ovr.move_timeout_ms+'</b></div>'+"
        "      '<div class=kv>Reversal (ms): <b>'+ovr.reversal_ms+'</b></div>';"
        "  }catch(e){document.getElementById('out').innerText='ERR '+e;}"
        "}"
        "async function cmd(action){"
        "  const fd=new URLSearchParams();fd.append('ClientID','1');fd.append('ClientTransactionID',txn++);"
        "  await fetch('/api/v1/dome/0/'+action,{method:'PUT',body:fd});"
        "  poll();"
        "}"
        "poll();setInterval(poll,2000);"
        "</script></body></html>";
        request->send(200, "text/html", page);
    });

    // Also serve the standard /setup/v1/dome/0 URL for jsonFormer config
    snprintf(url, sizeof(url), "/setup/v1/%s/%d", _device_type, _device_number);
    _alpacaServer->getServerTCP()->serveStatic(url, SPIFFS, "/www/setup.html");

    // Register the jsondata GET / POST endpoints for the jsonFormer Devices tab
    snprintf(url, sizeof(url), ALPACA_DEVICE_COMMAND, _device_type, _device_number, "jsondata");
    this->createCallBack(LHF(_getJsondata), HTTP_GET, "jsondata", false);
    AsyncCallbackJsonWebHandler* jsonhandler = new AsyncCallbackJsonWebHandler(url,
        [this](AsyncWebServerRequest *request, JsonVariant& json) {
            JsonObject jsonObj = json.as<JsonObject>();
            this->aReadJson(jsonObj);
            request->send(200, F("application/json"), F("{\"received\":\"true\"}"));
        });
    _alpacaServer->getServerTCP()->addHandler(jsonhandler);
}

// ----- jsonFormer integration -----
void DomeDevice::aReadJson(JsonObject& root) {
    AlpacaDome::aReadJson(root);

    // Allow the jsonFormer Configuration block to toggle the safety override.
    // Pin/timeout changes are managed via the /api/dome REST endpoint (require reboot).
    if (JsonObject obj_config = root[F("Configuration")]) {
        if (obj_config.containsKey(F("Safety_Override"))) {
            bool v = obj_config[F("Safety_Override")] | _ctrl.safetyOverride();
            _ctrl.setSafetyOverride(v);
        }
    }
}

void DomeDevice::aWriteJson(JsonObject& root) {
    AlpacaDome::aWriteJson(root);

    JsonObject obj_config = root.createNestedObject(F("Configuration"));
    obj_config[F("Safety_Override")] = _ctrl.safetyOverride();

    JsonObject obj_state = root.createNestedObject(F("State"));
    int s = (int)_ctrl.shutterStatus();
    const char* names[] = {"Open","Closed","Opening","Closing","Error"};
    obj_state[F("#Shutter_Status")]   = (s >= 0 && s <= 4) ? names[s] : "Unknown";
    obj_state[F("#Endstop_Open")]     = _ctrl.endstopOpen();
    obj_state[F("#Endstop_Closed")]   = _ctrl.endstopClosed();
}
