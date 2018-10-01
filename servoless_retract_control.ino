/**************************************************************************
* FILENAME :        servoless_retract_control.ino        DESIGN REF: SRC00
*
* DESCRIPTION :
*       Arduino code to operate a set of servoless retracts for a drone.
*       The contol is based on a BMP180 barometric pressure, altitude and 
*       temperature sensor.  The BMP180 is used to calculate altitude to
*       determine when to extend and retract the landing gear.
*
* EXTERNAL LIBRARIES :
*       Servo.h              Standard arduino servo library
*       Wire.h               Standard arduino wire library
*       Adafruit_BMP085.h    Library for the BMP085 and BMP180 sensors
*       EEPROM.h             Standard arduinoeeprom library for storing values
*
* WIRING :
*       The barometric pressure sensor connects to the I2C bus which connection 
*       points may vary depending on the Arduino module used.  In this version
*       of the code, the I2C bus is on DIO 2 and 3, and the retract control uses 
*       PWM on DIO 10. The tset circuit was originally built on an Arduino 
*       Genuino Micro.
*            
* LICENSE :  
*       This program is free software: you can redistribute it and/or modify
*       it under the terms of the GNU General Public License as published by
*       the Free Software Foundation, either version 3 of the License, or
*       (at your option) any later version.
*       
*       This program is distributed in the hope that it will be useful,
*       but WITHOUT ANY WARRANTY; without even the implied warranty of
*       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*       GNU General Public License for more details.
*
*       You should have received a copy of the GNU General Public License
*       along with this program.  If not, see <https://www.gnu.org/licenses/>.
*
* AUTHOR :    Dan Bemowski        START DATE :    September 29, 2018
*
* CHANGES :
*
***/

//#define DEBUG         //Uncomment to turn debugging on
#define SERVO_PIN 10    //Defines the PWM pin used to control the retracts

#include <Servo.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <EEPROM.h>

Servo myservo;          // create servo object to control a servo
Adafruit_BMP085 bmp;    //Create the barometer object to get altitude, pressure and temperature data

bool altInfo = false;   //Used to toggle the display of alittude information
bool tempInfo = false;  //Used to toggle the display of temperature information
bool presInfo = false;  //Used to toggle the display of barometric pressure information
bool extended = false;  //Used to identify if the landing gear is extended
bool retracted = false; //Used to identify if the landing gear is retracted

int altitudeVal = 72;   //Stores the current altitude setpoint
int extendVal = 1000;   //Defines the PWM value that will cause the gear to extend
int retractVal = 2000;  //Defines the PWM value that will cause the gear to retract
float seaLevelPressure = 102770; //This can be changed for your area to get a more precise measurement
float altitudem;        //Current altitude in meters
float altitudein;       //Current altitude in inches
float altitudeBaseline; //Stores an initial baseline measurement used to calculate height

String eepromBuffer;    //Used for storing our values to use on boot up
String readString;      //Stores the string read from the serial port

