/*
 * This module is run adc with a multiplexer
 * tested with ESP32 Audio Kit V2.2
 * Only tested with 8 inputs
 *
 * Define your adc mapping in the lookup table
 *
 * Author: Marcel Licence
 *
 * Reference: https://youtu.be/l8GrNxElRkc
 */

/* Notes by Michael - hack version of this 
 * Requires Boardmanager 1.0.4 or earlier of ESP32
 * doing math
*/

// from easySynth module code definitions
#define SYNTH_PARAM_VEL_ENV_ATTACK  0
#define SYNTH_PARAM_VEL_ENV_DECAY 1
#define SYNTH_PARAM_VEL_ENV_SUSTAIN 2
#define SYNTH_PARAM_VEL_ENV_RELEASE 3
#define SYNTH_PARAM_FIL_ENV_ATTACK  4
#define SYNTH_PARAM_FIL_ENV_DECAY 5
#define SYNTH_PARAM_FIL_ENV_SUSTAIN 6
#define SYNTH_PARAM_FIL_ENV_RELEASE 7
#ifdef USE_UNISON
#define SYNTH_PARAM_DETUNE_1    8
#define SYNTH_PARAM_UNISON_2    9
#else
#define SYNTH_PARAM_WAVEFORM_1    8
#define SYNTH_PARAM_WAVEFORM_2    9
#endif
#define SYNTH_PARAM_MAIN_FILT_CUTOFF  10
#define SYNTH_PARAM_MAIN_FILT_RESO    11
#define SYNTH_PARAM_VOICE_FILT_RESO   12
#define SYNTH_PARAM_VOICE_NOISE_LEVEL 13

//new customer parameters not handled by modules as forked
#define CONTROL_PARAM_MAX_VOL  14
//You can also call the simple_delay module 
// eg: void Delay_SetLength(uint8_t unused, float value)
//  in z_config midi-controller parameters the function calls are encoded as Delay_SetLength, 2   -Delay_SetLevel, 3    -Delay_SetFeedback, 4
#define CONTROL_DELAY_SET_LENGTH 15
#define CONTROL_DELAY_SET_LEVEL 16
#define CONTROL_DELAY_SET_FEEDBACK 17
#define CONTROL_SEMITONES 18


#ifdef extraButtons
#define bankButton 13 // for use with a single POT to select which parameter
#define downButton 4 // ditto
#else
#define bankButton LED_PIN // if not in use just define something else and keep pins free
#define downButton LED_PIN // ditto
#endif

#define NUMDIRECTPOTS   5  // 4 potentiometers wired to pings by position TL 35, TR 34, BL 39, BR 36
#define NUMBANKS        4
uint8_t adcSimplePins[NUMDIRECTPOTS] = { ADC_DIRECT_TL, ADC_DIRECT_TR, ADC_DIRECT_BL, ADC_DIRECT_BR, 15};// pot pins defined in config.h by location 
                        // extraButtons (above) used to change parameter type related to this pot
                     
uint8_t potBank[NUMBANKS][NUMDIRECTPOTS] = { {SYNTH_PARAM_VEL_ENV_ATTACK,SYNTH_PARAM_VEL_ENV_DECAY,
                                        SYNTH_PARAM_VEL_ENV_SUSTAIN,SYNTH_PARAM_VEL_ENV_RELEASE,SYNTH_PARAM_WAVEFORM_1},  //end bank 0
                                        {SYNTH_PARAM_FIL_ENV_ATTACK,SYNTH_PARAM_FIL_ENV_DECAY,
                                         SYNTH_PARAM_FIL_ENV_SUSTAIN,SYNTH_PARAM_FIL_ENV_RELEASE, SYNTH_PARAM_WAVEFORM_2 }, //end bank 1
                                         {SYNTH_PARAM_MAIN_FILT_RESO,SYNTH_PARAM_MAIN_FILT_CUTOFF,
                                        SYNTH_PARAM_VOICE_FILT_RESO ,SYNTH_PARAM_VOICE_NOISE_LEVEL,CONTROL_PARAM_MAX_VOL}, //end bank 2
                                        {CONTROL_DELAY_SET_LENGTH,CONTROL_DELAY_SET_LEVEL,
                                         CONTROL_DELAY_SET_FEEDBACK,SYNTH_PARAM_MAIN_FILT_CUTOFF, CONTROL_SEMITONES }}; //end bank 3
                                         


