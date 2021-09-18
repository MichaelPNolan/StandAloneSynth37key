/*
 * this file includes a system for managing arpeggios implementation
 *
 * Author: Michael Nolan
 * 
 * Initial design watches keyboard and sets up parameters like NoteLength, Tempo and a map of Keys to capture to fill an array with a list of notes - the arpeggio
 * Redesigned the whole use of a button as a modifier button with keyboard notes held together to change mode for parameters mapped to ADC input
 * Completed by Sep 11/2021
 * Next step - there is a parameter for the pattern type - trigger calls to determine a pattern out of the list mapped from keyboard - into a new array
 * So generate the array and then the pattern and always play based on pattern type selected
 */

#define PATTERN_LENGTH 24
bool useArpeggiator, arpHold, arpState, keyBoardChanged;
static float bpm;
static uint32_t minuteRATE;
bool noteMap[128];
uint8_t noteOrder[PATTERN_LENGTH],patternOrder[PATTERN_LENGTH]; //see MAX_POLY_VOICE in easySynth
static uint8_t nextNote, previousNoteNum; // nextNote ongoing index, whereas previousNoteNum is for making sure you turn off the last note played in case the list changes

typedef enum arpVariationKind  { up,down,walk,threetwo,fourthree,randArp,entry,doubleTap};
arpVariationKind arpPlayMethod;
typedef enum noteLengthKind  { NoteQuarter,NoteEighth,NoteSixteenth,NoteQuarterTrip,NoteEighthTrip,NoteSixteenthTrip,NoteHalf,NoteHalfTrip};
noteLengthKind arpNoteLength;




inline void arpeggiatorSetup(void)
{
    useArpeggiator = LOW;
    arpHold = LOW;
    bpm = 60.0;
    minuteRATE = (SAMPLE_RATE * 60);
   
    arpState = HIGH;
    keyBoardChanged = LOW;
    arpNoteLength = NoteQuarter;
    nextNote = 0;
    previousNoteNum = 0;
}

inline bool checkArpeggiator(void){ //this means we are in the arppeggiator bank and we are running bpm flashing
  return useArpeggiator;
  
}

inline bool checkArpState(void){ //this flag turns off arpeggio playback - defaults to on but pot can stop playback
  return arpState;
  
}

inline float checkBPM(void){
  return bpm;
  
}
inline void setBPM(float value){
  bpm = 300.0f*value+10;
}

inline uint32_t calcWaitPerBeat(void){
                     //a minute has 60 seconds so its the rate per sec * 60 then div beats per minute
  return uint32_t(minuteRATE/bpm);
}

uint32_t noteLengthCycles(){ //how many notes per beat ie quater, eighth, triplets
   switch(arpNoteLength){
     case NoteQuarter:
      return uint32_t(minuteRATE/(bpm*4));
      break;
    case NoteEighth:
      return uint32_t(minuteRATE/(bpm*8));
      break;
    case NoteSixteenth:
      return uint32_t(minuteRATE/(bpm*16));
      break;
    case NoteQuarterTrip:
      return uint32_t(minuteRATE/(bpm*6));
      break;
    case NoteEighthTrip:
      return uint32_t(minuteRATE/(bpm*12));
      break;
    case NoteSixteenthTrip:
      return uint32_t(minuteRATE/(bpm*12));
      break;
    case NoteHalf:
      return uint32_t(minuteRATE/(bpm*2));
      break;
    case NoteHalfTrip:
      return uint32_t(minuteRATE/(bpm*3));
      break;
  }
}

void useArpToggle(bool use){
  useArpeggiator = use;
}

void arpAllOff(){  //also a general reset of arpeggiator
  for(int i=0; i<PATTERN_LENGTH; i++){ // clear the note order array
    noteOrder[i] = 0;
    patternOrder[i] = 0;
  }
  for(int i=0; i<128; i++){ //send a note off to the whole range of notes
    Synth_NoteOff(0, i);
    noteMap[i] = LOW;      //erase the note map while you are at it
    updateNoteOrder();
  }
  nextNote = 0;
}


inline void Arpeggiator_Process(void)
{
   if(keyBoardChanged == HIGH){ //always be ready to have the latest notes when there is keyboard activity
     keyBoardChanged = LOW;
     updateNoteOrder();  

                        // need to make something to scan keymap properly for removed notes
   }
   
   if(previousNoteNum != 0)//(nextNote > 0)
      Synth_NoteOff(0, previousNoteNum);
   if(noteOrder[nextNote] > 0){
     Synth_NoteOn(0, patternOrder[nextNote], getKeyboardVolume());   //old version used noteOrder = now that is used to generate patternOrder 
     previousNoteNum = patternOrder[nextNote]; //there was a bug where if you removed a note from the pattern or map it wouldn't be ended before next note - so force capture it
     nextNote++;
   } else nextNote = 0; //restart at beginning of pattern if the current note is 0 indicating end of list (list is filled with 0 when unused)
   
     
}

void Arp_NoteOn(uint8_t note){
  if(arpHold)  //arpHold is a toggle note type of mode
    noteMap[note] = !noteMap[note];
  else
    noteMap[note] = HIGH;
  keyBoardChanged = HIGH;
}

