/* Multikey_MC17 - 16-bit I2C port conversion of Keypad's Multikey
:: G. D. (Joe) Young Feb 12, 2013
:: MC23017 version - GDY May 19, 2014
::
|| @file MultiKey.ino
|| @version 1.0
|| @author Mark Stanley
|| @contact mstanley@technologist.com
||
|| @description
|| | The latest version, 3.0, of the keypad library supports up to 10
|| | active keys all being pressed at the same time. This sketch is an
|| | example of how you can get multiple key presses from a keypad or
|| | keyboard.
|| #
*/

#include <Keypad.h>
#include <Keypad_MC17.h>    // I2C i/o library for Keypad
#include <Wire.h>           // I2C library for Keypad_MC17

#define I2CADDR 0x20        // address of MCP23017 chip on I2C bus

const byte ROWS = 6; // Four rows
const byte COLS = 7; // Three columns
// Define the Keymap
uint8_t keys[ROWS][COLS] = {
  {20,21,22,23,24,25,26},
  {27,28,29,30,31,32,33},
  {34,35,36,37,38,39,40},
  {41,42,43,44,45,46,47},  //this row was buggy on hardware side perhaps - red row wire seems to be an issue. Intermittent correct/incorrect resolution 
  {48,49,50,51,52,53,54},
  {55,56,0,0,0,0,0}  //42 combinations - 56 was the highest to get triggered
};

byte rowPins[ROWS] = { 6, 5, 4, 3, 2, 1 };

byte colPins[COLS] = { 14, 13, 12,11,10, 9, 8 }; 

// modify constructor for I2C i/o
Keypad_MC17 kpd( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR );

float volumeParam = 1.0f;
float semiModifier = 0.5f;



unsigned long loopCount = 0;
unsigned long startTime = millis();
String msg = "";
uint8_t  keyMod = 40;

void setupKeyboard() {
    //for USB serial switching boards
  Wire.begin( );
  kpd.begin( );                // now does not starts wire library
  kpd.setDebounceTime(1);
  //scan();
  //Serial.println("myLIST_MAX ="+String(myLIST_MAX));
  
}

// to receive changes from controller adc input see adc_module
void keyboardSetVolume(float value)
{
  volumeParam = value;
}

void keyboardSetSemiModifier(float value)
{
  semiModifier = value;
}

void serviceKeyboardMatrix() {
   uint8_t  keyUS;
  //const int myLIST_MAX = LIST_MAX - 2; //42
  // Fills kpd.key[ ] array with up-to 10 active keys.
  // Returns true if there are ANY active keys.
  if (kpd.getKeys())
  {
    for (int i=0; i<LIST_MAX; i++)   // Scan the whole key list.  LIST_MAX
    {
      if ( kpd.key[i].stateChanged )   // Only find keys that have changed state.
      {
        keyUS  = uint8_t(kpd.key[i].kchar);
        if (keyUS > 0){
           keyUS += keyMod*semiModifier;
          switch (kpd.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
              case PRESSED:
                  //msg = " PRESSED.";
                  Synth_NoteOn(0, keyUS, volumeParam); //unchecked if type works as a note - was defaulted to 1.0f for velocity
                  break;
              case HOLD:
                  msg = " HOLD.";
                  break;
              case RELEASED:
                  //msg = " RELEASED.";
                  Synth_NoteOff(0, keyUS);
                  break;
              case IDLE:
                  msg = " IDLE.";
          }
        }
        #ifdef DISPLAY_1306
        miniScreenString(6,1,"N#:"+String(keyUS),HIGH);
        
        #endif
        Serial.print("Key :/");//+String(LIST_MAX));
        Serial.print(uint8_t(kpd.key[i].kchar));
        Serial.println(msg);
        
      }
    }
  }
}  // End loop

void scan() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("done\n");
  }
  delay(5000);          
}
