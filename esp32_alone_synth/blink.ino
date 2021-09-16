/*
 * this file includes a simple blink task implementation
 *
 * Author: Marcel Licence
 */
#define ONBOARD_LED 0

String sixteen = "----------------";

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
  stepNum++;
  String generate1,generate2,generate3;
  if(stepNum == 16){
    stepNum=0;
    generate1 = ">";
    generate2 = String(sixteen);
    generate2.remove(15);
    generate1.concat(generate2);
    return generate1;
  } else {
    generate1 = ">";
    generate2 = String(sixteen);
    generate2.remove(stepNum);
    generate2.concat(generate1);
    generate3 = String(sixteen);
    generate3.remove(15 - stepNum);
    generate2.concat(generate3);
    
    return generate2;
  }
   
  

}