bool bankButtonState, downButtonState, lastBankButtonState, lastDownButtonState;
uint8_t  bankValue;
unsigned long lastUBDebounceTime,lastDBDebounceTime;
unsigned long debounceDelay = 50; 

struct adc_to_midi_s
{
    uint8_t ch;
    uint8_t cc;
};
extern int  analogueParamSet = 0;
extern int  waveformParamSet = 0;
const int maxParameterVal = 13; //the highest numbered synthparameter to avoid sending unhandled parameter
unsigned int adcSingleMin[NUMDIRECTPOTS],adcSingleMax[NUMDIRECTPOTS];
float adcSingle[NUMDIRECTPOTS],adcSingleAve[NUMDIRECTPOTS]; // single pins of ESP32 dedicated to pots ...
float adcSetpoint[NUMDIRECTPOTS];
extern struct adc_to_midi_s adcToMidiLookUp[]; /* definition in z_config.ino */

uint8_t lastSendVal[ADC_TO_MIDI_LOOKUP_SIZE];  /* define ADC_TO_MIDI_LOOKUP_SIZE in top level file */

//#define ADC_INVERT
//#define ADC_THRESHOLD       (1.0f/200.0f)
//#define ADC_OVERSAMPLING    2048

//#define ADC_DYNAMIC_RANGE
//#define ADC_DEBUG_CHANNEL0_DATA

static float adcChannelValue[NUMDIRECTPOTS];


float *AdcMul_GetValues(void)
{
    return adcChannelValue;
}
void readSimplePots(){
  for(int i=0; i<NUMDIRECTPOTS; i++)
    adcSimple(i);

}
                                                //void setTextColor(uint16_t color);
                                                //void setTextColor(uint16_t color, uint16_t backgroundcolor);

// the text for what parameter settings are mapped to the pots and slider
void screenLabelPotBank(){
  uint8_t color;
  switch(bankValue){
    case 0:
       color = 1;
       miniScreenString(0,color,"Attack",HIGH);
       miniScreenString(1,color,"Decay",HIGH);
       miniScreenString(2,color,"Sustain",HIGH);
       miniScreenString(3,color,"Release",HIGH);
       miniScreenString(5,color,"Waveform >",HIGH);
       break;
    case 1:
       color = 1;
       miniScreenString(0,color,"F.Attack",HIGH);
       miniScreenString(1,color,"F.Decay",HIGH);
       miniScreenString(2,color,"F.Sust",HIGH);
       miniScreenString(3,color,"F.Rels",HIGH);
       miniScreenString(5,color,"Waveform2>",HIGH);
       break;
    case 2:
       color = 1;
       miniScreenString(0,color,"M.Reson",HIGH);
       miniScreenString(1,color,"M.Cutoff",HIGH);
       miniScreenString(2,color,"Voice-Res",HIGH);
       miniScreenString(3,color,"Noise-Lev",HIGH);
       miniScreenString(5,color,"Max-Volume>",HIGH);
       break;
     case 3:
       color = 1;
       miniScreenString(0,color,"Delay-Len",HIGH);
       miniScreenString(1,color,"Del-Level",HIGH);
       miniScreenString(2,color,"D.Feedback",HIGH);
       miniScreenString(3,color,"M.Cutoff",HIGH);
       miniScreenString(5,color,"Semitones",HIGH);
       break;
  }
} 

