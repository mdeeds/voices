// The UNO and Nano only have 2048 bytes of RAM.  This needs to be used for
// local variables and the stack.  At complile time we don't know how much
// RAM will be required for the stack, so this parameter may need tuning.
#define kBufferSize 1800

volatile int nextSample = 0;
volatile byte sampleBuffer[kBufferSize];

//   rate    SampleRate
// 0 or 1       615 kHz
//      2       308 kHz
//      3       154 kHz
//      4      76.9 kHz
//      5      38.5 kHz
//      6      19.2 kHz
//      7      9.62 kHz  # Arduino Default
inline void setPrescaler(byte rate) {
  rate &= 0x3;  // Clean up input variable just in case.

  cli();  // Disable interrupts while we change ADC settings
  ADCSRA &= 0xf6;  // Clear previous rate
  ADCSRA |= rate;
  sei();
}

void setup() {
  ADCSRA = 0;
  ADCSRB = 0;
  ADMUX |= (/*pin=*/0 & 0x07); // A0
  ADMUX |= (1 << REFS0); // TODO: What reference voltage do we want?
  ADMUX |= (1 << ADLAR); // Make ADCH have the top 8 bits.  We will drop the lower 2.
  setPrescaler(5);
  ADCSRA |= (1 << ADATE);  // auto trigger
  ADCSRA |= (1 << ADIE);   // enable interrupts
  ADCSRA |= (1 << ADEN);   // Enable ADC
  ADCSRA |= (1 << ADSC);   // Start ADC
}

ISR(ADC_vect)
{
  byte x = ADCH;
  int i = nextSample;
  sampleBuffer[i] = x;
  ++i;
  if (nextSample >= kBufferSize) {
    nextSample = 0;
  }
  nextSample = i;
}

void loop() {
  // put your main code here, to run repeatedly:

}
