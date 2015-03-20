
#include <Wire.h> // i2C library for comms with Trellis modules.
#include "Adafruit_Trellis.h" // Trellis library
#include <Metro.h> // Interrupt free timer library for querying and midi note sending.
#include <EEPROM.h> // save settings to the eeprom
#include <Bounce.h>
#include <Adafruit_NeoPixel.h>

Bounce pushbutton = Bounce(1, 10);  // 10 ms debounce

elapsedMicros seqTimer;

#define NEOPIN 17

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, NEOPIN, NEO_GRB + NEO_KHZ800);
byte pixRed = 255;
byte pixGrn = 0;
byte pixBlu= 255;


//EEPROM Locations
const uint8_t EEPROM_Brightness = 0;
const uint8_t EEPROM_Octave = 1;
const uint8_t EEPROM_Mode = 2;
const uint8_t EEPROM_Channel = 3;
uint8_t trellisBrightness=5;

//MIDI Sequencer
byte currentStep = 0;
int16_t currentPad = 0;
byte currentPatternLoops = 0;
byte currentBPM;
byte minimumBPM=60; //this sets the minimum BPM. Max BPM is minimumBPM + 127;
byte stepLength=32;
byte stepResolution = 16; //1/32    stepLength / stepResolution = time sig - 32/32 = 4/4

uint16_t PatternsNoteOn[3][8][32]; // 3 types of pattern: drum, bass, lead, 8 patterns of each, 32 steps of each
uint16_t PatternsNoteOff[3][8][32]; // 3 types of pattern: drum, bass, lead, 8 patterns of each, 32 steps of each
#define patDrum 0
#define patBass 1
#define patLead 2
#define midiChannelOffset 0 //default:0 chans 0,1,2 Setting this to 5 would use channels 5,6, and 7. patDrum+midiChannelOffset, patBass+midiChannelOffset, patLead+midiChannelOffset
byte currentPattern[3] = {0,0,0};

byte Pattern1NoteLength = 6;
uint16_t padWords[16];


//MIDI Definiations
const int defaultVelocity = 127;
int currentChannel = 0;
int currentOctave = 4; // middle C is number 60, octave 5.
bool currentRecording = false;
bool currentOverdub = false;
bool currentStopped = false;
bool currentPlaying = false;
int currentMode = 0;
#define ModeSeq1 0
#define ModeSeq2 1
#define ModeSeq3 2
#define ModePattern 3
byte seq1MidiChannel = 0;
byte seq2MidiChannel = 1;
byte seq3MidiChannel = 2;


//#define MIDINOTE_C 0
//#define MIDINOTE_C# 1
//#define MIDINOTE_D 2
//#define MIDINOTE_D# 3
//#define MIDINOTE_E 4
//#define MIDINOTE_F 5
//#define MIDINOTE_F# 6
//#define MIDINOTE_G 7
//#define MIDINOTE_G# 8
//#define MIDINOTE_A 9
//#define MIDINOTE_A# 10
//#define MIDINOTE_B 11


// trellis definitions
Adafruit_Trellis matrix0 = Adafruit_Trellis();
Adafruit_Trellis matrix1 = Adafruit_Trellis();
Adafruit_Trellis matrix2 = Adafruit_Trellis();
Adafruit_Trellis matrix3 = Adafruit_Trellis();
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0, &matrix1, &matrix2, &matrix3);
#define NUMTRELLIS 4
#define numKeys (NUMTRELLIS * 16)
#define TRELLISINTPIN 5

const int pot1Pin = A8;    // select the input pins for the pots
const int pot2Pin = A0;    // select the input pins for the pots
const int pot3Pin = A2;    // select the input pins for the pots
const int pot4Pin = A3;    // select the input pins for the pots
const int pot5Pin = A6;    // select the input pins for the pots
const int pot6Pin = A7;    // select the input pins for the pots
const int pot7Pin = A1;    // select the input pins for the pots
const int pot8Pin = A9;    // select the input pins for the pots