//this is analog to digital conversion reading the (at the time of Aug 27) 4 pots we have being sampled and sending the 
//paramater change adjustements according to parameter value assigned by number potBank[bankValue][potNum] defined above
void  adcSimple(uint8_t potNum){
    unsigned int pinValue = 0;  //long int?  adcSingleMin, adcSingleMax to be same type
    float delta, error;
    bool midiMsg = false;
    const int oversample = 20;
    
    //read the pin multiple times
    for(int i=0;i<oversample;i++)      pinValue += analogRead(adcSimplePins[potNum]);         
      pinValue = pinValue / oversample;  //if wiring bug value inverted 4096-(pinValue / oversample); 
    if(adcSingleMin[potNum] > pinValue) adcSingleMin[potNum] = (pinValue+adcSingleMin[potNum])/2;
    if(adcSingleMax[potNum] < pinValue) adcSingleMax[potNum] = pinValue;
    //Serial.println(pinValue);
    adcSingle[potNum] = float(pinValue)/4096.0f; //(pinValue/10)*10

    //This "noise reduction in the pot value so its not constantly changing was derived experimentally and needs more analysis
    
    delta = adcSingleAve[potNum] - adcSingle[potNum]; //floating point absolute get rid of signed
    error = 0.009f+(0.012f*(adcSingle[potNum]+0.1));  //previous weird idea error = 0.03*((adcSingle+0.25)*0.75); 
    
    //the idea is that if the delta adjustment is larger than the error we pick up significant control reading changes
    //instead of just sending minute voltage reading changes that could be noise and cause un-needed calls to code too often
    if (fabs(delta) > error ){
       if(adcSetpoint[potNum] != adcSingleAve[potNum]) //was there a change - if so then do stuff
        {
          adcSetpoint[potNum] = adcSingleAve[potNum];
          Serial.println("---ADC read: " + String(adcSetpoint[potNum])+"--min: "+String(adcSingleMin[potNum])+"--max: "+String(adcSingleMax[potNum]));
          Serial.print(" Param: "+String(potNum));
          adcChannelValue[potNum] = adcSetpoint[potNum];
          //is the parameter setting for the bank within the range of parameter numbers handled by Synth_SetParam 0-13 #defined paramaters
          if(potBank[bankValue][potNum] < 14)
             Synth_SetParam(potBank[bankValue][potNum], adcChannelValue[potNum]);   //see easySynth module
          else
             Custom_SetParam(potBank[bankValue][potNum], adcChannelValue[potNum]);
          miniScreenBarSize(potNum, adcChannelValue[potNum]); //display a bar in the text area to show the current value
          midiMsg = true;
          
        } 
    
    }
    adcSingleAve[potNum] = (adcSingleAve[potNum]+adcSingle[potNum])/2;
    
/*
    if (midiMsg)
    {
        uint32_t midiValueU7 = (adcChannelValue[analogueParamSet] * 127.999);
        if (analogueParamSet < ADC_TO_MIDI_LOOKUP_SIZE)
        {
            #ifdef ADC_INVERT
            uint8_t idx = (ADC_INPUTS - 1) -analogueParamSet;
            #else
            uint8_t idx = analogueParamSet;
            #endif
            if (lastSendVal[idx] != midiValueU7)
            {
                Midi_ControlChange(adcToMidiLookUp[idx].ch, adcToMidiLookUp[idx].cc, midiValueU7);
                lastSendVal[idx] = midiValueU7;
            }
        }
    } */
}

void Custom_SetParam(uint8_t slider, float value)
{

  switch(slider){
    case CONTROL_PARAM_MAX_VOL:
      keyboardSetVolume(value);  //see multikeyTo37Midi module where keyboard entry calls notes on/off
      break;
    case CONTROL_DELAY_SET_LENGTH:
      Delay_SetLength(0, value); //see simple_delay module
      break;
    case CONTROL_DELAY_SET_LEVEL:
      Delay_SetLevel(0, value);  //see simple_delay module
      break;
    case CONTROL_DELAY_SET_FEEDBACK:
      Delay_SetFeedback(0, value);  //see simple_delay module
      break;
    case CONTROL_SEMITONES:
      keyboardSetSemiModifier(value);
  }
}