void Arp_NoteOff(uint8_t note){
  if(!arpHold)
    noteMap[note] = LOW;
  keyBoardChanged = HIGH;
}

void updateNoteOrder(){ //build a list of notes to play which is 0s for any unfilled slots
  bool noteRemoved = LOW;
  uint8_t noteSlot = 0;
  for(int i=0; i < 128; i++){
    if (noteMap[i]){
      //Serial.print(String(i));
      if(noteSlot<PATTERN_LENGTH){
        noteOrder[noteSlot] = i;
        noteSlot++;
      }
    } else { //if noteMap[i] is LOW check if it is in the pattern as a note
      for(int j=0; j<PATTERN_LENGTH; j++){ // clear the note order array
        if((noteOrder[j] == i) && !noteRemoved){ //only do this setting once 
          noteRemoved = HIGH;
          Synth_NoteOff(0, noteOrder[j] ); //if you don't do this now you have to make some system to process noteoff later
        } 
        if(noteRemoved) 
        {
          if(j == (PATTERN_LENGTH-1))  //aka if this is the last note in pattern leave a 0
             noteOrder[j] = 0;
          else
             noteOrder[j] = noteOrder[j+1]; // erase note by moving all notes back overwriting it
             
        }
        
      }
      noteRemoved = LOW;
    }
  }
  updatePatternOrder();
  //Serial.println();
}

void updatePatternOrder(){
  int counter=0;
  switch(arpPlayMethod){
    case down:
      for(int j=PATTERN_LENGTH-1; j>-1; j--){ // clear the note order array
        if(noteOrder[j] !=0){ //only do this setting once 
          patternOrder[counter] = noteOrder[j];
          counter++;
        }
      } 
      break;
    default: //this is up which is a direct copy of noteOrder
      for(int j=0; j<PATTERN_LENGTH; j++){ // clear the note order array
        patternOrder[j] = noteOrder[j];
        } 
      break;
  }
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

void setArpVariation(float value){ // up,down,walk,threetwo,fourthree,randArp,entry,doubleTap};
  uint8_t division = 9 * value;
  switch(division){
    case 1:
      arpPlayMethod = up;
      miniScreenString(1,1,"V.Upward",HIGH);
      break;
    case 2:
      arpPlayMethod = down;
      miniScreenString(1,1,"V.Down",HIGH);
      break;
    case 3:
      arpPlayMethod = walk;
      miniScreenString(1,1,"V.Walk",HIGH);
      break;
    case 4:
      arpPlayMethod = threetwo;
      miniScreenString(1,1,"V.3+2-",HIGH);
      break;
    case 5:
      arpPlayMethod = fourthree;
      miniScreenString(1,1,"V.4+3-",HIGH);
      break;
    case 6:
      arpPlayMethod = randArp;
      miniScreenString(1,1,"V.Random",HIGH);
      break;
    case 7:
      arpPlayMethod = entry;
      miniScreenString(1,1,"V.entryO",HIGH);
      break;
    case 8:
      arpPlayMethod = doubleTap;
      miniScreenString(1,1,"V.2-tap",HIGH);
      break;
  }
  updatePatternOrder();
}

boolean checkArpHold(){ //i've set it to check this in adc bank change - if arpHold is on its not going to toggle arp mode off or silence all notes
  return arpHold;
}

void setArpHold(float value){ //to be coded
   if(value > 0.5f){
      arpHold = HIGH;
      miniScreenString(2,1,"HOLD ON",HIGH);
   }else{
      arpHold = LOW;
      arpAllOff();  // this should send note off
      miniScreenString(2,0,"HOLD OFF",HIGH);
   }
}

void setArpNoteLength(float value){ //{ NoteQuarter,NoteEighth,NoteSixteenth,NoteQuarterTrip,NoteEighthTrip,NoteSixteenthTrip,NoteHalf,NoteHalfTrip};
  uint8_t division = 9 * value;
  switch(division){
    case 1:
      arpNoteLength = NoteQuarter;
      miniScreenString(3,1,"Quarter",HIGH);
      break;
    case 2:
      arpNoteLength = NoteEighth;
      miniScreenString(3,1,"Eighth",HIGH);
      break;
    case 3:
      arpNoteLength = NoteSixteenth;
      miniScreenString(3,1,"Sixtnth",HIGH);
      break;
    case 4:
      arpNoteLength = NoteQuarterTrip;
      miniScreenString(3,1,"QuarTrip",HIGH);
      break;
    case 5:
      arpNoteLength = NoteEighthTrip;
      miniScreenString(3,1,"EighTrip",HIGH);
      break;
    case 6:
      arpNoteLength = NoteSixteenthTrip;
      miniScreenString(3,1,"SixtnTrip",HIGH);
      break;
    case 7:
      arpNoteLength = NoteHalf;
      miniScreenString(3,1,"NoteHalf",HIGH);
      break;
    case 8:
      arpNoteLength = NoteHalfTrip;
      miniScreenString(3,1,"HalfTrip",HIGH);
      break;
  };
}
