#include "main.h"

FastAccelStepperEngine engine = FastAccelStepperEngine();
TMC2208Stepper driver1 = TMC2208Stepper(&STP1_SERIAL, TMC_RS);
TMC2208Stepper driver2 = TMC2208Stepper(&STP2_SERIAL, TMC_RS);

void setup() {
  // setup pins
  pinMode(LED_BUILTIN, OUTPUT);
  
  // setup serial and wait
  Serial.begin(115200, SERIAL_8N1);
  while (!Serial); delay(200);

  // setup wifi
  Serial.print(F("\n# Starting WiFi"));

  DoubleResetDetector drd = DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  ESP_WiFiManager ESP_wifiManager("AstroFocuser");

  if (ESP_wifiManager.WiFi_SSID() == "" || drd.detectDoubleReset()) {
    Serial.println(F("# Starting Config Portal"));
    digitalWrite(WIFI_LED, HIGH);
    if (!ESP_wifiManager.startConfigPortal()) {
      Serial.println(F("# Not connected to WiFi"));
    } else {
      Serial.println(F("# connected"));
    }
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin();
  } 
  WiFi.waitForConnectResult();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("# Failed to connect"));
  }
  else {
    Serial.print(F("# Local IP: "));
    Serial.println(WiFi.localIP());
    digitalWrite(WIFI_LED, HIGH);
  }

  // initialize stepper engine
  engine.init();

  // setup stepper1 TMC
  STP1_SERIAL.begin(115200, SERIAL_8N1, STP1_RX, STP1_TX);
  driver1.begin();
  driver1.rms_current(STP1_CURRENT);
  driver1.en_spreadCycle(false); // enable stealthchop
  driver1.shaft(false); // invert direction
  driver1.pwm_autoscale(true);
  driver1.microsteps(STP1_MICROSTEP);

  // setup stepper1 control
  stepper1 = engine.stepperConnectToPin(STP1_STEP);
  stepper1->setDirectionPin(STP1_DIR);
  stepper1->setEnablePin(STP1_EN);
  stepper1->setAutoEnable(false); // power save mode
  stepper1->setDelayToEnable(50);
  stepper1->setDelayToDisable(1000);
  stepper1->setSpeedInUs(1000000/(STP1_MAX_SPEED*STP1_STEPS_PER_MM));
  stepper1->setAcceleration(STP1_ACCELERATION*STP1_STEPS_PER_MM);
  stepper1->setCurrentPosition(target_position);
  stepper1->enableOutputs();

  // setup endstops
  pinMode(STP1_HOME, INPUT_PULLUP);
  pinMode(STP2_HOME, INPUT_PULLUP);
  attachInterrupt(STP1_HOME, homing1_isr, FALLING);
  //attachInterrupt(STP2_HOME, homing2_isr, FALLING);

  // setup encoder
  pinMode(ROT_ENC_A, INPUT_PULLUP);
  pinMode(ROT_ENC_B, INPUT_PULLUP);
  encA = digitalRead(ROT_ENC_A);
  encB = digitalRead(ROT_ENC_B);
  attachInterrupt(ROT_ENC_A, encoderA_isr, CHANGE);
  attachInterrupt(ROT_ENC_B, encoderB_isr, CHANGE);
  delay(1000);
}

void loop() {
  Serial.print("# Target: ");
  Serial.print(target_position);
  Serial.print(" Current: ");
  Serial.print(stepper1->getCurrentPosition());
  Serial.print("    \r");
  delay(50);
  stepper1->moveTo(target_position);
}

void ICACHE_RAM_ATTR homing1_isr() {
  stepper1->forceStopAndNewPosition(0);
}
void ICACHE_RAM_ATTR homing2_isr() {
  stepper2->forceStopAndNewPosition(0);
}
void ICACHE_RAM_ATTR encoderA_isr() {
    encA = digitalRead(ROT_ENC_A);
    if (encA == encB) {
      target_position++;
    } else {
      target_position--;
    }
    target_position = constrain(target_position, min_position, max_position);
}
void ICACHE_RAM_ATTR encoderB_isr() {
    encB = digitalRead(ROT_ENC_B);
    if (encA == encB) {
      target_position--;
    } else {
      target_position++;
    }
    target_position = constrain(target_position, min_position, max_position);
}