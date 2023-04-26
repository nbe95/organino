#include <MIDI.h>
#include <midi_Defs.h>

enum State : byte {
    Off,
    On,
    Holding
};

State NOTES[127] = { State::Off };
bool SUSTAIN_ACTIVE = false;

// Create and bind the MIDI interface to the default hardware Serial port
MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {
    digitalWrite(LED_BUILTIN, LOW);

    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.setThruFilterMode(midi::Thru::Mode::Off);

    MIDI.setHandleNoteOn(onNoteOn);
    MIDI.setHandleNoteOff(onNoteOff);
    MIDI.setHandleControlChange(onControlChange);
}

void loop() {
    MIDI.read();
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
    // Note: Since the Sustain pedal cannot be divided to different  parts on
    // our Clavinova, we're simply using Sustenuto as a workaround...
    if (number == midi::MidiControlChangeNumber::Sostenuto) {
        onSustainPedal(channel, value >= 64);
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
