#include "main.h"

#ifdef FOCUSER2_ENABLE
  #define N_FOCUSERS 2
#else
  #define N_FOCUSERS 1
#endif

OneWire oneWire(PIN_DS1820);
DallasTemperature sensors(&oneWire);
Focuser focuser[N_FOCUSERS] = {
  Focuser(&STP1_SERIAL, STP1_RX, STP1_TX, STP1_STEP, STP1_DIR, STP1_EN, PIN_HOME1)
#ifdef FOCUSER2_ENABLE
  ,Focuser(&STP2_SERIAL, STP2_RX, STP2_TX, STP2_STEP, STP2_DIR, STP2_EN, PIN_HOME2)
#endif
};
WiFiServer tcpServer(TCP_PORT);
WiFiClient tcpClient;

AlpacaServer alpacaServer("AstroFocus");

void setup() {
  // setup serial
  Serial.begin(115200, SERIAL_8N1);

  setup_wifi();

  // setup ASCOM Alpaca server
  alpacaServer.begin(ALPACA_UDP_PORT, ALPACA_TCP_PORT);

  // setup focuser
  for(uint8_t i=0; i<N_FOCUSERS; i++) {
    focuser[i].begin();
    alpacaServer.addDevice(&focuser[i]);
  }

  setup_encoder();
  setup_sensors();
}

void loop() {
  alpacaServer.update();
  update_encoder();
  update_sensors();
  update_focus();
  delay(50); 
}

void setup_wifi()
{
  pinMode(PIN_WIFI_LED, OUTPUT);

  // setup wifi
  Serial.print(F("\n# Starting WiFi"));

  //DoubleResetDetector drd = DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  ESP_WiFiManager ESP_wifiManager(HOSTNAME);
  ESP_wifiManager.setConnectTimeout(60);

  if (ESP_wifiManager.WiFi_SSID() == "") { // || drd.detectDoubleReset()) {
    Serial.println(F("# Starting Config Portal"));
    digitalWrite(PIN_WIFI_LED, HIGH);
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
    digitalWrite(PIN_WIFI_LED, HIGH);
    if(!MDNS.begin("HOSTNAME")) {
     Serial.println("# Error starting mDNS");
     return;
    }
  }
}

void setup_encoder()
{
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  encA = digitalRead(PIN_ENC_A);
  encB = digitalRead(PIN_ENC_B);
  attachInterrupt(PIN_ENC_A, encoderA_isr, CHANGE);
  attachInterrupt(PIN_ENC_B, encoderB_isr, CHANGE);
}

void setup_sensors()
{
  sensors.begin();

  if (sensors.getAddress(temp_address, 0)) {
    sensors.setResolution(12);
    sensors.setWaitForConversion(false);
    sensors.setCheckForConversion(true);
  }
}

void update_encoder()
{
  if(enc_counter) {
    uint8_t fIndex = digitalRead(PIN_ENC_SEL);
    focuser[fIndex].move(enc_counter);
    enc_counter = 0;
  }
}

void update_sensors()
{
  if(sensors.isConversionComplete()) {
    temperature = sensors.getTempC(temp_address);
    focuser[0].setTemperature(temperature);
    sensors.requestTemperatures();
  }
}

void update_focus()
{
  focuser[0].update();
}

// Interrupts
void ICACHE_RAM_ATTR encoderA_isr() {
    encA = digitalRead(PIN_ENC_A);
    if (encA == encB) {
      enc_counter++;
    } else {
      enc_counter--;
    }
}

void ICACHE_RAM_ATTR encoderB_isr() {
    encB = digitalRead(PIN_ENC_B);
    if (encA == encB) {
      enc_counter--;
    } else {
      enc_counter++;
    }
}