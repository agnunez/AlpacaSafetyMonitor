#include "AlpacaFocuser.h"

uint8_t AlpacaFocuser::_n_focusers = 0;

void AlpacaFocuser::registerCallbacks(AlpacaServer &alpaca_server)
{
    AlpacaDevice::registerCallbacks(alpaca_server);
    this->createCallBack(LHF(getAbsolute), HTTP_GET, "absolute");
    this->createCallBack(LHF(getIsMoving), HTTP_GET, "ismoving");
    this->createCallBack(LHF(getMaxIncrement), HTTP_GET, "maxincrement");
    this->createCallBack(LHF(getMaxStep), HTTP_GET, "maxstep");
    this->createCallBack(LHF(getPosition), HTTP_GET, "position");
    this->createCallBack(LHF(getStepSize), HTTP_GET, "stepsize");
    this->createCallBack(LHF(getTempComp), HTTP_GET, "tempcomp");
    this->createCallBack(LHF(putTempComp), HTTP_PUT, "tempcomp");
    this->createCallBack(LHF(getTempCompAvailable), HTTP_GET, "tempcompavailable");
    this->createCallBack(LHF(getTemperature), HTTP_GET, "temperature");
    this->createCallBack(LHF(putHalt), HTTP_PUT, "halt");
    this->createCallBack(LHF(putMove), HTTP_PUT, "move");
}

void AlpacaFocuser::getAbsolute(){

}
void AlpacaFocuser::getIsMoving(){

}
void AlpacaFocuser::getMaxIncrement(){

}
void AlpacaFocuser::getMaxStep(){

}
void AlpacaFocuser::getPosition(){

}
void AlpacaFocuser::getStepSize(){

}
void AlpacaFocuser::getTempComp(){

}
void AlpacaFocuser::putTempComp(){

}
void AlpacaFocuser::getTempCompAvailable(){

}
void AlpacaFocuser::getTemperature(){

}
void AlpacaFocuser::putHalt(){

}
void AlpacaFocuser::putMove(){

}