#include "AlpacaFocuser.h"

void AlpacaFocuser::registerCallbacks()
{
    AlpacaDevice::registerCallbacks();
    this->createCallBack(LHF(aGetAbsolute), HTTP_GET, "absolute");
    this->createCallBack(LHF(aGetIsMoving), HTTP_GET, "ismoving");
    this->createCallBack(LHF(aGetMaxIncrement), HTTP_GET, "maxincrement");
    this->createCallBack(LHF(aGetMaxStep), HTTP_GET, "maxstep");
    this->createCallBack(LHF(aGetPosition), HTTP_GET, "position");
    this->createCallBack(LHF(aGetStepSize), HTTP_GET, "stepsize");
    this->createCallBack(LHF(aGetTempComp), HTTP_GET, "tempcomp");
    this->createCallBack(LHF(aPutTempComp), HTTP_PUT, "tempcomp");
    this->createCallBack(LHF(aGetTempCompAvailable), HTTP_GET, "tempcompavailable");
    this->createCallBack(LHF(aGetTemperature), HTTP_GET, "temperature");
    this->createCallBack(LHF(aPutHalt), HTTP_PUT, "halt");
    this->createCallBack(LHF(aPutMove), HTTP_PUT, "move");
}

void AlpacaFocuser::aGetInterfaceVersion() {
    _alpacaServer->respond(ALPACA_FOCUSER_INTERFACE_VERSION);
};
