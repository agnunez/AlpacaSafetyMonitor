#include "AlpacaSafetyMonitor.h"

void AlpacaSafetyMonitor::registerCallbacks()
{
    AlpacaDevice::registerCallbacks();
    this->createCallBack(LHF(aGetIsSafe), HTTP_GET, "issafe");
}

void AlpacaSafetyMonitor::aGetInterfaceVersion(AsyncWebServerRequest *request) {
    _alpacaServer->respond(request, ALPACA_SAFETYMONITOR_INTERFACE_VERSION);
}
