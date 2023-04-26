#include <MIDI.h>
#include <midi_Defs.h>

#define BIT_ARRAY_TYPE unsigned int
#define BIT_ARRAY_SIZE (sizeof(BIT_ARRAY_TYPE) * 8)
#define BIT_SET(A, k) (A[(k) / BIT_ARRAY_SIZE] |= (1 << ((k) % BIT_ARRAY_SIZE)))
#define BIT_GET(A, k) (A[(k) / BIT_ARRAY_SIZE] & (1 << ((k) % BIT_ARRAY_SIZE)))
#define BIT_CLEAR(A, k) (A[(k) / BIT_ARRAY_SIZE] &= ~(1 << ((k) % BIT_ARRAY_SIZE)))

// Global bit array for note states (8*16 = 128 bits)
BIT_ARRAY_TYPE NOTES[8] = {0};
bool SUSTAIN_STATE = false;

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
    MIDI.sendNoteOn(note, velocity, channel);
    BIT_SET(NOTES, note & 0x7f);
}

void onNoteOff(const byte channel, const byte note, const byte velocity) {
    if (!SUSTAIN_STATE) {
        MIDI.sendNoteOff(note, 0, channel);
        BIT_CLEAR(NOTES, note & 0x7f);
    }
}

void onControlChange(const byte channel, const byte number, const byte value) {
    if (number == midi::MidiControlChangeNumber::Sustain) {
        onSustainPedal(channel, value >= 64);
    }
}

void onSustainPedal(const byte channel, const bool active) {
    SUSTAIN_STATE = active;
    digitalWrite(LED_BUILTIN, active ? HIGH : LOW);

    if (!active) {
        for (byte note = 0; note < 127; note++) {
            if (BIT_GET(NOTES, note)) {
                MIDI.sendNoteOff(note, 0, channel);
                BIT_CLEAR(NOTES, note);
            }
        }
    }
}
