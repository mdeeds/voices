
byte samples[210];

int noOffset[210];
int jiggered[210];

// The computed amplitude components.
// 1, 3, 5, 7 are used for square waves.
// 0 holds the noise level
// 2, 4, 6 are reserved for future expansion.
int amplitudes[8];

void setup() {

}


void offsetPass() {
  int sum = 0;
  for (int i = 0; i < 210; ++i) {
    sum += samples[i];
  }
  int mean = sum / 210;
  for (int i = 0; i < 210; ++i) {
    noOffset[i] = samples[i] - mean;
  }
}

// We jigger the input signal so that the parts corresponding to the
// positive part of the signal are first, followed by the negative
// parts.  I.e. if the original input signal is a square wave with
// `periods` periods in the sample buffer, this transform will result
// in all high followed by all low.  If there is a phase shift, it
// will be phase shifted.  If it is out of tune a little, it will be a
// pulse width change.
//
// Thus, after jiggering, we just need to do our best to detect a
// square wave.
void jigger(int periods) {
  int waveLength = 210 / periods;

  int jiggerIndex = 0;
  for (int i = 0; i < 210; i += waveLength) {
    for (j = 0; j < waveLength; ++j) {
      jiggered[jiggerIndex] = noOffset[i + j];
    }
  }
}

// IMPORTANT!
// We don't need to jigger and unjigger if we just step through the sample
// in jigger order.

void unjigger() {
  // DON'T DO
}

void squarePass(int periods) {
  jigger(periods);

  int lowSum = 0;
  int highSum = 0;
  for (int i = 0; i < 105; ++i) {
    lowSum += jiggered[i];
    highSum += jiggered[i+105];
  }
  // At this point lowSum is the sum of the left half, and high sum is
  // the sum of the right half.  highSum - lowSum is maximized if the
  // square wave is exactly out of phase.

  // In the next loop, we advance the sliding windows one sample at a
  // time and find the point where highSum - lowSum is maximized
  int best = highSum - lowSum;
  int bestOffset = 105;
  // TODO: need to go all the way to 210. It needs to wrap around.
  for (int i = 0; i < 105; ++i) {
    lowSum += jiggered[i + 105] - jiggered[i];
    highSum += jiggered[i] - jiggered[i + 105];
    int newDiff = highSum - lowSum;
    if (newDiff > best) {
      best = newDiff;
      bestOffset = i;
    }
  }
  // Now best represents the amplitude of the square wave that best
  // describes the signal, and bestOffset is the offset of the leading
  // edge.
  int bestAmplitude = best / 105;
  amplitudes[periods] = bestAmplitude;

  // Now we subtract off the best matching amplitude.
  for (int i = 0; i < 210; ++i) {
    // TODO
  }

  unjigger();
}


void loop() {
  unsigned long startUs = micros();
  offsetPass();
  squarePass(1);
  squarePass(3);
  squarePass(5);
  squarePass(7);
  unsigned long duration = micros() - startUs;
  Serial.print("Duration (us): ");
  Serial.println(duration);
  // Sleep for a second to make sure that the Serial buffer has time
  // to clear.  This also makes sure we don't flood the serial monitor
  // with too much data.
  delay(/*ms=*/1000);
}
