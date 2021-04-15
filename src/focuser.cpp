#include "focuser.h"

// todo: set update() to private and only run when needed

FastAccelStepperEngine Focuser::_engine = FastAccelStepperEngine();
bool Focuser::_engine_init = false;

// cannot call member functions directly from interrupt, so need these helpers for up to 4 focus motors.
uint8_t Focuser::_n_focusers = 0;
Focuser *Focuser::_focuser_array[4] = { nullptr, nullptr, nullptr, nullptr };
void ICACHE_RAM_ATTR Focuser::_isr_homing0() { _focuser_array[0]->_setHome(); }
void ICACHE_RAM_ATTR Focuser::_isr_homing1() { _focuser_array[1]->_setHome(); }
void ICACHE_RAM_ATTR Focuser::_isr_homing2() { _focuser_array[2]->_setHome(); }
void ICACHE_RAM_ATTR Focuser::_isr_homing3() { _focuser_array[3]->_setHome(); }
void (*Focuser::_isr_array[])() = { _isr_homing0, _isr_homing1, _isr_homing2, _isr_homing3 };

bool Focuser::begin()
{
    // initialize stepper engine
    if (!_engine_init) {
        _engine.init();
        _engine_init = true;
    }

    // setup TMC driver
    _serialport->begin(115200, SERIAL_8N1, _pin_rx, _pin_tx);
    _driver = new TMC2208Stepper(_serialport, TMC_RS);
    _driver->begin();
    _driver->rms_current(STP_CURRENT);
    _driver->en_spreadCycle(false); // enable stealthchop
    _driver->pwm_autoscale(true);
    _driver->microsteps(STP_MICROSTEP);

    // setup stepper pulse engine
    _stepper = _engine.stepperConnectToPin(_pin_step);
    _stepper->setDirectionPin(_pin_dir);
    _stepper->setEnablePin(_pin_en);
    _stepper->setAutoEnable(false); // power save mode
    _stepper->setCurrentPosition(_pos_target);

    // calculate motion parameters
    _updateMotionParam();

    // done
    _stepper->enableOutputs();

    // setup interrupt for homing
    pinMode(_pin_home, INPUT_PULLUP);
    _focuser_array[_focuser_index] = this;
    attachInterrupt(_pin_home, _isr_array[_focuser_index], CHANGE);

    return true;
}

void Focuser::_updateMotionParam()
{
    _driver->shaft(_inverted); // invert direction
    _stepper->setSpeedInUs(1000000/(_speed * _steps_per_mm));
    _stepper->setAcceleration(_acceleration * _steps_per_mm);
}

void Focuser::update()
{
    int target;
    // if stepper is stopping after command, wait for stop to reset target pos
    if (_pos_target == UINT32_MAX) {
        if (_stepper->isRunning())
            return;
        else
            _pos_target = _stepper->getCurrentPosition();
    }

    // do temp compensation if enabled
    if (_temp_comp && _temp_meas != NAN) {
        int32_t corrected = nearbyintf(_temp_coeff * _temp_meas + _pos_target);
        target = constrain(corrected, _pos_min, _pos_max);
        
    } else {
        target = _pos_target;
    }

    // move to target
    _stepper->moveTo(target);
}

void Focuser::zero()
{
    // set homing speed
    _stepper->runBackward();
    
    // wait til hit endstop
    while(_stepper->isRunning())
        delay(10);
    
    // move forward to reset endstop
    move( 1.5 * _steps_per_mm);

    // move back once more, slower for better precision
    _stepper->setSpeedInUs(1000000/(_speed * _steps_per_mm)/4);
    move(-2.0 * _steps_per_mm);
    while(_stepper->isRunning())
        delay(10);
    // reset speed to normal after zeroing is done
    _zeroed = true;
    _stepper->setSpeedInUs(1000000/(_speed * _steps_per_mm));
}