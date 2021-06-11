#include <MKRWAN.h>
#include <CayenneLPP.h>
#include "NewOLED.h" // Newhaven OLED Library
#include "PrecAg.h"  // Sensor Library
#include <RTCZero.h> // real-time clock

#include "PrecAgNode.h"

unsigned long previousMillis = 0;
unsigned long interval = 0; // no delay the first time through

NewOLED oled(NOLEDI2CA);
PrecAg pAg;

STATES currState, nextState;
OLED_SCREEN currScreen;

RTCZero rtc;

_lora_band region = AS923;
LoRaModem modem(Serial1);
CayenneLPP lpp(51);

void setup() {
  Serial.begin(9600);
  while(!Serial);    // time to get serial running

  // set RTC
  rtc.begin();
  rtc.setTime(9, 0, 0); // initialise to 09:00:00
  
  // initialise radio
  if (!modem.begin(region)) {
    Serial.println("Failed to start module");
    while (1) {}
  };

  Serial.print("Device EUI is: ");
  Serial.println(modem.deviceEUI());

  int connected = modem.joinOTAA(appEui, appKey);
  if (!connected) {
    Serial.println("Something went wrong; are you indoor? Move near a window and retry");
    while (1) {}
  }
  Serial.println("Successfully joined the network!");

  // Serial.println("Enabling ADR and setting low spreading factor");
  modem.setADR(true);
  modem.dataRate(5);
 
  // initialise the OLED
  oled.oledSetup();
  currScreen = SCREEN_1;

  pAg.setupSensors();
  
  // initialise the state machine for main loop()
  nextState = READ_SENSORS;
}

void ttnUpdate()
{
    
  Serial.println("Sending TTN update");
  
  // Prepare Cayenne LPP
  lpp.reset();
  lpp.addTemperature(1, pAg.getTemperature());
  lpp.addBarometricPressure(2, pAg.getPressure());
  lpp.addAnalogOutput(3, pAg.getAltitude());
  lpp.addRelativeHumidity(4, pAg.getHumidity());
  lpp.addLuminosity(5, (float) pAg.getLight());
  lpp.addAnalogOutput(6, pAg.getMoisture()/100.0);
  lpp.addAnalogOutput(7, (float) pAg.getNitrogen()); 
  lpp.addAnalogOutput(8, (float) pAg.getPhosphorous());
  lpp.addAnalogOutput(9, (float) pAg.getPotassium());

  // Send the data
  modem.beginPacket();
  modem.write(lpp.getBuffer(), lpp.getSize());
  int err = modem.endPacket(true);
  if (err > 0) {
    Serial.println("Message sent.");
  } else {
    Serial.println("Error sending data.");
  }
  // check for TTN downlinks
  char rcv[64];
  int i = 0;
  while (modem.available()) {
    rcv[i++] = (char)modem.read();
  }
  Serial.print("Received: ");
  for (unsigned int j = 0; j < i; j++) {
    Serial.print(rcv[j] >> 4, HEX);
    Serial.print(rcv[j] & 0xF, HEX);
    Serial.print(" ");
  }
  Serial.println();
  if (i==6) {
    Serial.println("Setting Valve Open/Close Times");
    pAg.setValveOpenTime((byte)rcv[0], (byte)rcv[1], (byte)rcv[2], (byte)rcv[3], (byte)rcv[4], (byte)rcv[5]);
  }
}

