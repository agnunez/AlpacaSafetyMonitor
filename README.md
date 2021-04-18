**Yet Another Focuser**

This is a telescope focus controller for use with Ascom Alpaca. It runs on an
ESP32 chip with one or more TMC2208 stepper drivers, DS1820 temperature sensors
and a optical 1600 steps/rev full quadrature rotation encoder for focusing.
The encoder can be used to control any of the connected stepper motors. To select
which stepper one can turn a selection switch.

It uses acceleration and top speed to calculate the movement of the focus to
reduce vibrations.

For Ascom Alpaca compability it uses ESPAscomAlpacaServer library (https://github.com/elenhinan/ESPAscomAlpacaServer)

<img src="https://github.com/elenhinan/YetAnotherFocuser/blob/master/pics/focuser.png?raw=true" width="300">
