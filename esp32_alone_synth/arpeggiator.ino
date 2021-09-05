/*
 * this file includes a system for managing arpeggios implementation
 *
 * Author: Michael Nolan
 */
bool useArpeggiator, arpHold, arpState;
static float bpm;
static uint32_t minuteRATE;
float noteLength;

inline void arpeggiatorSetup(void)
{
    useArpeggiator = LOW;
    arpHold = LOW;
    bpm = 60.0;
    minuteRATE = (SAMPLE_RATE * 60);
    noteLength = 1.0f;
    arpState = HIGH;
}

inline bool checkArpeggiator(void){
  return useArpeggiator;
  
}
inline float checkBPM(void){
  return bpm;
  
}
inline void setBPM(float value){
  bpm = 300.0f*value+10;
}

inline uint32_t calcWaitPerBeat(){
                     //a minute has 60 seconds so its the rate per sec * 60 then div beats per minute
  return uint32_t(minuteRATE/bpm);
}

void useArpToggle(bool use){
  useArpeggiator = use;
}

void arpAllOff(){
  for(int i=0; i<128; i++)
    Synth_NoteOff(0, i);
}


inline void Arpeggiator_Process(void)
{
   
}

void Arp_NoteOn(uint8_t note){
  
}

void Arp_NoteOff(uint8_t note){
  
}

void setArpState(float value){  
   if(value > 0.5f){
      arpState = HIGH;
      miniScreenString(0,1,"Arpeg-On!",HIGH);
   }    else{
      arpState = LOW;
      miniScreenString(0,0,"Arpeg-Off",HIGH);
   }
      
}

void setArpVariation(float value){ //to be coded
}

void setArpHold(float value){ //to be coded
   if(value > 0.5f){
      arpHold = HIGH;
      miniScreenString(2,1,"HOLD ON",HIGH);
   }else{
      arpHold = LOW;
      miniScreenString(2,0,"HOLD OFF",HIGH);
   }
}

void setArpNoteLength(float value){ //to be coded
  noteLength = value;
}