void loop() {
  char line[17];
  byte hours, mins, secs;
  String str_temp;
  static unsigned long counter = 0;

  // avoid delay() for precise control of timing
  unsigned long currentMillis = millis();
    
  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    currState = nextState;

    // check to see if the valve should be opened or closed
    pAg.checkValve(rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
  
    // process State Machine  
    switch (currState) {
      case READ_SENSORS:
        counter++;
        pAg.takeReadings();
        // change state to read NPK values
        nextState = NITRO_REQ;
        interval = 0; // read NPK values without delay
        break;
      case NITRO_REQ:
        pAg.sendNReq();
        interval = 250; // wait 250 millis for data to arrive
        nextState = NITRO_RESP;
        break;
      case NITRO_RESP:
        pAg.getNResp();
        interval = 0; // send next request immediately
        nextState = PHOS_REQ;
        break;
      case PHOS_REQ:
        pAg.sendPReq();
        interval = 250; // wait 250 millis for data to arrive
        nextState = PHOS_RESP;
        break;
      case PHOS_RESP:
        pAg.getPResp();
        interval = 0; // send next request immediately
        nextState = POTA_REQ;
        break;
      case POTA_REQ:
        pAg.sendKReq();
        interval = 250; // wait 250 millis for data to arrive
        nextState = POTA_RESP;
        break;
      case POTA_RESP:
        pAg.getKResp();
        interval = 0; // display values immediately
        nextState = DISPLAY_VALUES;
        break;
      case DISPLAY_VALUES:
        // Display values on Serial Monitor
        Serial.print(counter);
        Serial.print(": Temp:");
        Serial.print(pAg.getTemperature());
        Serial.print("(°C),Pres:");
        Serial.print(pAg.getPressure());
        Serial.print("(hPa),");
        Serial.print("Alt:");
        Serial.print(pAg.getAltitude());
        Serial.print("(m)");
        Serial.print(",Hum:");
        Serial.print(pAg.getHumidity());
        Serial.print("(%),Light:");
        Serial.print(pAg.getLight());
        Serial.print(",Moist:");
        Serial.print(pAg.getMoisture());
        Serial.print(",N:");
        Serial.print(pAg.getNitrogen());
        Serial.print(",P:");
        Serial.print(pAg.getPhosphorous());
        Serial.print(",K:");
        Serial.print(pAg.getPotassium());
        Serial.println("(mg/kg)");

        // update OLED
        switch (currScreen) {
          case SCREEN_1:
            str_temp = String("Temp: ") + String(pAg.getTemperature()) + String(" C");
            str_temp.toCharArray(line, 17);
            oled.outputLine(1, (unsigned char *)line);
            str_temp = String("Humi: ") + String(pAg.getHumidity()) + String(" %");
            str_temp.toCharArray(line, 17);
            oled.outputLine(2, (unsigned char *)line);
            break;
          case SCREEN_2:
            str_temp = String("Pres: ") + String(pAg.getPressure());
            str_temp.toCharArray(line, 17);
            oled.outputLine(1, (unsigned char *)line);
            str_temp = String("Alti: ") + String(pAg.getAltitude()) + String(" m");
            str_temp.toCharArray(line, 17);
            oled.outputLine(2, (unsigned char *)line);
            break;
          case SCREEN_3:
            str_temp = String("Moist: ") + String(pAg.getMoisture());
            str_temp.toCharArray(line, 17);
            oled.outputLine(1, (unsigned char *)line);
            str_temp = String("Light: ") + String(pAg.getLight());
            str_temp.toCharArray(line, 17);
            oled.outputLine(2, (unsigned char *)line);
            break;
          case SCREEN_4:
            str_temp = String("N,P,K (mg/kg):");
            str_temp.toCharArray(line, 17);
            oled.outputLine(1, (unsigned char *)line);
            str_temp = String(pAg.getNitrogen()) + String(",") + 
              String(pAg.getPhosphorous()) + String(",") +
              String(pAg.getPotassium());
            str_temp.toCharArray(line, 17);
            oled.outputLine(2, (unsigned char *)line);
            break;
          case SCREEN_5:
            hours = rtc.getHours();
            mins = rtc.getMinutes();
            secs = rtc.getSeconds();
            str_temp = String("Time: ") +
              ((hours < 10) ? String("0") : String("")) +
              String(hours) + String(":") +
              ((mins < 10) ? String("0") : String("")) +
              String(mins) + String(":") +
              ((secs < 10) ? String("0") : String("")) +
              String(secs);
            str_temp.toCharArray(line, 17);
            oled.outputLine(1, (unsigned char *)line);
            if (pAg.getValve() == true)
              str_temp = String("Valve: Open");
            else
              str_temp = String("Valve: Closed");
            str_temp.toCharArray(line, 17);
            oled.outputLine(2, (unsigned char *)line);
            break;
        }

        // switch to next state, depending on value of counter
        if ((counter % SCREEN_CHANGE_INTERVAL) == 0) {
          interval = 0; // change screen immediately
          nextState = CHANGE_OLED_SCREEN;
        }
        else if ((counter % TTN_UPDATE_INTERVAL) == 0) {
          interval = 0; // send TTN update immediately
          nextState = UPDATE_TTN;
        }
        else {
          interval = SENSOR_READING_INTERVAL;
          nextState = READ_SENSORS;
        }
        break;
      case CHANGE_OLED_SCREEN:
        switch (currScreen) {
          case SCREEN_1:
            currScreen=SCREEN_2;
            break;
          case SCREEN_2:
            currScreen=SCREEN_3;
            break;
          case SCREEN_3:
            currScreen=SCREEN_4;
            break;
          case SCREEN_4:
            currScreen=SCREEN_5;
            break;
          case SCREEN_5:
            currScreen=SCREEN_1;
            break;
          }
        if ((counter % TTN_UPDATE_INTERVAL) == 0) {
          interval = 0; // send TTN update immediately
          nextState = UPDATE_TTN;
        }
        else {
          interval = SENSOR_READING_INTERVAL; // 1000 millis between sensor readings
          nextState = READ_SENSORS;
        }
        break;
      case UPDATE_TTN:
        ttnUpdate();
        interval = SENSOR_READING_INTERVAL; // 1000 millis between sensor readings
        nextState = READ_SENSORS;
        break;
    }
  }
}
