const int kNumPeriods=6;

unsigned long halfPeriod[kNumPeriods];
unsigned long nextTrigger[kNumPeriods];
int pins[kNumPeriods];
bool lastValue[kNumPeriods];
unsigned char currentNote[kNumPeriods];

void setPulseFromNoteNumber(int voice, int noteNumber) {
  float h = (noteNumber - 69.0)/12.0;
  float freqHz = pow(2.0, h) * 440.0;
  float periodS = 1.0 / freqHz;
  unsigned long periodMicros = (unsigned long)(1000000 * periodS);
  halfPeriod[voice] = periodMicros / 2;
  nextTrigger[voice] = micros() + periodMicros / 2;
}

void setupVoices() {
  pinMode(LED_BUILTIN, OUTPUT);  // AKA D13
  digitalWrite(LED_BUILTIN, LOW);

  pins[0] = 12;
  pins[1] = 11;
  pins[2] = 10;
  pins[3] = 9;
  pins[4] = 8;
  pins[5] = 7;
  
  for (int i=0; i<kNumPeriods; ++i) {
    currentNote[i] = 0;
    halfPeriod[i] = 0;
    pinMode(pins[i], OUTPUT);
    lastValue[i] = 0;
    digitalWrite(pins[i], LOW);
  }

  setPulseFromNoteNumber(0, 60);  // C
  setPulseFromNoteNumber(1, 64);  // E
  setPulseFromNoteNumber(2, 67);  // G
  setPulseFromNoteNumber(3, 70);  // Bb
  setPulseFromNoteNumber(4, 52);  // E
  setPulseFromNoteNumber(5, 48);  // C
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

  TCCR1B = 0x09;
}

// writes a byte to the Fast PWM channels (9 and 10)
void AnalogWrite(byte level){
  OCR1AL = level;
  OCR1BL = level;
}

void handleNoteEvent(byte channel, byte note, byte velocity) {
  if (channel != 0) {
    // Only handle MIDI channel 1.
    return;
  }
  int match = -1;
  for (int i = 0; i<kNumPeriods; ++i) {
    if (currentNote[i] == note || currentNote[i] == 0) {
      match = i;
      digitalWrite(LED_BUILTIN, HIGH);
      break;
    }
  }
  if (match >= 0) {
    if (velocity > 0) {
      setPulseFromNoteNumber(match, note);
    } else {
      halfPeriod[match] = 0;
    }
  }
}

// Override the ISR for the serial interface
ISR(TIMER2_COMPA_vect) {
  while (Serial.available() >= 3) {
    // Note on and off messages are three bytes.
    byte commandByte = Serial.read();//read first byte
    byte command = commandByte & 0xf0;
    byte channel = commandByte & 0x0f;
    byte noteByte = 0;
    byte velocityByte = 0;
    switch (command) {
      case 0x80: // Note OFF
      case 0x90: // Note ON
        noteByte = Serial.read();
        velocityByte = Serial.read();
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
        Serial.read();
        Serial.read();
      break;
      // Two-byte messages - Not implemented
      case 0xc0:  // Program Change
      case 0xd0:  // Channel aftertouch
        Serial.read();
      break;
      case 0xf0: // Sysex and other similar messages
        switch (commandByte) {
          case 0xf0:  // Sysex
            // Serial.read() will return -1 if nothing is available.  So, this
            // code is going to block until the Sysex message is complete.  
            // That means that Sysex messages will probably cause a noticable blip
            // for the oscilator.
            while(Serial.read() != 0xf7) {}
            break;
          case 0xf1:  // MIDI time code quarter frame
          case 0xf3: // Song select
            Serial.read();
            break;
          case 0xf2: // Song Position
            Serial.read();
            Serial.read();
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
  for (int i=0; i<kNumPeriods; ++i) {
    if (halfPeriod[i] == 0) {
      continue;
    }
    if (nowClock >= nextTrigger[i]) {
      nextTrigger[i] += halfPeriod[i]; 
      if (!lastValue[i]) {
        digitalWrite(pins[i], HIGH);
        lastValue[i] = 1;
      } else {
        digitalWrite(pins[i], LOW);
        lastValue[i] = 0;
      }
    }
  }
}


// This code does not handle integer overflow correctly
// so every 1.2 hours there will be a little pop.
void loop() {
  sendVoices();
  // MIDI is handled by IRC
}
