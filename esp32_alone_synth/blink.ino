/*
 * this file includes a simple blink task implementation
 *
 * Author: Marcel Licence
 */
#define ONBOARD_LED 0

const char sixteen[17] = "----------------";

uint8_t  stepNum;
inline
void Blink_Setup(void)
{
    pinMode(LED_PIN, OUTPUT);
    stepNum = 0;
}


inline
void Blink_Process(void) //I'm using the blink as a tempo meter and I'll display that 
{
    static bool ledOn = true;
    if (ledOn)
    {
        digitalWrite(LED_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        if(checkBankValue() == 4)  //bank 4 is arpeggiator - there you can draw the moving pulse message 'pulse string'
          miniScreenString(7,1,pulseString(),HIGH);


    }
    else
    {
        digitalWrite(LED_PIN, LOW);    // turn the LED off
        if(checkBankValue() == 4)
          miniScreenString(7,1,pulseString(),HIGH);

    }
    ledOn = !ledOn;
}

String pulseString(){
  String cursorString,dashProgress,dashesAfter;
  uint8_t arpNotes = readHeldNotes();
  int cursorLen;
  stepNum++;
  if(arpNotes > 0)
    cursorString = String(arpNotes);
  else
    cursorString = ">";
  cursorLen = cursorString.length();
  
  if(stepNum == 16)
    stepNum=0;
  dashProgress = String(sixteen);
  dashProgress.remove(stepNum+cursorLen);
  dashProgress.concat(cursorString);
  dashesAfter = String(sixteen);
  dashesAfter.remove(15 - stepNum);
  dashProgress.concat(dashesAfter);
    
  return dashProgress;
}
   
  
