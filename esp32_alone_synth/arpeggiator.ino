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
static uint8_t noteOrder[PATTERN_LENGTH],noteSequential[PATTERN_LENGTH],patternOrder[PATTERN_LENGTH]; //see MAX_POLY_VOICE in easySynth
static uint8_t nextNote, previousNoteNum, walkResume; // nextNote ongoing index, whereas previousNoteNum is for making sure you turn off the last note played in case the list changes

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
    walkResume = 0;
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
    noteSequential[i] = 0;
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
   //Serial.print("* ");
   if(keyBoardChanged == HIGH){ //always be ready to have the latest notes when there is keyboard activity
     keyBoardChanged = LOW;
     updateNoteOrder();  

                        // need to make something to scan keymap properly for removed notes
   }
   
   if(previousNoteNum != 0)//(nextNote > 0)
      Synth_NoteOff(0, previousNoteNum);
   if(patternOrder[nextNote] > 0){
     if(patternOrder[nextNote] >1)
       Synth_NoteOn(0, patternOrder[nextNote], getKeyboardVolume());   //old version used noteOrder = now that is used to generate patternOrder 
     previousNoteNum = patternOrder[nextNote]; //there was a bug where if you removed a note from the pattern or map it wouldn't be ended before next note - so force capture it
     nextNote++;
   } else {
     nextNote = 0; //restart at beginning of pattern if the current note is 0 indicating end of list (list is filled with 0 when unused)
     if(arpPlayMethod == walk) //of we are at the end of patter in walk playMethod we should generate a new pattern - walkResume is the maintained to give continutity
       updatePatternOrder();  
     if(patternOrder[nextNote] >1)
       Synth_NoteOn(0, patternOrder[nextNote], getKeyboardVolume());   //old version used noteOrder = now that is used to generate patternOrder 
     previousNoteNum = patternOrder[nextNote]; //there was a bug where if you removed a note from the pattern or map it wouldn't be ended before next note - so force capture it
     nextNote++;
   }
   
     
}

void Arp_NoteOn(uint8_t note){
  Serial.print("On: "+String(note));
  if(arpHold)  //arpHold is a toggle note type of mode
    noteMap[note] = !noteMap[note];
  else
    noteMap[note] = HIGH;
  keyBoardChanged = HIGH;
  addNoteSeq(note);
}

void Arp_NoteOff(uint8_t note){
  Serial.print("Off: "+String(note));
  if(!arpHold)
    noteMap[note] = LOW;
  keyBoardChanged = HIGH;
}

/*
 * N
 */
void addNoteSeq(uint8_t note){
  Serial.println("Add: "+String(note));
  bool insertionDone = LOW;
  for(int j=0; j<PATTERN_LENGTH; j++){
    if(noteSequential[j] == note) //if we have the note already don't add it because arpeggiator input is defining a chord not a sequence as such
      insertionDone = HIGH;
    if((noteSequential[j] == 0) && !insertionDone){
     noteSequential[j] = note;
     insertionDone = HIGH;
       
    }
    Serial.print(String(noteSequential[j]));
  }
  Serial.println();
  //if there was no space left write a message that buffer is full
  #ifdef DISPLAY_1306
  if(!insertionDone)
    miniScreenString(6,1,"FULL-B",HIGH);
  
  #endif
}

void delNoteSeq(uint8_t note){ //very similar to the updateNoteOrder and called by that routine to manage the noteSequential array
  bool noteRemoved = LOW;
  Serial.println("Del: "+String(note));
  for(int j=0; j<PATTERN_LENGTH; j++){
    if(noteSequential[j] == note) //check notes until you get to the note that needs deletion then flag
       noteRemoved = HIGH;
    if(noteRemoved) //remove and backfill array from this position to the end
      {
        if(j == (PATTERN_LENGTH-1))  //aka if this is the last note in pattern leave a 0
           noteSequential[j] = 0;
        else
           noteSequential[j] = noteOrder[j+1]; // erase note by moving all notes back overwriting it
      }
  }
  //this is only a once through note deletion scan so we don't need to reset noteRemoved flag like we do in updateNoteOrder
}

void delTailSeq(){
  for(int j=PATTERN_LENGTH-1; j>-1; j--){ // clear the note order array
    if(noteSequential[j] !=0){ //only do this setting once 
      
      noteMap[noteSequential[j]] = LOW;
      updateNoteOrder(); // call the noteOrder array builder which should now call a note remove instead of directly removing noteSequential[j] = 0;
      j=-1;
    }
  }
}
/* 
 *  Design notes: patterns and note order 
 *  Scanning keyboard reveals the notes in order by collecting from the note map of whats on and off
 *  Keepign both a array of the noteOrder (currently set arp notes) and the pattern order means as the arp patterns change
 *  we can keep recreating them from the noteOrder array ... so the seeming redundancy is a kind of reference
 *  If we want to play them according to when they were entered we have to store some kind of sequential order
 *  That means we might need to maintain that separately because there is a variation for "according to when they were entered"
 *  so I added noteSequential array (more like FIFO) and write commands to maintain the order they were added but its not a sequence as such
 *  because you can't add 2 notes - tht will be a different module for sequencing (needs ties and rests)
 */

//I'm sure there is a more efficient way to build a list over time without notemap a binary array 0-127 but i made this to get started fast
//its possible of course you can just manage NoteOrder without noteMap but the idea is that keys can get turn off and on in note map and less frequently
// you update the list of notes held for an arpeggio by scanning through
// when the playback hold parameter is on if(!arpHold) prevents the notemap from being turned off by noteOff signals and instead you toggle notes off by pressing
// the same key see the NoteOn noteMap[note] = !noteMap[note]; is called if(arpHold) - ie toggles the notemap
// that is the design elegance, perhaps, of the noteMap ... can update fast and later be used to make note lists

