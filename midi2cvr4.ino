

#include <MIDI.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>

#define MIDI_CHANNEL 6

#define CC_CONTROL 31

#define MIN_C 36

#define MIDI_LED 12
#define PITCH_LED 7
#define GATE_LED 6
#define CC_LED 5

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

Adafruit_MCP4725 adafruit_dac;
int currentPitchDac = 0;
int currentCCDac = 0;
byte currentVelocity = 0;
volatile bool needsToUpdate = false;
unsigned long thisTime = 0;

class BlinkingLed {
public:
    BlinkingLed(byte pin) : pin(pin) {
    }

    void setup() {
        pinMode(pin, OUTPUT);
    }

    void setOn(unsigned long time) {
        state = true;
        endTime = time + blinkTime;
        digitalWrite(pin, HIGH);
    }

    void loop(unsigned long time) {
        if ((time >= endTime) && state) {
            state = false;
            digitalWrite(pin, LOW);
        }
    }

private:
    byte pin = 0;
    bool state = false;
    unsigned long endTime = 0;
    static const unsigned long blinkTime = 100;
};

BlinkingLed pitchLed = BlinkingLed(PITCH_LED);
BlinkingLed midiLed = BlinkingLed(MIDI_LED);
BlinkingLed ccLed = BlinkingLed(CC_LED);

void setup() {
  // Initialize serial communication and wait up to 2.5 seconds for a connection
  Serial.begin(31250);
  Serial1.begin(31250);
  
  for (auto startNow = millis() + 2500; !Serial && millis() < startNow; delay(500));

  // For Adafruit MCP4725A1 the address is 0x62 (default) or 0x63 (ADDR pin tied to VCC)
  // For MCP4725A0 the address is 0x60 or 0x61
  // For MCP4725A2 the address is 0x64 or 0x65
  // To resume : if this current address doesn't work, try between 0x60 and 0x65
  adafruit_dac.begin(0x60);
  
  // Set DAC resolution to 12-bit for maximum precision
  analogWriteResolution(12);

  pinMode(LED_BUILTIN, OUTPUT);

  ccLed.setup();
  pitchLed.setup();
  pinMode(GATE_LED, OUTPUT);
  midiLed.setup();
  
  Serial.println("- Arduino Nano R4 - DAC Basic Output Example started...");
  Serial.println("- Generating precise voltages on pin A0");
  Serial.println("- Connect a multimeter to A0 to measure output");

  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);

  MIDI.begin(MIDI_CHANNEL_OMNI);
  
  MIDI.turnThruOn();
  
  digitalWrite(LED_BUILTIN, LOW);

  digitalWrite(MIDI_LED, HIGH);
  digitalWrite(PITCH_LED, HIGH);
  digitalWrite(GATE_LED, HIGH);
  digitalWrite(CC_LED, HIGH);

  delay(2000);

  digitalWrite(MIDI_LED, LOW);
  digitalWrite(PITCH_LED, LOW);
  digitalWrite(GATE_LED, LOW);
  digitalWrite(CC_LED, LOW);
}

int noteToVolt(int noteIn) {
    return (int)round(((float)(4095.0/5.0)/12.0) * noteIn);
}

void handleNoteOn(byte channel, byte note, byte velocity) {
    
    if (channel == MIDI_CHANNEL) {
        int inRange = constrain(note, MIN_C, MIN_C + 12*5);
        currentPitchDac = noteToVolt(inRange - MIN_C);

        needsToUpdate = true;
        currentVelocity = velocity;

        midiLed.setOn(thisTime);
        pitchLed.setOn(thisTime);
    }
}

void handleNoteOff(byte channel, byte note, byte velocity) {

    if (channel == MIDI_CHANNEL) {
        midiLed.setOn(thisTime);
        needsToUpdate = true;
        currentVelocity = 0;
    }
}


void handleControlChange(byte channel, byte control, byte value) {

    if (channel == MIDI_CHANNEL) {
        midiLed.setOn(thisTime);
        if (control == CC_CONTROL) {
            ccLed.setOn(thisTime);
            currentCCDac = map(value, 0, 127, 0, 4095);
            needsToUpdate = true;
        }
    }
}

void loop() {
    
  thisTime = millis();
  MIDI.read();

  pitchLed.loop(thisTime);
  midiLed.loop(thisTime);
  ccLed.loop(thisTime);

  if (needsToUpdate) {
    analogWrite(DAC, currentPitchDac);
    adafruit_dac.setVoltage(currentCCDac, false);
    bool gate = currentVelocity != 0;
    digitalWrite(LED_BUILTIN, gate ? HIGH : LOW);
    digitalWrite(GATE_LED, gate ? HIGH : LOW);
    needsToUpdate = false;
  }

}