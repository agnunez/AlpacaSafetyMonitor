// General
#define STEPPER2_ENABLE     false

// TMC2208
#define TMC_RS              0.11f   // Rs of Fysetc TMC2208 is 0.11 Ohm

// Stepper settings
#define STP1_MICROSTEP      32   // Microsteps internally
#define STP1_CURRENT        400  // Stepper current per phase (mAmps)
#define STP1_MAX_SPEED      10.0f // mm/s
#define STP1_ACCELERATION   0.5f // mm/s^2

// Rotary encoder
#define ROT_STEP_PER_REV    1600

// Mechanics
#define STP1_MM_PER_REV     (3.0 * 2 * 3.14)
#define STP1_GEAR_RATIO     (30/1)
#define STP1_STEPS_PER_MM   (200 * STP1_MICROSTEP * STP1_GEAR_RATIO / STP1_MM_PER_REV)