void setup() {
  //Attempt to start the serial port in the event that we are connected wit USB
  Serial.begin(9600);
  //Give the sreial port time to initialize
  delay(1000);//Set up the servo control
  myservo.attach(SERVO_PIN);
  //Starts the barometric pressure sensor
  if (!bmp.begin()) {
    sprintln("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {}
  }
  // read the eeprom values
  eepromRead();
  //If data was found to be stored, retrieve it and set our initial values for comparison
  if (eepromBuffer.length() > 0) {
    int *bufferVals = getDelimiters(eepromBuffer, ":");
    altitudeVal = bufferVals[0];
    #ifdef DEBUG
    sprint("altitudeVal=");
    sprintln(altitudeVal);
    #endif
    extendVal = bufferVals[1];
    #ifdef DEBUG
    sprint("extendVal=");
    sprintln(extendVal);
    #endif
    retractVal = bufferVals[2];
    #ifdef DEBUG
    sprint("retractVal=");
    sprintln(retractVal);
    #endif
  } else {
    sprintln("Buffer empty");
  }

  //Set our baseline readings from the BMP180 sensor
  altitudem = bmp.readAltitude(seaLevelPressure);
  altitudeBaseline = altitudem*39.3700787402;
}

void loop() {
  //Check if we have serial data coming in and retrieve it
  while (Serial.available()) {
    //gets one byte from serial buffer
    char c = Serial.read();  
    //makes the String readString
    readString += c; 
    //slow looping to allow buffer to fill with next character
    delay(2);  
  }
  
  //Check if a sttring was sent and decide what to do with it
  if (readString.length() >0) {
    //Check if we requested the menu
    if (readString == "menu\r") {
      menu();
    }
    //Check if we are toggling altitude info
    if (readString == "aon\r") {
      altitudeInfo(true);
    }
    if (readString == "aoff\r") {
      altitudeInfo(false);
    }
    //Check if we are toggling temperature info
    if (readString == "ton\r") {
      temperatureInfo(true);
    }
    if (readString == "toff\r") {
      temperatureInfo(false);
    }
    //Check if we are toggling pressure info
    if (readString == "pon\r") {
      pressureInfo(true);
    }
    if (readString == "poff\r") {
      pressureInfo(false);
    }
    //Check if we are toggling all infos
    if (readString == "allon\r") {
      altitudeInfo(true);
      temperatureInfo(true);
      pressureInfo(true);
    }
    if (readString == "alloff\r") {
      altitudeInfo(false);
      temperatureInfo(false);
      pressureInfo(false);
    }
    //Check if an extend request was sent
    if (readString == "extend\r") {
      myservo.writeMicroseconds(extendVal);
      #ifdef DEBUG
      sprintln("Extending");
      #endif
      extended = true;
      retracted = false;
    }
    //Check if a retract request was sent
    if (readString == "retract\r") {
      myservo.writeMicroseconds(retractVal);
      #ifdef DEBUG
      sprintln("retracting");
      #endif
      extended = false;
      retracted = true;
    }
    //Check if we are setting or requesting our altitude set point
    if (readString.startsWith("alt:")) {
      String alt = readString.substring(4);
      String lastAlt = String(altitudeVal);
      if (alt.toInt() != 0) {
        sprint("Altitude: ");
        sprint(alt);
        sprintln(" inches");
        altitudeVal = alt.toInt();
        eepromWrite();
      } else {
        sprint("Current trigger altitude: ");
        sprint(String(altitudeVal));
        sprintln(" inches");
      }
    }
    //Check if we are settin our servo  control values
    if (readString.startsWith("servo:")) {
      sprint("Servo extend: ");
      int *servoVals = getDelimiters(readString.substring(6), ":");
      extendVal = servoVals[0];
      retractVal = servoVals[1];
      sprint(String(extendVal));
      sprint(" | retract: ");
      sprintln(String(retractVal));
      eepromWrite();
    }
    // Clear the serial string read for the next round
    readString = "";
  }
  //Check if we requested to display altitude info
  if (altInfo) {
    displayAlt();
  }
  //Check if we requested to display temperature info
  if (tempInfo) {
    displayTemp();
  }
  //Check if we requested to display pressure info
  if (presInfo) {
    displayPres();
  }
  //Check if we requested to display all infos
  if (altInfo || tempInfo || presInfo) {
    delay(500);
  }
  //Read the current altitude to see if we need to extend or retract
  altitudem = bmp.readAltitude(seaLevelPressure);
  altitudein = altitudem*39.3700787402;
  //If we are less than our baseline, extend
  if (((altitudein - altitudeBaseline) < (altitudeVal - 25)) && !extended) {
    myservo.writeMicroseconds(extendVal);
    #ifdef DEBUG
    sprintln("Extending");
    #endif
    extended = true;
    retracted = false;
  }
  //If we are greater than our baseline, retract
  if (((altitudein - altitudeBaseline) > (altitudeVal + 25)) && !retracted) {
    myservo.writeMicroseconds(retractVal);
    #ifdef DEBUG
    sprintln("Retracting");
    #endif
    extended = false;
    retracted = true;
  }
  delay(100);
}

/**
 * menu - displays the menu of control options
 */
void menu() {
  sprintln("---=== Debug Menu ===---");
  sprintln("Toggle options:");
  sprintln("  aon/aoff - toggle displaying altitude information");
  sprintln("  ton/toff - toggle displaying temperature information");
  sprintln("  pon/poff - toggle displaying pressure information");
  sprintln("  allon/alloff - toggle displaying all 3");
  sprintln("");
  sprintln("Eeprom settings:");
  sprintln("");
  sprintln("  alt: - Set altitude height in inches to trigger the landing gear. (Only use 60-254 no decimal)");
  sprintln("         Below this height the gear will extend.  Above this height gear will retract.  Sending");
  sprintln("         \"alt:\" with no height will show the current trigger height.");
  sprintln("  Example:");
  sprintln("  alt:72  (72 inches is 6 feet)");
  sprintln("");
  sprintln("  servo: - Sets the servos low and high settings to extend and retract");
  sprintln("  Example:");
  sprintln("  servo:1000:2000  (extends at 1000, retracts at 2000)");
  sprintln("");
  sprintln("Control:");
  sprintln("");
  sprintln("  extend - Forces the gear to etend.  This is to help release the retracts if stuck in");
  sprintln("           a certain position.");
  sprintln("");
  sprintln("  retract - Forces the gear to retract.  This is to help release the retracts if stuck");
  sprintln("            in a certain position.");  
  sprintln("  Depending on the current height of the drone in relation to the baseline, retract and/or extend");
  sprintln("  might not have any affect.");
}

/**
 * altitudeInfo - Turns altitude information on or off
 */
void altitudeInfo(bool onOff) {
  altInfo = onOff;
}

/**
 * temperatureInfo - Turns temperature information on or off
 */
void temperatureInfo(bool onOff) {
  tempInfo = onOff;
}

/**
 * pressureInfo - Turns pressure information on or off
 */
void pressureInfo(bool onOff) {
  presInfo = onOff;
}
  
/**
 * displayAlt - Displays altitude information
 */
void displayAlt() {
  altitudem = bmp.readAltitude(seaLevelPressure);
  altitudein = altitudem*39.3700787402;
  sprint("Altitude = ");
  sprint(String(altitudein));
  sprint(" inches | Baseline difference = ");
  sprintln(String(altitudein - altitudeBaseline));
}

/**
 * displayTemp - Displays temperature information
 */
void displayTemp() {
  sprint("Temperature = ");
  sprint(String(bmp.readTemperature()));
  sprintln(" *C");
}

/**
 * displayPres - Displays pressure information
 */
void displayPres() {
  sprint("Pressure = ");
  sprint(String(bmp.readPressure()));
  sprintln(" Pa");
}

/**
 * getDelimiters - Used to split a string by a delimiter
 * 
 * ARGS :
 *   DelString - String with delimiters
 *   Delby     - Delimiter
 */
int *getDelimiters(String DelString, String Delby) {
 static int Delimiters[3];//important!!! now much Delimiters;
 int i = 0;
 while (DelString.indexOf(Delby) >= 0) {
   int delim = DelString.indexOf(Delby);
   Delimiters[i] = (DelString.substring(0, delim)).toInt();
   DelString = DelString.substring(delim + 1, DelString.length());
   i++;
   if (DelString.indexOf(Delby) == -1) {
     Delimiters[i] = DelString.toInt();
   }
 }
 return Delimiters;
}

/**
 * eepromWrite - Write all values to eeprom
 */
void eepromWrite() {
  //clear the eeprom
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  String str = String(altitudeVal) + ":" + String(extendVal) + ":" + String(retractVal);

  for(int i = 0; i < (str.length()); i++){
      EEPROM.write(i, str[i]);
  }
  EEPROM.write(str.length()+1, '~');
  eepromRead();
}

/**
 * eepromRead - Read all values from eeprom
 */
void eepromRead() {
  int i = 0;
  char val;
  eepromBuffer = "";
  
  while(val != '~'){
    val = EEPROM.read(i);
    eepromBuffer += String(val);
    i++;
    if (i > 500) {
      sprintln("i > 500");
      break;
    }
  }
}

/**
 * sprint - used to perform a controlled serial print based on if we are connected to USB or not
 */
void sprint(String str) {
  if (Serial) {
    Serial.print(str);
  }
}

/**
 * sprintln - used to perform a controlled serial print line based on if we are connected to USB or not
 */
void sprintln(String str) {
  if (Serial) {
    Serial.println(str);
  }
}

