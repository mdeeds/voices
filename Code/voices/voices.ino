const int kNumPeriods=6;

#define kUlongMax 0xffffffffUL;

unsigned long attackUs = 10000; // 0.010 seconds
unsigned long decayUs = 10000;  // 0.010 seconds
byte sustain = 255;
unsigned long releaseUs = 500000; // Half a second.
byte sensitivity = 0;  // 0 to 127

unsigned long halfPeriod[kNumPeriods];
unsigned long nextTrigger[kNumPeriods];
bool lastValue[kNumPeriods];
unsigned char currentNote[kNumPeriods];
unsigned long releaseTimeUs[kNumPeriods];
byte voiceMaxAmplitude = 255 / kNumPeriods;
byte voiceLevel[kNumPeriods];

void setPulseFromNoteNumber(int voice, int noteNumber, byte velocity) {
  float h = (noteNumber - 69.0)/12.0;
  float freqHz = pow(2.0, h) * 440.0;
  float periodS = 1.0 / freqHz;
  unsigned long periodMicros = (unsigned long)(1000000 * periodS);
  halfPeriod[voice] = periodMicros / 2;
  nextTrigger[voice] = micros() + periodMicros / 2;
  currentNote[voice] = noteNumber;
  // https://docs.google.com/spreadsheets/d/1jr34u8wYBd19wifZUzedr6Hz2aj2d3egbtnYYMyftjw/edit#gid=0
  int amplitude = sensitivity + ((int)velocity * (127 - sensitivity)) / 127;
  voiceLevel[voice] = amplitude * voiceMaxAmplitude / 127; 
}

void setupVoices() {
  pinMode(LED_BUILTIN, OUTPUT);  // AKA D13
  digitalWrite(LED_BUILTIN, LOW);  
  for (int i=0; i<kNumPeriods; ++i) {
    currentNote[i] = 0;
    halfPeriod[i] = 0;
    lastValue[i] = 0;
    releaseTimeUs[i] = kUlongMax;
  }
}

// Mostly copied from https://www.instructables.com/id/Send-and-Receive-MIDI-with-Arduino/
void setupMidi() {
  Serial.begin(31250);
  cli();  //stop interrupts

  //set timer2 interrupt every 128us
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 7.8khz increments
  OCR2A = 255;// = (16*10^6) / (7812.5*8) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS11 bit for 8 prescaler
  TCCR2B |= (1 << CS11);   
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);
  
  sei();  //allow interrupts
}

void setupAnalogOut() {
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);

  analogWrite(9, 0);
  analogWrite (10, 0);

  // Set PWM to fast mode.  Do not call analogWrite
  // or it will override this setting.
  TCCR1B = 0x09;
}

#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

// writes a byte to the Fast PWM channels (9 and 10)
void setAnalogLevel(byte level){
  sbi(TCCR1A, COM1A1);
  OCR1AL = level;
  sbi(TCCR1A, COM1B1);
  OCR1BL = level;
}

void handleNoteEvent(byte channel, byte note, byte velocity) {
  if (channel != 0) {
    // Only handle MIDI channel 1.
    return;
  }
  if (velocity == 0) {
    for (int i = 0; i<kNumPeriods; ++i) {
      if (currentNote[i] == note) {
        releaseTimeUs[i] = micros() + releaseUs;
        break;
      }
    }
  } else {
    for (int i = 0; i < kNumPeriods; ++i) {
      if (currentNote[i] == note || currentNote[i] == 0) {
        setPulseFromNoteNumber(i, note, velocity);
        break;
      }
    }
  }
}

int serialThru() {
  int value = Serial.read();
  Serial.write(value);
  return value;
}

// Override the ISR for the serial interface
ISR(TIMER2_COMPA_vect) {
  while (Serial.available() >= 3) {
    // Note on and off messages are three bytes.
    byte commandByte = serialThru();//read first byte
    byte command = commandByte & 0xf0;
    byte channel = commandByte & 0x0f;
    byte noteByte = 0;
    byte velocityByte = 0;
    switch (command) {
      case 0x80: // Note OFF
      case 0x90: // Note ON
        noteByte = serialThru();
        velocityByte = serialThru();
        if (command == 0x80) {
          // Note off can also be coded as a note on with velocity zero.
          // So, if we get a note-off event, we just treat this as a note-on with velocity zero
          velocityByte = 0;
        }
        handleNoteEvent(channel, noteByte, velocityByte);
      break;
      // Other three-byte messages - Not implemented
      case 0xa0: // Polyohonic aftertouch
      case 0xb0: // Control change
      case 0xe0: // Pitch bend
        serialThru();
        serialThru();
      break;
      // Two-byte messages - Not implemented
      case 0xc0:  // Program Change
      case 0xd0:  // Channel aftertouch
        serialThru();
      break;
      case 0xf0: // Sysex and other similar messages
        switch (commandByte) {
          case 0xf0:  // Sysex
            // Serial.read() will return -1 if nothing is available.  So, this
            // code is going to block until the Sysex message is complete.  
            // That means that Sysex messages will probably cause a noticable blip
            // for the oscilator.
            while(serialThru() != 0xf7) {}
            break;
          case 0xf1:  // MIDI time code quarter frame
          case 0xf3: // Song select
            serialThru();
            break;
          case 0xf2: // Song Position
            serialThru();
            serialThru();
            break;
          default:
            // Remaining messages are all single-byte.
            break;
        }
      break;
    }
  }
}

void setup() {
  setupVoices();
  setupMidi();
  setupAnalogOut();
}

void sendVoices() {
  unsigned long nowClock = micros();
  byte sum = 0;
  for (int i = 0; i < kNumPeriods; ++i) {
    if (halfPeriod[i] == 0) {
      continue;
    }
    if (releaseTimeUs[i] <= nowClock) {
      halfPeriod[i] = 0;
      currentNote[i] = 0;
      releaseTimeUs[i] = kUlongMax;
      continue;
    }
    byte v = lastValue[i];
    sum += v * voiceLevel[i];
    if (nowClock >= nextTrigger[i]) {
      nextTrigger[i] += halfPeriod[i]; 
      if (!v) {
        lastValue[i] = 1;
      } else {
        lastValue[i] = 0;
      }
    }
  }
  setAnalogLevel(sum);
}

// This code does not handle integer overflow correctly
// so every 1.2 hours there will be a little pop.
void loop() {
  sendVoices();
  // MIDI is handled by IRC
}
