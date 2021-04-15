#pragma once
#include <Arduino.h>
#include <TMCStepper.h>
#include <FastAccelStepper.h>
#include "config.h"
#include "messages.h"
#include "AlpacaFocuser.h"

class Focuser : public AlpacaFocuser {
    private:
        static FastAccelStepperEngine _engine;
        static bool _engine_init;
        static uint8_t _n_focusers;
        static Focuser *_focuser_array[4];
        static void (*_isr_array[])();
        static ICACHE_RAM_ATTR void _isr_homing0();
        static ICACHE_RAM_ATTR void _isr_homing1();
        static ICACHE_RAM_ATTR void _isr_homing2();
        static ICACHE_RAM_ATTR void _isr_homing3();

        TMC2208Stepper *_driver = NULL;
        FastAccelStepper *_stepper = NULL;
        uint8_t _focuser_index;

        HardwareSerial *_serialport;
        uint8_t _pin_rx;
        uint8_t _pin_tx;
        uint8_t _pin_step;
        uint8_t _pin_dir;
        uint8_t _pin_en;
        uint8_t _pin_home;

        int32_t _pos_target = 0;
        int32_t _pos_min = 0;
        int32_t _pos_max = 32000;
        int32_t _backlash = 0;

        bool _inverted = false;
        float _steps_per_mm = STP_STEPS_PER_MM;
        float _speed = STP_MAX_SPEED;
        float _acceleration = STP_ACCELERATION;
        
        float _temp_coeff = 0.0f;
        float _temp_meas = NAN;
        bool _temp_comp = false;
        bool _zeroed = false;

        void _updateMotionParam();
        void _setHome() { _stepper->forceStopAndNewPosition(0); _pos_target=0; }

    public:
        Focuser(HardwareSerial *serialport, uint8_t pin_rx, uint8_t pin_tx, uint8_t pin_step, uint8_t _pin_dir, uint8_t pin_en, uint8_t pin_home)
            : AlpacaFocuser()
            , _serialport(serialport)
            , _pin_rx(pin_rx)
            , _pin_tx(pin_tx)
            , _pin_step(pin_step)
            , _pin_dir(_pin_dir)
            , _pin_en(pin_en)
            , _pin_home(pin_home)
        { _focuser_index = _n_focusers++; }
        bool begin();
        void update();
        void zero();
        void stop() { _stepper->stopMove(); _pos_target = UINT32_MAX; }
        void move(int32_t distance) { _pos_target = constrain(_pos_target + distance, _pos_min, _pos_max); update();}
        void setTemperature(float temp_reading) { _temp_meas = temp_reading; };

        // alpaca getters
        void aGetAbsolute()             { _alpacaServer->respond(1); }
        void aGetIsMoving()             { _alpacaServer->respond(_stepper->isRunning()); }
        void aGetMaxIncrement()         { _alpacaServer->respond(_pos_max); }
        void aGetMaxStep()              { _alpacaServer->respond(_pos_max); }
        void aGetPosition()             { _alpacaServer->respond(_stepper->getCurrentPosition()); }
        void aGetStepSize()             { _alpacaServer->respond(1000/_steps_per_mm); }
        void aGetTempComp()             { _alpacaServer->respond(_temp_comp); }
        void aGetTempCompAvailable()    { _alpacaServer->respond(1); }
        void aGetTemperature()          { _alpacaServer->respond(_temp_meas); }

        // alpaca setters
        void aPutTempComp()             { _alpacaServer->getParam("TempComp", _temp_comp); _alpacaServer->respond(nullptr); }
        void aPutHalt()                 { stop(); _alpacaServer->respond(nullptr); }
        void aPutMove()                 { _alpacaServer->getParam("Position", _pos_target); _alpacaServer->respond(nullptr); }
};