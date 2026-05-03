#include "AlpacaDome.h"

void AlpacaDome::registerCallbacks()
{
    AlpacaDevice::registerCallbacks();

    // Capabilities (read-only)
    this->createCallBack(LHF(aGetCanSetShutter),   HTTP_GET, "cansetshutter");
    this->createCallBack(LHF(aGetCanSetAzimuth),   HTTP_GET, "cansetazimuth");
    this->createCallBack(LHF(aGetCanSetAltitude),  HTTP_GET, "cansetaltitude");
    this->createCallBack(LHF(aGetCanSetPark),      HTTP_GET, "cansetpark");
    this->createCallBack(LHF(aGetCanPark),         HTTP_GET, "canpark");
    this->createCallBack(LHF(aGetCanFindHome),     HTTP_GET, "canfindhome");
    this->createCallBack(LHF(aGetCanSlave),        HTTP_GET, "canslave");
    this->createCallBack(LHF(aGetCanSyncAzimuth),  HTTP_GET, "cansyncazimuth");

    // Status (GET)
    this->createCallBack(LHF(aGetSlewing),         HTTP_GET, "slewing");
    this->createCallBack(LHF(aGetSlaved),          HTTP_GET, "slaved");
    this->createCallBack(LHF(aGetShutterStatus),   HTTP_GET, "shutterstatus");
    this->createCallBack(LHF(aGetAzimuth),         HTTP_GET, "azimuth");
    this->createCallBack(LHF(aGetAltitude),        HTTP_GET, "altitude");
    this->createCallBack(LHF(aGetAtHome),          HTTP_GET, "athome");
    this->createCallBack(LHF(aGetAtPark),          HTTP_GET, "atpark");

    // Commands (PUT)
    this->createCallBack(LHF(aPutSlaved),          HTTP_PUT, "slaved");
    this->createCallBack(LHF(aPutOpenShutter),     HTTP_PUT, "openshutter");
    this->createCallBack(LHF(aPutCloseShutter),    HTTP_PUT, "closeshutter");
    this->createCallBack(LHF(aPutAbortSlew),       HTTP_PUT, "abortslew");
    this->createCallBack(LHF(aPutSlewToAzimuth),   HTTP_PUT, "slewtoazimuth");
    this->createCallBack(LHF(aPutSlewToAltitude),  HTTP_PUT, "slewtoaltitude");
    this->createCallBack(LHF(aPutPark),            HTTP_PUT, "park");
    this->createCallBack(LHF(aPutSetPark),         HTTP_PUT, "setpark");
    this->createCallBack(LHF(aPutFindHome),        HTTP_PUT, "findhome");
    this->createCallBack(LHF(aPutSyncToAzimuth),   HTTP_PUT, "synctoazimuth");
}

void AlpacaDome::aGetInterfaceVersion(AsyncWebServerRequest *request)
{
    _alpacaServer->respond(request, ALPACA_DOME_INTERFACE_VERSION);
}
