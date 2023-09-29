// https://fortyseveneffects.github.io/arduino_midi_library/index.html

#include <MIDI.h>     // Add Midi Library for DIN connections
#include <MIDIUSB.h>  // USB library for... USB

#define LED 8  // Arduino Board LED is on Pin 8

//Create an instance of the library with default name, serial port and settings
MIDI_CREATE_DEFAULT_INSTANCE();

midiEventPacket_t notesHeld[10];  // Array to store MIDI data for held notes
int notesHeldCount = 0;           // Enables program to check if a note is held

void setup() {                    // setup arduino
  pinMode(LED, OUTPUT);           // Set Arduino board pin 8 to output
  MIDI.begin(MIDI_CHANNEL_OMNI);  // Initialize the Midi Library.
  // OMNI sets it to listen to all channels.. MIDI.begin(2) would set it
  // to respond to notes on channel 2 only.
  MIDI.turnThruOff();                                // Turns MIDI through off DIN to DIN
  MIDI.setHandleControlChange(handleControlChange);  // Set the callback function for control changes
  MIDI.setHandleNoteOn(handleNoteOn);                // This is important!! This command
  // tells the Midi Library which function you want to call when a NOTE ON command
  // is received. In this case it's "handleNoteOn".
  MIDI.setHandleNoteOff(handleNoteOff);  // This command tells the Midi Library
  // to call "handleNoteOff" when a NOTE OFF command is received.
}

void loop() {   // Main loop
  MIDI.read();  // Continuously check if Midi data has been received.
  MidiUSB.read();
  MidiUSB.flush();  //Send any MIDI data contained within buffer

  // Check if any notes are held, and set the LED state accordingly
  if (notesHeldCount >= 1) {
    digitalWrite(LED, HIGH);  // Turn the LED on if multiple notes are held
  } else {
    digitalWrite(LED, LOW);  // Turn the LED off if no or only one note is held
  }
}

// MIDI IN: Handle Control Change messages
void handleControlChange(byte channel, byte control, byte value) {

  MIDI.sendControlChange(control, value, channel);

  // Send MIDI event to USB
  midiEventPacket_t event = { 0x0B, 0xB0 | channel, control, value };
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();

  Serial.print("Control Change (CC) - Channel: ");
  Serial.print(channel);
  Serial.print(", Control: ");
  Serial.print(control);
  Serial.print(", Value: ");
  Serial.println(value);
}

// handleNoteON is the function that will be called by the Midi Library
// when a MIDI NOTE ON message is received.
// It will be passed bytes for Channel, Pitch, and Velocity
void handleNoteOn(byte channel, byte note, byte velocity) {
  notesHeldCount++;  // Track if note is held


  // MIDI.sendNoteOn(note, velocity, channel);  // Send MIDI to DIN


  // Create new MIDI Event for USB
  midiEventPacket_t noteOn = { 0x09, 0x90 | channel, note, velocity };
  // MidiUSB.sendMIDI(noteOn);  // Send MIDI event to USB
  // MidiUSB.flush();           // Send midi event immediately

  triggerUSB_DIN(noteOn);  // Send to both USB and DIN connections

  Serial.print("Note On - Channel: ");
  Serial.print(channel);
  Serial.print(", Note Number: ");
  Serial.print(note);
  Serial.print(", Velocity: ");
  Serial.println(velocity);
}

// function for different types of 3 byte midi events will send events to both DIN and USB
void triggerUSB_DIN(midiEventPacket_t midiEvent) {
  uint8_t type = (midiEvent.byte1 & 0xF0) >> 4;  // Masking to get the upper 4 bits and shifting to the right
  uint8_t channel = (midiEvent.byte1 & 0x0F);    // Masking to get the lower 4 bits
  uint8_t note = midiEvent.byte2;
  uint8_t velocity = midiEvent.byte3;



  // note off condition
  if (type == 8) {
    Serial.println("DIN off");
    MIDI.sendNoteOff(note, velocity, channel);
  } else if (type == 9) {
    Serial.println("DIN on");
    MIDI.sendNoteOn(note, velocity, channel);
  }

  uint8_t header = midiEvent.header;
  Serial.print("Header: ");
  Serial.println(header);
  // Status Byte
  // 2 parts the type and the channel
  // Note on, Note off, Aftertouch, Control Change, Channel Pressure, Pitch Bend, or System
  uint8_t midi1 = midiEvent.byte1;
  Serial.print("byte1 HEX: ");
  Serial.println(midi1, HEX);
  Serial.print("Type: ");
  Serial.println(type);
  Serial.print("Channel: ");
  Serial.println(channel);
  // Data Byte 1
  // Note, Control number, Program Number, or Pressure
  uint8_t midi2 = midiEvent.byte2;
  Serial.print("byte2: ");
  Serial.println(midi2);
  // Data Byte 2
  // Velocity, pressure, or control value
  uint8_t midi3 = midiEvent.byte3;
  Serial.print("byte3: ");
  Serial.println(midi3);
}

// handleNoteOFF is the function that will be called by the Midi Library
// when a MIDI NOTE OFF message is received.
// * A NOTE ON message with Velocity = 0 will be treated as a NOTE OFF message *
// It will be passed bytes for Channel, Pitch, and Velocity
void handleNoteOff(byte channel, byte note, byte velocity) {
  notesHeldCount--;  // Track if note is held

  // MIDI.sendNoteOff(note, velocity, channel);

  // Create new MIDI Event for USB
  midiEventPacket_t noteOff = { 0x08, 0x80 | channel, note, velocity };


  // MidiUSB.sendMIDI(noteOff);  // Send MIDI event to USB
  // MidiUSB.flush();            // Send midi event immediately

  triggerUSB_DIN(noteOff);  // Send to both USB and DIN connections

  Serial.print("Note Off - Channel: ");
  Serial.print(channel);
  Serial.print(", Note Number: ");
  Serial.print(note);
  Serial.print(", Velocity: ");
  Serial.println(velocity);
}