int ledPin = 13;      // select the pin for the LED
int pot1Value = 0;  // variable to store the value coming from the pots
int pot2Value = 0;  // variable to store the value coming from the pots
int pot3Value = 0;  // variable to store the value coming from the pots
int pot4Value = 0;  // variable to store the value coming from the pots
int pot5Value = 0;  // variable to store the value coming from the pots
int pot6Value = 0;  // variable to store the value coming from the pots
int pot7Value = 0;  // variable to store the value coming from the pots
int pot8Value = 0;  // variable to store the value coming from the pots
int lastpot1Value = 0;  // variable to store the value coming from the pots
int lastpot2Value = 0;  // variable to store the value coming from the pots
int lastpot3Value = 0;  // variable to store the value coming from the pots
int lastpot4Value = 0;  // variable to store the value coming from the pots
int lastpot5Value = 0;  // variable to store the value coming from the pots
int lastpot6Value = 0;  // variable to store the value coming from the pots
int lastpot7Value = 0;  // variable to store the value coming from the pots
int lastpot8Value = 0;  // variable to store the value coming from the pots

float smoothPot1 = 0;
float smoothPot2 = 0;
float smoothPot3 = 0;
float smoothPot4 = 0;
float smoothPot5 = 0;
float smoothPot6 = 0;
float smoothPot7 = 0;
float smoothPot8 = 0;

unsigned long stepMicros = 62500; //120bpm default (7.5 / 120)ms = 0.0625 s * 1,000,000

const int inputLatency = 30; //in ms, query the inputs every XX mS. Found 30ms Is the fastest reliable time for updating the trellis key inputs, otherwise you get double hits.
const int serialLatency = 1000; //in ms, query the inputs every XX mS

Metro inputsMetro = Metro(inputLatency);  // pickup keypresses
Metro serialMetro = Metro(serialLatency);  // Instantiate an instance

void loopSequencer();
void trellisBootSequence()
{
   // light up all the LEDs in order
  for (uint8_t j=0; j<numKeys; j++) {
    trellis.setLED(j);
    trellis.writeDisplay();    
    delay(5);
  }
  // then turn them off
  for (uint8_t j=0; j<numKeys; j++) {
    trellis.clrLED(j);
    trellis.writeDisplay();    
    delay(5);
  }
}

void updateBPM(byte newBPM) {
  float factor = 32 / stepResolution;
	if (newBPM==0){
		Serial.println("Sequencer Halted");
		Serial.print(currentBPM);
		Serial.println("bpm");
		}
		else
		{
          		stepMicros = int32_t((((float(float(60)/float(newBPM))/(stepLength/4))*1000)*1000)*factor);
			currentBPM = newBPM;
			Serial.print("BPM Updated: ");
                        Serial.print(currentBPM);
                        Serial.print(" - ");
                        Serial.print(stepMicros);
                        Serial.println(" Microseconds per step");
                        
			//tick1 = pulsesPerBar / 4;
			//tick2 = tick1 * 2;
			//tick3 = tick2+tick1;			
		}
}


void setupPatterns()
{
  padWords[0] = 1;
  padWords[1] = 2;
  padWords[2] = 4;
  padWords[3] = 8;
  padWords[4] = 16;
  padWords[5] = 32;
  padWords[6] = 64;
  padWords[7] = 128;
  padWords[8] = 256;
  padWords[9] = 512;
  padWords[10] = 1024;
  padWords[11] = 2048;
  padWords[12] = 4096;
  padWords[13] = 8192;
  padWords[14] = 16384;
  padWords[15] = 32768;  
  
  for (uint8_t x=0; x<3; x++) {
     for (uint8_t y=0; y<8; y++) {
       for (uint8_t z=0; z<32; z++) {
       PatternsNoteOn[x][y][z]=0; // zero all the data to be sure.
       PatternsNoteOff[x][y][z]=0; // zero all the data to be sure.
       }
     }
  }
}

void clearSequencerLEDs(){
  for (uint8_t i=0; i < 32; i++) {
    trellis.clrLED(i);
  }
}

