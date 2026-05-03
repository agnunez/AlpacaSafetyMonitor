#include "AlpacaObservingConditions.h"

void AlpacaObservingConditions::registerCallbacks()
{
    AlpacaDevice::registerCallbacks();
    this->createCallBack(LHF(aGetAveragePeriod), HTTP_GET, "averageperiod");
    this->createCallBack(LHF(aPutAveragePeriod), HTTP_PUT, "averageperiod");
    this->createCallBack(LHF(aGetDewPoint), HTTP_GET, "dewpoint");
    this->createCallBack(LHF(aGetHumidity), HTTP_GET, "humidity");
    this->createCallBack(LHF(aGetPressure), HTTP_GET, "pressure");
    this->createCallBack(LHF(aGetRainRate), HTTP_GET, "rainrate");
    this->createCallBack(LHF(aGetSkyBrightness), HTTP_GET, "skybrightness");
    this->createCallBack(LHF(aGetSkyQuality), HTTP_GET, "skyquality");
    this->createCallBack(LHF(aGetSkyTemperature), HTTP_GET, "skytemperature");
    this->createCallBack(LHF(aGetStarFwhm), HTTP_GET, "starfwhm");
    this->createCallBack(LHF(aGetTemperature), HTTP_GET, "temperature");
    this->createCallBack(LHF(aGetWindDirection), HTTP_GET, "winddirection");
    this->createCallBack(LHF(aGetWindGust), HTTP_GET, "windgust");
    this->createCallBack(LHF(aGetWindSpeed), HTTP_GET, "windspeed");
    this->createCallBack(LHF(aPutRefresh), HTTP_PUT, "refresh");
    this->createCallBack(LHF(aGetSensorDescription), HTTP_GET, "sensordescription");
    this->createCallBack(LHF(aGetTimeSinceLastUpdate), HTTP_GET, "timesincelastupdate");
    this->createCallBack(LHF(aGetCloudCover), HTTP_GET, "cloudcover");
}

void AlpacaObservingConditions::aGetInterfaceVersion(AsyncWebServerRequest *request) {
    _alpacaServer->respond(request, ALPACA_OBSERVINGCONDITIONS_INTERFACE_VERSION);
}
