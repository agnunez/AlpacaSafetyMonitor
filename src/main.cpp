#include "main.h"

OneWire oneWire(PIN_DS1820);
DallasTemperature sensors(&oneWire);
Focuser focuser[N_FOCUSERS] = {
  Focuser(&STP1_SERIAL, STP1_RX, STP1_TX, STP1_STEP, STP1_DIR, STP1_EN, PIN_HOME1)
};
WiFiServer tcpServer(TCP_PORT);
WiFiClient tcpClient;
//Focuser focuser2 = Focuser(&STP2_SERIAL, STP2_RX, STP2_TX, STP2_STEP, STP2_DIR, STP2_EN, PIN_HOME2);

void setup() {
  // setup serial
  Serial.begin(115200, SERIAL_8N1);

  setup_wifi();

  // setup focuser
  for(uint8_t i=0; i<N_FOCUSERS; i++)
    focuser[i].begin();
  //focuser2.begin();

  setup_encoder();
  setup_sensors();

  // setup tcp server
  tcpServer.begin();
}

void loop() {
  update_serial();
  update_tcpip();
  update_encoder();
  update_sensors();
  update_focus();
  delay(50);

  // Serial.print("# Temp: ");
  // Serial.print(temperature,1);
  // Serial.print(" Target: ");
  // Serial.print(focuser1.getTarget());
  // Serial.print(" Current: ");
  // Serial.print(focuser1.getPosition());
  // Serial.print("    \r");
}

void setup_wifi()
{
  pinMode(PIN_WIFI_LED, OUTPUT);

  // setup wifi
  Serial.print(F("\n# Starting WiFi"));

  DoubleResetDetector drd = DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  ESP_WiFiManager ESP_wifiManager(HOSTNAME);
  ESP_wifiManager.setConnectTimeout(60);

  if (ESP_wifiManager.WiFi_SSID() == "" || drd.detectDoubleReset()) {
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

void update_serial()
{

}

void update_tcpip()
{
  // if not connected, check for incomming connection
  if (!tcpClient.connected())
  {
    tcpClient = tcpServer.available();    
    if (tcpClient.connected()) {
      printConnect(tcpClient);
      Serial.print("# Client connected from ");
      Serial.print(tcpClient.remoteIP());
      Serial.print("\n");
      tcpClient.setTimeout(1);
    }
  } else {
    while (tcpClient.available() > 4)
      parseCommand(tcpClient);
  }
}

// Templated function for constructing com replies
template<typename T>
void reply(Stream &stream, char reg, uint8_t idx, T (Focuser::*arg)())
{
  stream.print('%');
  stream.print(reg);
  stream.print(idx+1);
  stream.print(':');
  stream.print((focuser[idx].*arg)());
  stream.print('\n');
}

void parseCommand(Stream &stream)
{
  //  [0] cmd
  //    $ = write
  //    % = read
  //  [1] register
  //    P = current position / set position(RW)
  //    M = move to / target (RW)
  //    R = temperature (R)
  //    Z = zero / is zeroed (RW)
  //    H = is moving / stop (RW)
  //    C = temp compensation (RW)
  //    X = temp compensation coefficent (RW)
  //    E = end of travel (RW)
  //  [2] device # (1 or 2, * for n/a)
  //  [3] ':' (optional if no argument is following)
  //  [4:] argument (only for some writes)
  //
  // examples:
  //    $M1:12345
  //    %P1:
  //    %T1:
  //    %T1

  // get line from stream
  String msg = stream.readStringUntil('\n');
  // remove trailing carriage return if windows system
  if (msg[msg.length()-1] == '\r')
    msg[msg.length()-1] = '\0';

  char cmd = msg[0];
  char reg = msg[1];
  uint8_t idx = msg[2] - '1'; // convert ascii 1,2,3,4 to uint8_t 0,1,2,3
  String arg = msg.substring(4);
  uint32_t arg_len = arg.length();

  if (idx>N_FOCUSERS) {
    stream.println(msg_invalid_index);
    return;
  }

  // read
  if (cmd == '%') {
    if (arg_len > 0) {
      stream.println(msg_invalid_format);
    } else {
      switch (reg) {
        case 'P': reply(stream, reg, idx, &Focuser::getPosition); break;
        case 'M': reply(stream, reg, idx, &Focuser::getTarget); break;
        case 'T': reply(stream, reg, idx, &Focuser::getTemperature); break;
        case 'Z': reply(stream, reg, idx, &Focuser::isZeroed); break;
        case 'S': reply(stream, reg, idx, &Focuser::isMoving); break;
        case 'C': reply(stream, reg, idx, &Focuser::getTempComp); break;
        case 'X': reply(stream, reg, idx, &Focuser::getTempCoeff); break;
        case 'E': reply(stream, reg, idx, &Focuser::getLimit); break;
        case 'R': reply(stream, reg, idx, &Focuser::getStepsPerMM); break;
        default:
          stream.println(msg_invalid_format);
      }
    }
  }
  // write
  else if (cmd == '$') {
    if (reg == 'Z' || reg == 'S') { // if Z or S, no arguments expected
      if (arg_len > 0) {
        stream.println(msg_invalid_format);
      }
    } else if (arg_len < 1) { // all other need minimum length 1 argument
      stream.println(msg_invalid_format);
    } else { // go ahead and hope argument is valid
      switch (reg) {
        case 'P': focuser[idx].setPosition(arg.toInt()); break;
        case 'M': focuser[idx].moveTo(arg.toInt()); break;
        case 'Z': focuser[idx].zero(); break;
        case 'S': focuser[idx].stop(); break;
        case 'C': focuser[idx].setTempComp(arg == "1"); break;
        case 'X': focuser[idx].setTempCoeff(arg.toFloat()); break;
        case 'E': focuser[idx].setLimit(arg.toInt());
        case 'R': focuser[idx].setStepsPerMM(arg.toInt()); break;
      }
    }
  } else {
    stream.println(msg_invalid_format);
  }
}

void printConnect(Stream &stream)
{
  stream.print("# ");
  stream.print(DEVICENAME);
  stream.print(" ");
  stream.print(VERSION);
  stream.print("\n");
  stream.print("# ");
  stream.print(COPYRIGHT);
  stream.print("\n");
  stream.flush();
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