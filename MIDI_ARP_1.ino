// https://fortyseveneffects.github.io/arduino_midi_library/index.html

#include <MIDI.h>     // Add Midi Library for DIN connections
#include <MIDIUSB.h>  // USB library for... USB

#define LED 8  // Arduino Board LED is on Pin 8

//Create an instance of the library with default name, serial port and settings
MIDI_CREATE_DEFAULT_INSTANCE();

const int MAX_NOTES = 10;
midiEventPacket_t notesHeld[MAX_NOTES];  // Array to store MIDI data for held notes to arpeggiate over
uint8_t notesHeldCount = 0;              // Enables program to check if a note is held

void setup() {                    // setup arduino
  pinMode(LED, OUTPUT);           // Set Arduino board pin 8 to output
  MIDI.begin(MIDI_CHANNEL_OMNI);  // Initialize the Midi Library.
  // OMNI sets it to listen to all channels.. MIDI.begin(2) would set it
  // to respond to notes on channel 2 only.
  MIDI.turnThruOff();                  // Turns MIDI through off DIN to DIN
  MIDI.setHandleNoteOn(handleNoteOn);  // This is important!! This command
  // tells the Midi Library which function you want to call when a NOTE ON command
  // is received. In this case it's "handleNoteOn".
  MIDI.setHandleNoteOff(handleNoteOff);  // This command tells the Midi Library
  // to call "handleNoteOff" when a NOTE OFF command is received.
  MIDI.setHandleControlChange(handleControlChange);  // Set the callback function for control changes
}

void loop() {   // Main loop
  MIDI.read();  // Continuously check if Midi data has been received.

  // not sure if these two need to be here like this still figuring out the usb midi in
  MidiUSB.read();
  MidiUSB.flush();  //Send any MIDI data contained within buffer

  // Check if any notes are held, and set the LED state accordingly
  if (notesHeldCount >= 1) {
    digitalWrite(LED, HIGH);  // Turn the LED on if multiple notes are held
  } else {
    digitalWrite(LED, LOW);  // Turn the LED off if no or only one note is held
  }
  printNotesHeld();
  Serial.print("NOTES HELD: ");
  Serial.println(notesHeldCount);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MIDI IN: Handle Control Change messages
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// handleNoteON is the function that will be called by the Midi Library
// when a MIDI NOTE ON message is received.
// It will be passed bytes for Channel, Pitch, and Velocity
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void handleNoteOn(byte channel, byte note, byte velocity) {
  // Make sure array has room
  if (notesHeldCount < MAX_NOTES) {
    // Create new MIDI Event for USB the - 1 is because the MIDIUSB library misses the channel for some reason
    midiEventPacket_t noteOn = { 0x09, 0x90 | channel - 1, note, velocity };

    notesHeld[notesHeldCount] = noteOn;  // Add MIDI event to array
    notesHeldCount++;                    // Track if note is held

    triggerUSB_DIN(noteOn);  // Send to both USB and DIN connections
  }

  // Helpful for debugging
  //~~~~~~~~~~~~~~~~~~~~~~~
  // Serial.print("Note On - Channel: ");
  // Serial.print(channel);
  // Serial.print(", Note Number: ");
  // Serial.print(note);
  // Serial.print(", Velocity: ");
  // Serial.println(velocity);
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// handleNoteOFF is the function that will be called by the Midi Library
// when a MIDI NOTE OFF message is received.
// * A NOTE ON message with Velocity = 0 will be treated as a NOTE OFF message *
// It will be passed bytes for Channel, Pitch, and Velocity
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void handleNoteOff(byte channel, byte note, byte velocity) {
  // Create new MIDI Event for USB
  midiEventPacket_t noteOff = { 0x08, 0x80 | channel - 1, note, velocity };
  // Search for the note in the array and remove it
  for (int i = 0; i < notesHeldCount; ++i) {
    if (notesHeld[i].byte2 == note) {
      // Found the note, remove it from the array
      for (int j = i; j < notesHeldCount - 1; ++j) {
        notesHeld[j] = notesHeld[j + 1];
      }
      notesHeldCount--; //Track it

      // Trigger USB and DIN events
      triggerUSB_DIN(noteOff);

      break;  // Exit the loop after removing the note
    }
  }


  // Helpful for debugging
  //~~~~~~~~~~~~~~~~~~~~~~~
  // Serial.print("Note Off - Channel: ");
  // Serial.print(channel);
  // Serial.print(", Note Number: ");
  // Serial.print(note);
  // Serial.print(", Velocity: ");
  // Serial.println(velocity);
}

// function for different types of 3 byte midi events will send events to both DIN and USB
void triggerUSB_DIN(midiEventPacket_t midiEvent) {
  // Status Byte
  // 2 parts: type and channel
  // Note on, Note off, Aftertouch, Control Change, Channel Pressure, Pitch Bend, or System
  uint8_t type = (midiEvent.byte1 & 0xF0) >> 4;    // Masking to get the upper 4 bits and shifting to the right
  uint8_t channel = (midiEvent.byte1 & 0x0F) + 1;  // Masking to get the lower 4 bits adding one to correct for USB MIDI packet

  // Data Byte 2
  // Note, Control number, Program Number, or Pressure
  uint8_t note = midiEvent.byte2;

  // Data Byte 3
  // Velocity, pressure, or control value
  uint8_t velocity = midiEvent.byte3;

  switch (type) {
    case 8:  // note off condition
      MIDI.sendNoteOff(note, velocity, channel);
      MidiUSB.sendMIDI(midiEvent);  // Send MIDI event to USB
      MidiUSB.flush();              // Send midi event immediately
      break;
    case 9:  // note on condition
      MIDI.sendNoteOn(note, velocity, channel);
      MidiUSB.sendMIDI(midiEvent);  // Send MIDI event to USB
      MidiUSB.flush();              // Send midi event immediately
      break;
  }


  // Helpful for debugging
  //~~~~~~~~~~~~~~~~~~~~~~~
  // Serial.print("Type: ");
  // Serial.print(type);
  // Serial.print(" Channel: ");
  // Serial.print(channel);
  // Serial.print(" Note: ");
  // Serial.print(note);
  // Serial.print(" Velocity: ");
  // Serial.println(velocity);
}

void printNotesHeld() {
  Serial.println("Notes Held:");
  for (int i = 0; i < notesHeldCount; ++i) {
    midiEventPacket_t note = notesHeld[i];
    Serial.print("Note ");
    Serial.print(i + 1);
    Serial.print(": Channel ");
    Serial.print((note.byte1 & 0x0F) + 1);  // Extract channel
    Serial.print(", Pitch ");
    Serial.print(note.byte2);
    Serial.print(", Velocity ");
    Serial.println(note.byte3);
  }
  Serial.println("End of Notes Held");
}