void setupButtons(){
  #ifdef extraButtons
  pinMode(bankButton, INPUT_PULLUP);  //pinMode(2, INPUT_PULLUP);
  pinMode(downButton, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);  //use for mode toggle for pot params

  miniScreenString(0,1,"Button_in"+String(bankButton),HIGH);
  #endif
  //pinMode(adcSimplePin, INPUT);
  bankValue = 0;
  screenLabelPotBank();
}

void setupADC_MINMAX(){
  for(int i=0; i<NUMDIRECTPOTS; i++){
    adcSingleMin[i] = 0000;
    adcSingleMax[i] = 4095;
  }
}

void toggleBankButton(){
   bankValue = (bankValue+1)% NUMBANKS;
   digitalWrite(LED_PIN, bankValue);
   screenLabelPotBank();
   miniScreenString(4,1,"Bank: "+ String(bankValue),HIGH);
   
}

void waveFormSet(float potVal){
     Synth_SetParam(8, potVal); // if you have a int then  float(waveformParamSet/7.0f)SYNTH_PARAM_WAVEFORM_1 = 8 unless unison mode in the its detune
   //Synth_SetParam(9, float(waveformParamSet/7.0f));
   //Serial.println("WaveformSet: "+ String(waveformParamSet));
}

// added by Michael to this library which is really for processing knobs and buttons - this is buttons
// Before Aug 27 2020 there was only one button for the alone synth for changing bank for the 4 pots
void processButtons(){

  // read the state of the switch into a local variable:
  int readBankButton = digitalRead(bankButton);
  int readDownButton = digitalRead(downButton);
  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (readBankButton != lastBankButtonState) {
    // reset the debouncing timer
    lastUBDebounceTime = millis();
  }
   if (readDownButton != lastDownButtonState) {
    // reset the debouncing timer
    lastDBDebounceTime = millis();
  }

  if ((millis() - lastUBDebounceTime) > debounceDelay) {

    // if the button state has changed:
    if (readBankButton != bankButtonState) {
      bankButtonState = readBankButton;

      // only toggle the LED if the new button state is HIGH
      if (bankButtonState == LOW) {
         toggleBankButton();
         waveformParamSet = waveformParamSet + 1; //analogueParamSet++;
         
      }
    }
  }
  if ((millis() - lastDBDebounceTime) > debounceDelay) {

    // if the button state has changed:
    if (0){//(readDownButton != downButtonState) {
      downButtonState = readDownButton;

      // only toggle the LED if the new button state is HIGH
      if (downButtonState == HIGH) {
         analogueParamSet = analogueParamSet - 1;
         if (analogueParamSet < 0) analogueParamSet=7;
         if (analogueParamSet > 7) analogueParamSet=0;
         Serial.print("ParamSet: ");
         Serial.println(analogueParamSet);
      }
    }
  }
    // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastBankButtonState = readBankButton;
  lastDownButtonState = readDownButton;

}

