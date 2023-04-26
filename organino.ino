// Copyright (c) 2023 Niklas Bettgen

#include <MIDI.h>
#include <midi_Defs.h>

#include "timer.h"

// Configuration
#define ROTARY_SPEED_PIN 4
#define ROTARY_SPEED_PULSE_MS 50

// Global note and sustain status
enum State : byte { Off, On, Holding };
State NOTES[127] = { State::Off };
bool SUSTAIN_ACTIVE = false;

// Timer for rotary pedal pulse
Timer ROTARY_TIMER(ROTARY_SPEED_PULSE_MS);

// Create and bind the MIDI interface to the default hardware Serial port
MIDI_CREATE_DEFAULT_INSTANCE();


void setup() {
    // Hardware setup
    pinMode(ROTARY_SPEED_PIN, OUTPUT);
    digitalWrite(ROTARY_SPEED_PIN, LOW);
    digitalWrite(LED_BUILTIN, LOW);

    // Set up MIDI interface on Serial port
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.setThruFilterMode(midi::Thru::Mode::Off);

    // Register callbacks
    MIDI.setHandleNoteOn(onNoteOn);
    MIDI.setHandleNoteOff(onNoteOff);
    MIDI.setHandleControlChange(onControlChange);
}

void loop() {
    // Handle incoming MIDI traffic
    MIDI.read();

    // Reset pedal pulse for the rotary speed
    if (ROTARY_TIMER.check()) {
        digitalWrite(ROTARY_SPEED_PIN, LOW);
        ROTARY_TIMER.reset();
    }
}

void onNoteOn(const byte channel, const byte note, const byte velocity) {
    if (NOTES[note & 0x7f] == State::Off) {
        MIDI.sendNoteOn(note, velocity, channel);
    }
    if (NOTES[note & 0x7f] != State::On) {
        NOTES[note & 0x7f] = State::On;
    }
}

void onNoteOff(const byte channel, const byte note, const byte velocity) {
    if (NOTES[note & 0x7f] == State::On) {
        if (SUSTAIN_ACTIVE) {
            NOTES[note & 0x7f] = State::Holding;
        } else {
            MIDI.sendNoteOff(note, 0, channel);
            NOTES[note & 0x7f] = State::Off;
        }
    }
}

void onControlChange(const byte channel, const byte number, const byte value) {
    // Since the Sustain pedal cannot be divided to different parts on
    // our Clavinova, we're simply using Sustenuto as a workaround.
    if (number == midi::MidiControlChangeNumber::Sostenuto)
        onSustainPedal(channel, value >= 64);

    // Any positive edge of an incoming Portamento will trigger rotary speed.
    if (number == midi::MidiControlChangeNumber::Portamento)
        onPortamentoPedal(value >= 64);
}

void onPortamentoPedal(const bool active) {
    if (active) {
        digitalWrite(ROTARY_SPEED_PIN, HIGH);
        ROTARY_TIMER.restart();
    }
}

void onSustainPedal(const byte channel, const bool active) {
    SUSTAIN_ACTIVE = active;
    digitalWrite(LED_BUILTIN, active ? HIGH : LOW);

    if (!active) {
        for (byte note = 0; note < 127; note++) {
            if (NOTES[note & 0x7f] == State::Holding) {
                MIDI.sendNoteOff(note, 0, channel);
                NOTES[note & 0x7f] = State::Off;
            }
        }
    }
}
