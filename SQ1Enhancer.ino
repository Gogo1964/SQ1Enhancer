/*
  SQ1Enhancer
  created by Karsten Gorkow
*/

int audioPin = A0; // input pin for the audio
int thresholdPin = A1; // input pin for the threshold pot
int swingPotPin = A2; // input pin for the swing pot
int ledPin = LED_BUILTIN;      // select the pin for the LED
int syncOutPin = 2;  
int settingsBtnInPin = 3; // input for button to switch sync resolution

int audioValue = 0;  // variable to store the value coming from the audio in
int thresholdValue = 0;
int swingValue = 0;
bool firstPulseSeen = false;
unsigned long lastPulseTS;
unsigned long thisPulseTS;
int minDeltaPulseTS = 150;
bool pulseRateSet = false;
float pulseRate;
unsigned long nextPulseTS = 0;
int outputPulseTime = 15;
bool stopOnNoPulse = true;
int stopDelay = 3000;

int state = HIGH;      // the current toggled state of the subDivBtn
int reading;           // the current reading from the settingsBtnInPin pin
int previous = LOW;    // the previous reading from the settingsBtnInPin pin
unsigned long time = 0;         // the last time the output settingsBtnInPin was toggled
int debounce = 200;   // the debounce time, increase if the output flickers

float fac[] = {4.0f, 3.0f, 2.0f, 1.0f+1.0f/3.0f, 1.0f, 0.5f, 1.0f/3.0f, 0.25f, 1.0f/6.0f, 0.125f};
int skips[] =   {3,    2,    1,    3,              0,    0,    0,         0,     0,         0     };
int subs[] =    {0,    0,    0,    2,              0,    1,    2,         3,     5,         7     };
int setIndexDef = 5;
int setIndex = -1;
int skipCounter;
int subCounter;

void pulseOutput();
void checkPulse();

void setup() {
  // declare the ledPin as an OUTPUT:
  pinMode(ledPin, OUTPUT);
  pinMode(syncOutPin, OUTPUT);
  pinMode(settingsBtnInPin, INPUT);
  Serial.begin(9600);
}

bool setSetIndex() {
  bool oldState = state;
  bool changeForced = setIndex < 0;
  if (changeForced) {
    setIndex = setIndexDef;
  }
  reading = digitalRead(settingsBtnInPin);
  // if the input just went from LOW and HIGH and we've waited long enough
  // to ignore any noise on the circuit, toggle the output pin and remember
  // the time
  if (reading == HIGH && previous == LOW && millis() - time > debounce) {
    if (state == HIGH)
      state = LOW;
    else
      state = HIGH;

    setIndex++;
    if (setIndex >= sizeof(fac) / sizeof(fac[0])) {
      setIndex = 0;
    }
    time = millis();    
  }

  previous = reading;
  return oldState != state || changeForced;
}

float currentFac() {
  return fac[setIndex];
}

int currentSkips() {
  return skips[setIndex];
}

int currentSubs() {
  return subs[setIndex];
}

void loop() {
  audioValue = analogRead(audioPin);
  thresholdValue = analogRead(thresholdPin);
  bool subDivsChanged = setSetIndex();
  checkPulse();
  if (pulseRateSet && millis() >= nextPulseTS) {
    if (currentSubs() > subCounter) {
      pulseOutput();
      subCounter++;
      nextPulseTS = lastPulseTS + (unsigned long)(((subCounter + 1) * currentFac() * pulseRate) + .5f);
    }
  }
}

void checkPulse() {
  if (audioValue > thresholdValue) {
    thisPulseTS = millis();
    if (!firstPulseSeen) {
      firstPulseSeen = true;
      lastPulseTS = thisPulseTS;
      skipCounter = 0;
    }
    else if ((thisPulseTS - lastPulseTS) > minDeltaPulseTS) {
      pulseRate = thisPulseTS - lastPulseTS;
      lastPulseTS = thisPulseTS;
      pulseRateSet = true;
      if (skipCounter >= currentSkips()) {
        subCounter = 0;
        skipCounter = 0;
        pulseOutput();
        nextPulseTS = thisPulseTS + ((pulseRate * currentFac()) + 0.5f);
      }
      else {
        ++skipCounter;
      }
    }
  }
  else if (firstPulseSeen && stopOnNoPulse && (millis() - lastPulseTS) > stopDelay) {
    firstPulseSeen = false;
    pulseRateSet = false;
  }
}

void pulseOutput() {
  digitalWrite(ledPin, HIGH);
  digitalWrite(syncOutPin, HIGH);
  delay(outputPulseTime);
  digitalWrite(ledPin, LOW);
  digitalWrite(syncOutPin, LOW);
  Serial.print("bpm=\t");
  Serial.print(60000 / pulseRate);
  Serial.print("\tsubdiv=\t");
  Serial.println(currentFac());
}