void updateNoteOrder(){ //build a list of notes to play which is 0s for any unfilled slots
  bool noteRemoved = LOW;
  uint8_t noteSlot = 0;
  for(int i=1; i < 128; i++){ //start from 1 (rather than 0) to look for Del because noteOrder[j] == i would always trigger because noteOrder is full of 0 for no note
    if (noteMap[i]){  //note map is 0/LOW for any key that is off and 1/HIGH for any key that is on
      
      if(noteSlot<PATTERN_LENGTH){  //turn all the keys in note map that are on or held keys of keyboard into a list of notes in noteOrder array
        noteOrder[noteSlot] = i;
        noteSlot++;
      }
    } else { //if noteMap[i] is LOW check if it is in the pattern as a note
      
      for(int j=0; j<PATTERN_LENGTH; j++){ // clear the note order array
        if((noteOrder[j] == i) && !noteRemoved){ //only do this setting once so check if noteRemoved
          noteRemoved = HIGH;
          delNoteSeq(noteOrder[j]);
          Synth_NoteOff(0, noteOrder[j] ); //if you don't do this now you have to make some system to process noteoff later
        } 
        if(noteRemoved) //remove and backfill array
        {
          if(j == (PATTERN_LENGTH-1))  //aka if this is the last note in pattern leave a 0
             noteOrder[j] = 0;
          else
             noteOrder[j] = noteOrder[j+1]; // erase note by moving all notes back overwriting it         
        }
      }
      noteRemoved = LOW; //reset for next potential note that is now off 
    }
  }
  walkResume = 0;
  updatePatternOrder();
  
}
uint8_t dice4(){
    return rand() % 4;
}

void updatePatternOrder(){ //uses an algorithm to generate the pattern from noteOrder (a low to high list of notes in an array)
  int counter=0;
  int notesLength = 0;
  bool up = HIGH; //HIGH means up LOW means back for an up three steaps back two pattern
  int delta = 3;
  switch(arpPlayMethod){
    case down:
      for(int j=PATTERN_LENGTH-1; j>-1; j--){ // clear the note order array
        if(noteOrder[j] !=0){ //only do this setting once 
          patternOrder[counter] = noteOrder[j];
          counter++;
        }
      } 
      break;
      //------------------------------------
    case walk:            /*  Arturia keystep defn: With the Arp mode encoder set to Walk, the arpeggiator will play the held notes in a
                          controlled random order. It's as if the arpeggiator 'threw a dice' at the end of each step:
                          there's a 50% chance it will play the next step, a 25% chance it will play the current step
                            again and a 25% chance it will play the previous step. */
          while((noteOrder[notesLength] != 0) && (notesLength < PATTERN_LENGTH)){ //initialize notesLength to simplify further code
            notesLength++; 
          }
          //Serial.print("Nlen: "+String(notesLength));
          patternOrder[0] = noteOrder[walkResume]; //first note is root 
          counter = walkResume; 
      for(int j=1; j<PATTERN_LENGTH; j++){ //fill the rest of the pattern with the walk 
        switch(dice4()){ //adjust counter using walk algorithm described above with range of notesLength
          case 0 ... 1: //50% chance to add next step note
            (counter+1 < notesLength) ? counter++ : counter = 0; //return to first note if we hit end of PATTERN_LENGTH   
            //Serial.print("+ ");     
          break;
          case 2: //25% chance to add current step note
            //Serial.print("/ ");   
            break;
          case 3: //25% chance to add previous note
            //Serial.print("- "); 
            (counter-1 >= 0) ? counter-- : counter = notesLength-1; //return to last note if we get below 0
            break;
        }
        patternOrder[j] = noteOrder[counter];
       
      }
      (walkResume+1 < notesLength) ? walkResume++ : walkResume = 0;
      //Serial.println();                    
      break;
    case threetwo:
      delta = 3;
      for(int j=0; j<PATTERN_LENGTH; j++){
        if(noteOrder[counter] > 0)
          patternOrder[j] = noteOrder[counter];
        else
          j=PATTERN_LENGTH;
        if(delta > 0){
          counter++;
          delta--;
        } else if (delta < 0){
          delta++;
          counter--;
        } 
        if (delta==0) //3 steps forward done started 3 went to 0
          delta=-3;
        if(delta==-1) //2 steps back done started -3 went to -1
          delta=3;
      }
      break; 
    case fourthree:
      delta = 4;
      for(int j=0; j<PATTERN_LENGTH; j++){
        if(noteOrder[counter] > 0)
          patternOrder[j] = noteOrder[counter];
        else
          j=PATTERN_LENGTH;
        if(delta > 0){
          counter++;
          delta--;
        } else if (delta < 0){
          delta++;
          counter--;
        } 
        if (delta==0) //3 steps forward done started 3 went to 0
          delta=-4;
        if(delta==-1) //2 steps back done started -3 went to -1
          delta=4;
      }
      break;
    case randArp:
      while((noteOrder[notesLength] != 0) && (notesLength < PATTERN_LENGTH)){ //initialize notesLength to simplify further code
            notesLength++; 
      }
      for(int j=0; j<PATTERN_LENGTH; j++){ 
         patternOrder[j] = noteOrder[rand()%notesLength];
      }
      break;
      //------------------------------------------------------
    case entry: //the order notes were manually played in
      for(int j=0; j<PATTERN_LENGTH; j++){ //blind copy noteSequential 
        patternOrder[j] = noteSequential[j];
      } 
      break;
      //----------------------------
    default: //this is up which is a direct copy of noteOrder
      for(int j=0; j<PATTERN_LENGTH; j++){ //blind copy noteOrder
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
      walkResume = 0;
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
      arpPlayMethod = entry;  //according to when they were entered
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
