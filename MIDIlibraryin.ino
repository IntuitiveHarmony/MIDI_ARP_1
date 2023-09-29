// https://fortyseveneffects.github.io/arduino_midi_library/index.html

#include <MIDI.h>     // Add Midi Library for DIN connections
#include <MIDIUSB.h>  // USB library for... USB

#define LED 8  // Arduino Board LED is on Pin 8

//Create an instance of the library with default name, serial port and settings
MIDI_CREATE_DEFAULT_INSTANCE();

int notesHeldCount = 0;  // Enables program to check if a note is held

void setup() {                    // setup arduino
  pinMode(LED, OUTPUT);           // Set Arduino board pin 8 to output
  MIDI.begin(MIDI_CHANNEL_OMNI);  // Initialize the Midi Library.
  // OMNI sets it to listen to all channels.. MIDI.begin(2) would set it
  // to respond to notes on channel 2 only.
  MIDI.turnThruOff();  // Turns MIDI through off DIN to DIN
  MIDI.setHandleControlChange(handleControlChange); // Set the callback function for control changes
  MIDI.setHandleNoteOn(handleNoteOn);  // This is important!! This command
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
  midiEventPacket_t event = { 0x0B, 0xB0 | channel - 1, control, value };
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
  notesHeldCount++; // Track if note is held

  MIDI.sendNoteOn(note, velocity, channel);  // Send MIDI to DIN


  // Create new MIDI Event for USB
  midiEventPacket_t noteOn = { 0x09, 0x90 | channel - 1, note, velocity };
  MidiUSB.sendMIDI(noteOn);  // Send MIDI event to USB
  MidiUSB.flush();           // Send midi event immediately

  Serial.print("Note On - Channel: ");
  Serial.print(channel);
  Serial.print(", Note Number: ");
  Serial.print(note);
  Serial.print(", Velocity: ");
  Serial.println(velocity);
}

// handleNoteOFF is the function that will be called by the Midi Library
// when a MIDI NOTE OFF message is received.
// * A NOTE ON message with Velocity = 0 will be treated as a NOTE OFF message *
// It will be passed bytes for Channel, Pitch, and Velocity
void handleNoteOff(byte channel, byte note, byte velocity) {
  notesHeldCount--; // Track if note is held

  MIDI.sendNoteOff(note, velocity, channel);

  // Create new MIDI Event for USB
  midiEventPacket_t noteOff = { 0x08, 0x80 | channel - 1, note, velocity };
  MidiUSB.sendMIDI(noteOff);  // Send MIDI event to USB
  MidiUSB.flush();            // Send midi event immediately

  Serial.print("Note Off - Channel: ");
  Serial.print(channel);
  Serial.print(", Note Number: ");
  Serial.print(note);
  Serial.print(", Velocity: ");
  Serial.println(velocity);
}