void clearBottomLeftLEDs(){
  for (uint8_t i=0; i < 16; i++) {
    trellis.clrLED(i+32);
  }
}

void setSequencerLED(byte sequencerLED){
  if (sequencerLED < 4) { trellis.setLED(sequencerLED);}
  else if (sequencerLED < 8) { trellis.setLED(sequencerLED+12);}
  else if (sequencerLED < 12) { trellis.setLED(sequencerLED-4);}
  else if (sequencerLED < 16) { trellis.setLED(sequencerLED+8);}
  else if (sequencerLED < 20) { trellis.setLED(sequencerLED-8);}
  else if (sequencerLED < 24) { trellis.setLED(sequencerLED+4);}
  else if (sequencerLED < 28) { trellis.setLED(sequencerLED-12);}
  else {trellis.setLED(sequencerLED);}
}

void flipSequencerLED(byte sequencerLED){
  if (sequencerLED < 4) { if (trellis.isLED(sequencerLED)) {trellis.clrLED(sequencerLED);} else {trellis.setLED(sequencerLED);}}
  else if (sequencerLED < 8) { if (trellis.isLED(sequencerLED+12)) {trellis.clrLED(sequencerLED+12);} else {trellis.setLED(sequencerLED+12);}}
  else if (sequencerLED < 12) { if (trellis.isLED(sequencerLED-4)) {trellis.clrLED(sequencerLED-4);} else {trellis.setLED(sequencerLED-4);}}
  else if (sequencerLED < 16) { if (trellis.isLED(sequencerLED+8)) {trellis.clrLED(sequencerLED+8);} else {trellis.setLED(sequencerLED+8);}}
  else if (sequencerLED < 20) { if (trellis.isLED(sequencerLED-8)) {trellis.clrLED(sequencerLED-8);} else {trellis.setLED(sequencerLED-8);}}
  else if (sequencerLED < 24) { if (trellis.isLED(sequencerLED+4)) {trellis.clrLED(sequencerLED+4);} else {trellis.setLED(sequencerLED+4);}}
  else if (sequencerLED < 28) { if (trellis.isLED(sequencerLED-12)) {trellis.clrLED(sequencerLED-12);} else {trellis.setLED(sequencerLED-12);}}
  else { if (trellis.isLED(sequencerLED)) {trellis.clrLED(sequencerLED);} else {trellis.setLED(sequencerLED);}}
}

byte translateSequencerSteptoPad(byte seqStep){ //could do with checking
  if (seqStep < 4) { return seqStep;}
  else if (seqStep < 8) { return (seqStep+12);}
  else if (seqStep < 12) { return (seqStep-4);}
  else if (seqStep < 16) { return (seqStep+8);}
  else if (seqStep < 20) { return (seqStep-8);}
  else if (seqStep < 24) { return (seqStep+6);}
  else if (seqStep < 28) { return (seqStep-5);}
  else {return seqStep;}
}

byte translateSequencerPadtoStep(byte seqPad){ //correct and tested
  if (seqPad < 4) { return seqPad;}
  else if (seqPad < 8) { return (seqPad+4);}
  else if (seqPad < 12) { return (seqPad+8);}
  else if (seqPad < 16) { return (seqPad+12);}
  else if (seqPad < 20) { return (seqPad-12);}
  else if (seqPad < 24) { return (seqPad-8);}
  else if (seqPad < 28) { return (seqPad-4);}
  else {return seqPad;}
}

void clearSequencerLED(byte sequencerLED){
  if (sequencerLED < 4) { trellis.clrLED(sequencerLED);}
  else if (sequencerLED < 8) { trellis.clrLED(sequencerLED+12);}
  else if (sequencerLED < 12) { trellis.clrLED(sequencerLED-4);}
  else if (sequencerLED < 16) { trellis.clrLED(sequencerLED+8);}
  else if (sequencerLED < 20) { trellis.clrLED(sequencerLED-8);}
  else if (sequencerLED < 24) { trellis.clrLED(sequencerLED+4);}
  else if (sequencerLED < 28) { trellis.clrLED(sequencerLED-12);}
  else {trellis.clrLED(sequencerLED);}
}


