/*  
  PrecAgNode.h - Header for Precision Agriculture Arduino Sketch.
  Created for ENGG4201/8201 by Andrew Miller, Phil McMillan
*/

#ifndef PRECAGNODE_H
#define PRECAGNODE_H

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
  SCREEN_4  // N,P,K
};

#endif
