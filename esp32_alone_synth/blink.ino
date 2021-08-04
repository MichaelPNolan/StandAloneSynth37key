/*
 * this file includes a simple blink task implementation
 *
 * Author: Marcel Licence
 */
#define ONBOARD_LED 0
inline
void Blink_Setup(void)
{
    pinMode(ONBOARD_LED, OUTPUT);
}


inline
void Blink_Process(void)
{
    static bool ledOn = true;
    if (ledOn)
    {
        digitalWrite(ONBOARD_LED, HIGH);   // turn the LED on (HIGH is the voltage level)
      //  miniScreenString(4,"--Flash---",HIGH);

    }
    else
    {
        digitalWrite(ONBOARD_LED, LOW);    // turn the LED off
       // miniScreenString(4,"----------",HIGH);

    }
    ledOn = !ledOn;
}