void sequencerUpdate(){
  //update other leds that should be on this cycle.
  for (uint8_t i=0; i < 32; i++) {
    if (PatternsNoteOn[currentMode][currentPattern[currentMode]][i] & padWords[currentPad]){setSequencerLED(i);}
  }
}


void patternUpdate()
{
   //Serial.print("MIDI Step: ");
   //Serial.println(currentStep);
   clearSequencerLEDs();
   sequencerUpdate();
   flipSequencerLED(currentStep);
}

void loopSequencer(){
  
    for (uint8_t j=0; j < 16; j++) {
      if (currentStep == 0){ //this fixes the currentStep 0 loop point, if its zero, we can't look at currentStep-1!!
       for (uint8_t instrument=0; instrument < 3; instrument++) {
         if (PatternsNoteOff[instrument][currentPattern[instrument]][currentStep+31] & padWords[j]) { // if there are any noteOffs pending, run them first of all.PatternsNoteOff[0][0][currentStep+31]
            usbMIDI.sendNoteOff(j+64, defaultVelocity, instrument+midiChannelOffset);
            usbMIDI.send_now();
            //PatternsNoteOff[instrument][currentPattern[instrument]][currentStep+31] = PatternsNoteOff[instrument][currentPattern[instrument]][currentStep+31] ^ padWords[j]; //xor clears the bit 
         }
       }
      }
      else { //ok, we're not on zero, we can just look at the step behind us now, currentStep -1 is fine.
        for (uint8_t instrument=0; instrument < 3; instrument++) {
          if (PatternsNoteOff[instrument][currentPattern[instrument]][currentStep-1] & padWords[j]) { // if there are any noteOffs pending, run them first of all.
            usbMIDI.sendNoteOff(j+64, defaultVelocity, instrument+midiChannelOffset);
            usbMIDI.send_now();
            //PatternsNoteOff[instrument][currentPattern[instrument]][currentStep-1] = PatternsNoteOff[instrument][currentPattern[instrument]][currentStep-1] ^ padWords[j]; //xor clears the bit
          }
         }
      }
      
      for (uint8_t instrument=0; instrument < 3; instrument++) {
        if (PatternsNoteOn[instrument][currentPattern[instrument]][currentStep] & padWords[j]){ // Any new notes to trigger on this step? then havea look and see about triggering notes.
          //usbMIDI.sendNoteOff(j+64, defaultVelocity, seq1MidiChannel);
          usbMIDI.sendNoteOn(j+64, defaultVelocity, instrument+midiChannelOffset);
          usbMIDI.send_now();
          
        }
      }
         
  }
  
  if (currentStep < 31){ currentStep++;} else {currentStep = 0;}
}

uint8_t returnMIDINote(uint8_t note, uint8_t octave) // note is a value from 0-11, octave is an octave from 0 to 10 - see MIDINoteDefs
{
    uint8_t result;
    result = (octave*12)+note;
    return result;
}