//not used - part of the original code
void AdcMul_Init(void)
{
    for (int i = 0; i < ADC_INPUTS; i++)
    {
        adcChannelValue[i] = 0.5f;
    }

    memset(lastSendVal, 0xFF, sizeof(lastSendVal));

    //analogReadResolution(10);
    //analogSetAttenuation(ADC_11db);

 //   analogSetCycles(1);
 //   analogSetClockDiv(1);

 //  adcAttachPin(ADC_MUL_SIG_PIN);

    pinMode(ADC_MUL_S0_PIN, OUTPUT);
#if ADC_INPUTS > 2
    pinMode(ADC_MUL_S1_PIN, OUTPUT);
#endif
#if ADC_INPUTS > 4
    pinMode(ADC_MUL_S2_PIN, OUTPUT);
#endif
#if ADC_INPUTS > 8
    pinMode(ADC_MUL_S3_PIN, OUTPUT);
#endif
}
/*
void AdcMul_Process(void)
{
    static float readAccu = 0;
    static float adcMin = 0;//4000;
    static float adcMax = 420453;//410000;

    for (int j = 0; j < ADC_INPUTS; j++)
    {
        digitalWrite(ADC_MUL_S0_PIN, ((j & (1 << 0)) > 0) ? HIGH : LOW);
#if ADC_INPUTS > 2
        digitalWrite(ADC_MUL_S1_PIN, ((j & (1 << 1)) > 0) ? HIGH : LOW);
#endif
#if ADC_INPUTS > 4
        digitalWrite(ADC_MUL_S2_PIN, ((j & (1 << 2)) > 0) ? HIGH : LOW);
#endif
#if ADC_INPUTS > 8
        digitalWrite(ADC_MUL_S3_PIN, ((j & (1 << 3)) > 0) ? HIGH : LOW);
#endif

        // give some time for transition 
        delay(1);

        readAccu = 0;
        adcStart(ADC_MUL_SIG_PIN);
        for (int i = 0 ; i < ADC_OVERSAMPLING; i++)
        {

            if (adcBusy(ADC_MUL_SIG_PIN) == false)
            {
                readAccu += adcEnd(ADC_MUL_SIG_PIN);
                adcStart(ADC_MUL_SIG_PIN);
            }
        }
        adcEnd(ADC_MUL_SIG_PIN);

#ifdef ADC_DYNAMIC_RANGE
        if (readAccu < adcMin - 0.5f)
        {
            adcMin = readAccu + 0.5f;
            Serial.printf("adcMin: %0.3f\n", readAccu);
        }

        if (readAccu > adcMax + 0.5f)
        {
            adcMax = readAccu - 0.5f;
            Serial.printf("adcMax: %0.3f\n", readAccu);
        }
#endif

        if (adcMax > adcMin)
        {
            //normalize value to range from 0.0 to 1.0
             
            float readValF = (readAccu - adcMin) / ((adcMax - adcMin));
            readValF *= (1 + 2.0f * ADC_THRESHOLD); // extend to go over thresholds 
            readValF -= ADC_THRESHOLD; // shift down to allow go under low threshold 

            bool midiMsg = false;

            // check if value has been changed 
            if (readValF > adcChannelValue[j] + ADC_THRESHOLD)
            {
                adcChannelValue[j] = (readValF - ADC_THRESHOLD);
                midiMsg = true;
            }
            if (readValF < adcChannelValue[j] - ADC_THRESHOLD)
            {
                adcChannelValue[j] = (readValF + ADC_THRESHOLD);
                midiMsg = true;
            }

            // keep value in range from 0 to 1 
            if (adcChannelValue[j] < 0.0f)
            {
                adcChannelValue[j] = 0.0f;
            }
            if (adcChannelValue[j] > 1.0f)
            {
                adcChannelValue[j] = 1.0f;
            }

            // MIDI adoption 
            if (midiMsg)
            {
                uint32_t midiValueU7 = (adcChannelValue[j] * 127.999);
                if (j < ADC_TO_MIDI_LOOKUP_SIZE)
                {
#ifdef ADC_INVERT
                    uint8_t idx = (ADC_INPUTS - 1) - j;
#else
                    uint8_t idx = j;
#endif
                    if (lastSendVal[idx] != midiValueU7)
                    {
                        Midi_ControlChange(adcToMidiLookUp[idx].ch, adcToMidiLookUp[idx].cc, midiValueU7);
                        lastSendVal[idx] = midiValueU7;
                    }
                }
#ifdef ADC_DEBUG_CHANNEL0_DATA
                switch (j == 0)
                {
                    float adcValFrac = (adcChannelValue[j] * 127.999) - midiValueU7;
                    Serial.printf("adcChannelValue[j]: %f -> %0.3f -> %0.3f-> %d, %0.3f\n", readAccu, readValF, adcChannelValue[j], midiValueU7, adcValFrac);
                }
#endif
            }
        }
    }
} */
