/*
  SQ1Enhancer
  created by Karsten Gorkow
*/

int audioPin = A0;    // select the input pin for the audio
int thresholdPin = A1;    // select the input pin for the potentiometer
int ledPin = LED_BUILTIN;      // select the pin for the LED
int syncOutPin = 2;  
int subDivBtnInPin = 3; // input for button to switch sync resolution
int audioValue = 0;  // variable to store the value coming from the audio in
int thresholdValue = 0;
bool firstPulseSeen = false;
unsigned long lastPulseTS;
unsigned long thisPulseTS;
unsigned long expectedNextPulseTS; 
int allowedPulseDelay = 30;
int minDeltaPulseTS = 150;
bool pulseRateSet = false;
float pulseRate;
unsigned long nextSubDivTS = 0;
int outputPulseTime = 15;
bool stopOnNoPulse = true;
int stopDelay = 3000;
int skipCounter;

int state = HIGH;      // the current toggled state of the subDivBtn
int reading;           // the current reading from the subDivBtnInPin pin
int previous = LOW;    // the previous reading from the subDivBtnInPin pin
unsigned long time = 0;         // the last time the output subDivBtnInPin was toggled
int debounce = 200;   // the debounce time, increase if the output flickers

float subDivSettings[] = {4.0f, 3.0f, 2.0f, 1.0f, 0.5f, 1.0f/3.0f, 0.25f, 1.0f/6.0f, 0.125f};
int eighthsSubDivSettingIndex = 4;
int subDivSettingsIndex = -1;

void pulseOutput();
bool setSubDivs();
int checkPulseRate();

void setup() {
  // declare the ledPin as an OUTPUT:
  pinMode(ledPin, OUTPUT);
  pinMode(syncOutPin, OUTPUT);
  pinMode(subDivBtnInPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  audioValue = analogRead(audioPin);
  thresholdValue = analogRead(thresholdPin);
  bool subDivsChanged = setSubDivs();
  bool onPulse = checkPulseRate() == 2;
  if (onPulse) {
    nextSubDivTS = lastPulseTS;
  }

  unsigned long now = millis();
  if (pulseRateSet && (onPulse || (now >= nextSubDivTS && now <= expectedNextPulseTS - allowedPulseDelay))) {
    pulseOutput(); // output subDiv
    nextSubDivTS += (unsigned long)((currentSubDiv() * pulseRate) + .5f);
  }
}

bool setSubDivs() {
  bool oldState = state;
  bool changeForced = subDivSettingsIndex < 0;
  if (changeForced) {
    subDivSettingsIndex = eighthsSubDivSettingIndex;
  }
  reading = digitalRead(subDivBtnInPin);
  // if the input just went from LOW and HIGH and we've waited long enough
  // to ignore any noise on the circuit, toggle the output pin and remember
  // the time
  if (reading == HIGH && previous == LOW && millis() - time > debounce) {
    if (state == HIGH)
      state = LOW;
    else
      state = HIGH;

    subDivSettingsIndex++;
    if (subDivSettingsIndex >= sizeof(subDivSettings) / sizeof(subDivSettings[0])) {
      subDivSettingsIndex = 0;
    }
    time = millis();    
  }

  previous = reading;
  return oldState != state || changeForced;
}

float currentSubDiv() {
  return subDivSettings[subDivSettingsIndex];
}

int checkPulseRate() {
  bool onPulse = false;
  if (audioValue > thresholdValue) {
    thisPulseTS = millis();
    if (!firstPulseSeen) {
      firstPulseSeen = true;
      lastPulseTS = thisPulseTS;
    }
    else if ((thisPulseTS - lastPulseTS) > minDeltaPulseTS) {
      pulseRate = thisPulseTS - lastPulseTS;
      lastPulseTS = thisPulseTS;
      expectedNextPulseTS = thisPulseTS + (pulseRate + 0.5f);
      pulseRateSet = true;
      if (skipCounter <= 0) {
        onPulse = true;
        skipCounter = (int)currentSubDiv();
      }
      --skipCounter;
    }
  }
  else if (firstPulseSeen && stopOnNoPulse && (millis() - lastPulseTS) > stopDelay) {
    firstPulseSeen = false;
    pulseRateSet = false;
  }
  return onPulse ? 2 : (pulseRateSet ? 1 : 0);
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
  Serial.println(currentSubDiv());
}