void loopTrellis()
{
  if (trellis.readSwitches()){
      Serial.println("trellis pressed");
      //Modes
      if (trellis.justPressed(56)) {currentMode=ModeSeq1; trellis.setLED(56); trellis.clrLED(57); trellis.clrLED(58); trellis.clrLED(59);}
      if (trellis.justPressed(57)) {currentMode=ModeSeq2; trellis.setLED(57); trellis.clrLED(56); trellis.clrLED(58); trellis.clrLED(59);}
      if (trellis.justPressed(58)) {currentMode=ModeSeq3; trellis.setLED(58); trellis.clrLED(56); trellis.clrLED(57); trellis.clrLED(59);}
      if (trellis.justPressed(59)) {currentMode=ModePattern; trellis.setLED(59); trellis.clrLED(56); trellis.clrLED(57); trellis.clrLED(58);}
      //Transport
      if (trellis.justPressed(60)) {currentRecording = true; currentOverdub=false; currentPlaying=true; currentStopped=false;
                                    trellis.setLED(60); trellis.setLED(63); trellis.clrLED(61); trellis.clrLED(62);}
      if (trellis.justPressed(61)) {currentRecording = false; currentOverdub=true; currentPlaying=true; currentStopped=false;
                                    trellis.setLED(61); trellis.setLED(63); trellis.clrLED(60); trellis.clrLED(62);}
      if (trellis.justPressed(62)) {if (currentStopped)
                                      {currentStep = 0; clearSequencerLEDs(); trellis.setLED(0);} 
                                    else 
                                      {currentRecording = false; currentOverdub=false; currentPlaying = false; currentStopped=true; 
                                       trellis.clrLED(60); trellis.clrLED(61); trellis.setLED(62); trellis.clrLED(63);}
                                   }
      if (trellis.justPressed(63)) {currentRecording = false; currentOverdub=false; currentStopped = false; currentPlaying = true; 
                                    trellis.clrLED(60); trellis.clrLED(61); trellis.clrLED(62); trellis.setLED(63);}
      if (currentMode<3){
                for (uint8_t i=32; i < 48; i++) { //if the playPad has been pressed....
                  if (trellis.justPressed(i)) {
                    clearBottomLeftLEDs(); 
                    trellis.setLED(i); 
                    currentPad = i-32;
                    if (currentOverdub | currentRecording) { //if we're recording then punch in your note to the sequencer
                      PatternsNoteOn[currentMode][currentPattern[currentMode]][currentStep] = PatternsNoteOn[currentMode][currentPattern[currentMode]][currentStep] | padWords[currentPad]; 
                      } 
                    usbMIDI.sendNoteOn(i+32, defaultVelocity, currentMode+midiChannelOffset); //don't forget to play it!
                    usbMIDI.send_now();
                  }
                  if (trellis.justReleased(i)) {
                    if (currentOverdub | currentRecording) { //if we're recording then punch in your note to the sequencer
                        PatternsNoteOff[currentMode][currentPattern[currentMode]][currentStep] = PatternsNoteOff[currentMode][currentPattern[currentMode]][currentStep] | padWords[currentPad]; 
                      } 
                  usbMIDI.sendNoteOff(i+32, defaultVelocity, currentMode+midiChannelOffset);
                  usbMIDI.send_now();
                  }                                    
                }
                for (uint8_t i=0; i < 32; i++) {
                  if (trellis.justPressed(i)) {
                    byte theStep = translateSequencerPadtoStep(i);
                    Serial.print("Step:");
                    Serial.println(theStep);
                                       
                    if ((PatternsNoteOn[currentMode][currentPattern[currentMode]][theStep] & padWords[currentPad])==padWords[currentPad]) //already set? disable it.
                        {
                          clearSequencerLED(theStep); PatternsNoteOn[currentMode][currentPattern[currentMode]][theStep]  = PatternsNoteOn[currentMode][currentPattern[currentMode]][theStep] ^ padWords[currentPad]; //xor it, if both the padWords bitmap and pattern have any bits set, they will change to 0
                          Serial.print("Clear LED ");
                          Serial.println(theStep);
                        }
                    else                                    // not set, add it.
                        {
                          setSequencerLED(theStep); PatternsNoteOn[currentMode][currentPattern[currentMode]][theStep] = PatternsNoteOn[currentMode][currentPattern[currentMode]][theStep] | padWords[currentPad];
                          //PatternsNoteOff[currentMode][currentPattern[currentMode]][theStep+1] |= padWords[currentPad]; //set the noteoff to run on the next step, for now. 
                          PatternsNoteOff[currentMode][currentPattern[currentMode]][theStep+1] = PatternsNoteOff[currentMode][currentPattern[currentMode]][theStep+1] | padWords[currentPad];
                          Serial.print("Set LED ");
                          Serial.println(theStep);
                        }
                   }                    
                }
                
                }
    
    
    else if (currentMode==ModePattern){ //do patterny stuff 
    }
  }

}

