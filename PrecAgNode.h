/*  
  PrecAgNode.h - Header for Precision Agriculture Arduino Sketch.
  Created for ENGG4201/8201 by Andrew Miller, Phil McMillan
*/

#ifndef PRECAGNODE_H
#define PRECAGNODE_H

// LoRaWAN Settings
const char *appEui = "70B3D57ED003E470";
const char *appKey = "9FD4F69F9274AE52A7D7111E19C95389";
//const char *appEui = "0000000000000000";
//const char *appKey = "0B15ADDE14611E5067C349AE6DFC1692";

const unsigned long SENSOR_READING_INTERVAL = 1000;
const unsigned long SCREEN_CHANGE_INTERVAL = 10; // change screen every x readings
const unsigned long TTN_UPDATE_INTERVAL = 30; // change screen every x readings

// I2C address
#define NOLEDI2CA   (char) 0x3C

enum STATES {
  READ_SENSORS,
  NITRO_REQ, NITRO_RESP,
  PHOS_REQ, PHOS_RESP,
  POTA_REQ, POTA_RESP,
  DISPLAY_VALUES,
  CHANGE_OLED_SCREEN,
  UPDATE_TTN
};

enum OLED_SCREEN {
  SCREEN_1, // temperature, humidity
  SCREEN_2, // pressure, altitude
  SCREEN_3, // Moisture, Light
  SCREEN_4,  // N,P,K
  SCREEN_5  // Time
};

#endif
