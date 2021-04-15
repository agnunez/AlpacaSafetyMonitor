#include "main.h"

OneWire oneWire(PIN_DS1820);
DallasTemperature sensors(&oneWire);
Focuser focuser[N_FOCUSERS] = {
  Focuser(&STP1_SERIAL, STP1_RX, STP1_TX, STP1_STEP, STP1_DIR, STP1_EN, PIN_HOME1)
};
WiFiServer tcpServer(TCP_PORT);
WiFiClient tcpClient;

MsgBuffer tcpMsgBuffer("TCP/IP");
MsgBuffer usbMsgBuffer("USB Serial");

AlpacaServer alpacaServer("AstroFocus");
//Focuser focuser2 = Focuser(&STP2_SERIAL, STP2_RX, STP2_TX, STP2_STEP, STP2_DIR, STP2_EN, PIN_HOME2);

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

  // setup tcp server
  tcpServer.begin();
}

void loop() {
  alpacaServer.update();
  update_serial();
  update_tcpip();
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

void update_serial()
{
  while(usbMsgBuffer.parse(Serial)) {
    parseCommand(Serial, usbMsgBuffer.getMsg());
  }
}

void update_tcpip()
{
  // if not connected, check for incomming connection
  if (!tcpClient.connected())
  {
    if(tcpConnected) {
      Serial.println("# Client disconnected");
      tcpConnected = false;
    }
    tcpClient = tcpServer.available();    
    if (tcpClient.connected()) {
      // send welcome
      tcpConnected = true;
      printConnect(tcpClient);
      Serial.print("# Client connected from ");
      Serial.print(tcpClient.remoteIP());
      Serial.print("\n");
      tcpClient.setTimeout(1);
    }
  } else {
    // parse commands in buffer
    while(tcpMsgBuffer.parse(tcpClient)) {
      parseCommand(tcpClient, tcpMsgBuffer.getMsg());
    }
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
void reply(Stream &stream, char reg, uint8_t idx, float (Focuser::*arg)())
{
  stream.print('%');
  stream.print(reg);
  stream.print(idx+1);
  stream.print(':');
  stream.print((focuser[idx].*arg)(), 5);
  stream.print('\n');
}

void parseCommand(Stream& stream, msg_t msg)
{
  //  [0] cmd
  //    $ = write
  //    % = read
  //  [1] register
  //    P = current position / set position(RW)
  //    M = move to / target (RW)
  //    T = temperature (R)
  //    Z = zero / is zeroed (RW)
  //    H = is moving / stop (RW)
  //    C = temp compensation (RW)
  //    X = temp compensation coefficent (RW)
  //    E = end of travel (RW)
  //    R = resolution (steps per um)
  //    
  //  [2] 0-9 device # (1 or 2, * for n/a)
  //  [3] ':' only if follower by argument
  //  [4:] bool/int/float as string argument (only for some writes)
  //  [-1] ';' last byte, end of command
  //
  // examples:
  //    $M1:12345;
  //    %P1;
  //    %T1;

#ifdef DEBUG
  uint32_t t_start = micros();
#endif

  // check index
  if (msg.idx()>N_FOCUSERS) { 
    stream.println(msg_invalid_index);
  }
  // read
  else if (msg.cmd() == '%') {
    if (!msg.col_empty()) {
      stream.println(msg_invalid_arg);
    } else {
      switch (msg.reg()) {
        case 'P': reply(stream, msg.reg(), msg.idx(), &Focuser::getPosition); break;
        case 'M': reply(stream, msg.reg(), msg.idx(), &Focuser::getTarget); break;
        case 'T': reply(stream, msg.reg(), msg.idx(), &Focuser::getTemperature); break;
        case 'Z': reply(stream, msg.reg(), msg.idx(), &Focuser::isZeroed); break;
        case 'S': reply(stream, msg.reg(), msg.idx(), &Focuser::isMoving); break;
        case 'C': reply(stream, msg.reg(), msg.idx(), &Focuser::getTempComp); break;
        case 'X': reply(stream, msg.reg(), msg.idx(), &Focuser::getTempCoeff); break;
        case 'E': reply(stream, msg.reg(), msg.idx(), &Focuser::getLimit); break;
        case 'R': reply(stream, msg.reg(), msg.idx(), &Focuser::getStepsPerMM); break;
        default:
          stream.println(msg_invalid_reg);
      }
    }
  }

  // write
  else if (msg.cmd() == '$') {
    if ((msg.reg() == 'Z' || msg.reg() == 'S') && !msg.col_empty()) { // if Z or S, no arguments expected
      stream.println(msg_invalid_arg);
    } else if (!msg.has_arg()) { // all other need minimum length 1 argument
      stream.println(msg_invalid_arg);
    } else {
      switch (msg.reg()) {
        case 'P': focuser[msg.idx()].setPosition(msg.arg_int32()); break;
        case 'M': focuser[msg.idx()].moveTo(msg.arg_int32()); break;
        case 'Z': focuser[msg.idx()].zero(); break;
        case 'S': focuser[msg.idx()].stop(); break;
        case 'C': focuser[msg.idx()].setTempComp(msg.arg_bool()); break;
        case 'X': focuser[msg.idx()].setTempCoeff(msg.arg_float()); break;
        case 'E': focuser[msg.idx()].setLimit(msg.arg_int32());
        case 'R': focuser[msg.idx()].setStepsPerMM(msg.arg_float()); break;
        default:
          stream.println(msg_invalid_reg);
      }
    }
  } else {
    stream.println(msg_invalid_cmd);
  }

#ifdef DEBUG
  uint32_t t_stop = micros();
  stream.print("# Time: ");
  stream.print(t_stop-t_start);
  stream.print("us \n");
#endif
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