void OnNoteOn(byte channel, byte note, byte velocity)
{
  if (note<64) {  trellis.setLED(note); }
}

void OnNoteOff(byte channel, byte note, byte velocity)
{
  if (note<64) {  trellis.clrLED(note); }
}

void loopAnalogue() {
//  smoothPot1 = 0.9 * smoothPot1 + 0.1 * analogRead(pot1Pin); //signal smoothing
//  pot1Value = int16_t(smoothPot1)/8; // Divide 0-1023 by 8 for 0-127
  smoothPot2 = 0.9 * smoothPot2 + 0.1 * analogRead(pot2Pin);
  pot2Value = int16_t(smoothPot2)/8;
  smoothPot3 = 0.9 * smoothPot3 + 0.1 * analogRead(pot3Pin);
  pot3Value = int16_t(smoothPot3)/8;
  smoothPot4 = 0.9 * smoothPot4 + 0.1 * analogRead(pot4Pin);
  pot4Value = int16_t(smoothPot4)/8;
  smoothPot5 = 0.9 * smoothPot5 + 0.1 * analogRead(pot5Pin);
  pot5Value = int16_t(smoothPot5)/8;
  smoothPot6 = 0.9 * smoothPot6 + 0.1 * analogRead(pot6Pin);
  pot6Value = int16_t(smoothPot6)/8;
  smoothPot7 = 0.9 * smoothPot7 + 0.1 * analogRead(pot7Pin);
  pot7Value = int16_t(smoothPot7)/8;
  smoothPot8 = 0.9 * smoothPot8 + 0.1 * analogRead(pot8Pin);
  pot8Value = int16_t(smoothPot8)/8;

   pot1Value = ((analogRead(pot1Pin)* 0.8)+0.2)/8; // Divide 0-1023 by 8 for 0-127
//   pot2Value = ((analogRead(pot2Pin)* 0.8)+0.2)/8; // Divide 0-1023 by 8 for 0-127
//   pot3Value = ((analogRead(pot3Pin)* 0.8)+0.2)/8; // Divide 0-1023 by 8 for 0-127
//   pot4Value = ((analogRead(pot4Pin)* 0.8)+0.2)/8; // Divide 0-1023 by 8 for 0-127
//   pot5Value = ((analogRead(pot5Pin)* 0.8)+0.2)/8; // Divide 0-1023 by 8 for 0-127
//   pot6Value = ((analogRead(pot6Pin)* 0.8)+0.2)/8; // Divide 0-1023 by 8 for 0-127
//   pot7Value = ((analogRead(pot7Pin)* 0.8)+0.2)/8; // Divide 0-1023 by 8 for 0-127
//   pot8Value = ((analogRead(pot8Pin)* 0.8)+0.2)/8; // Divide 0-1023 by 8 for 0-127
   
//  smoothPot2 = 0.9 * smoothPot2 + 0.1 * analogRead(pot2Pin);
  
  if (lastpot1Value != pot1Value) {
  //usbMIDI.sendControlChange(70, pot1Value, currentChannel);
  //Serial.print("Pot1 : ");
  //Serial.println(pot1Value);
  int val = pot1Value + minimumBPM; 
  updateBPM(val);
  lastpot1Value = pot1Value;}
  
  if (lastpot2Value != pot2Value) {
  //usbMIDI.sendControlChange(71, pot2Value, currentChannel);
  Serial.print("Pot2 : ");
  Serial.println(pot2Value);
  lastpot2Value = pot2Value;} 
  
  if (lastpot3Value != pot3Value) {
  //usbMIDI.sendControlChange(72, pot3Value, currentChannel);
  Serial.print("Pot3 : ");
  Serial.println(pot3Value);
  pixGrn = pot3Value * 2;
  lastpot3Value = pot3Value;}
  
  if (lastpot4Value != pot4Value) {
  Serial.print("Pot4 : ");
  Serial.println(pot4Value);
  //usbMIDI.sendControlChange(73, pot4Value, currentChannel);
  pixBlu = pot4Value * 2;
  lastpot4Value = pot4Value;}
  
  if (lastpot5Value != pot5Value) {
  //usbMIDI.sendControlChange(74, pot5Value, currentChannel);
  Serial.print("Pot5 : ");
  Serial.println(pot5Value);
  lastpot5Value = pot5Value;} 
  
  if (lastpot6Value != pot6Value) {
  //usbMIDI.sendControlChange(75, pot6Value, currentChannel);
  pixRed = pot6Value * 2;
  Serial.print("Pot6 : ");
  Serial.println(pot6Value);
  lastpot6Value = pot6Value;}
 
 if (lastpot7Value != pot7Value) {
  //usbMIDI.sendControlChange(76, pot7Value, currentChannel);
  
  lastpot7Value = pot7Value;} 
  
 if (lastpot8Value != pot8Value) {
  usbMIDI.sendControlChange(77, pot8Value, currentChannel);
  Serial.print("Pot8 : ");
  Serial.println(pot8Value);
  lastpot8Value = pot8Value;}  
  
  
  //pot2Value = analogRead(pot2Pin);
  //pot3Value = analogRead(pot3Pin);
  //pot4Value = analogRead(pot4Pin);
  //pot5Value = analogRead(pot5Pin);
  //pot6Value = analogRead(pot6Pin);
  //pot7Value = analogRead(pot7Pin);
  //pot8Value = analogRead(pot8Pin);
}


void loopUpdateInputs() {
  Serial.println("loopUpdateInputs ");
  loopTrellis(); // update I/O from trellis
  loopAnalogue(); // update I/O from analogue pots
  patternUpdate(); // update remaining trellis led situation
  //int a = touchRead(0);
  //Serial.print("Touch: ");
  //Serial.println(a);
}

void setup() {
  // declare the ledPin as an OUTPUT:
  pinMode(ledPin, OUTPUT); 
  pinMode(TRELLISINTPIN, INPUT);
  digitalWrite(TRELLISINTPIN, HIGH); 
  Serial.begin(9600);
  delay(1000);
  Serial.println("Serial output running");
  trellis.begin(0x70, 0x71, 0x72, 0x73);  
  trellis.setBrightness(trellisBrightness);
    trellisBootSequence();
    usbMIDI.setHandleNoteOff(OnNoteOff);
    usbMIDI.setHandleNoteOn(OnNoteOn);
   Serial.println("setupPatterns();");
   setupPatterns(); 
   currentMode=ModeSeq1; //boot into sequencer
   trellis.setLED(56);  //tell the user the mode
   trellis.setLED(62); // start in stopped mode.
   trellis.setLED(0); // start in stopped mode.
   currentStep = 0;
   // setMode at bootup
   trellis.setLED(32); //set to 
   currentPad = 0;
   //
   currentPlaying = false;
   currentStopped = true;
   currentRecording = false;
   currentOverdub = false;
   Serial.println("updateBPM(96);");
   updateBPM(96);
   Serial.println("trellis.writeDisplay();");
   trellis.writeDisplay();
   strip.begin();
   strip.show(); // Initialize all pixels to 'off'
}


void loop() {
  // This is the high technology sequencer - no interrupts, just exists in the loop. It is quite possible it could lose time at some point. 
  if (seqTimer > stepMicros) {
    seqTimer = 0; //reset the timer
    if (currentPlaying) {loopSequencer();
    }
  }
  
  
  if (inputsMetro.check() == 1) { // check if the metro has passed it's interval .
      loopUpdateInputs();
      strip.setPixelColor(0, pixRed, pixGrn, pixBlu);
      strip.show();
      
      
      while (usbMIDI.read(currentChannel)) { }// check for midi input and update the LEDs.
      trellis.writeDisplay();
      }
       
      if (serialMetro.check() == 1) { // check if the metro has passed it's interval .
      }
      
}
