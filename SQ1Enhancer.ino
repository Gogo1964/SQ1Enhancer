/*
  SQ1Enhancer
  created by Karsten Gorkow
*/

int audioPin = A0; // input pin for the audio
int thresholdPin = A1; // input pin for the threshold pot
int swingPotPin = A2; // input pin for the swing pot
int ledPin = LED_BUILTIN;      // select the pin for the LED
int syncOutPin = 2;  
int skipsBtnInPin = 3; // input for button to switch skips
int subsBtnInPin = 4; // input for button to switch subs

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

struct ToggleButton {
  int inPin;
  int state;      // the current toggled state of the button
  int reading;           // the current reading from the button's pin
  int previous;    // the previous reading from the button's pin
  unsigned long time = 0;  // the last time the output was toggled
};

const int buttonDebounceTime = 200;

ToggleButton skipsBtn;
ToggleButton subsBtn;

const int maxSkips = 7;
const int maxSubs = 7;

int skips = 0;
int subs = 1;
int skipCounter;
int subCounter;

void setupToggleButton(int pin, ToggleButton* pbtn) {
  pbtn->inPin = pin;
  pbtn->state = HIGH;
  pbtn->previous = LOW;
  pbtn->time = 0;
  pinMode(pbtn->inPin, INPUT);
}

void setup() {
  // declare the ledPin as an OUTPUT:
  pinMode(ledPin, OUTPUT);
  pinMode(syncOutPin, OUTPUT);
  setupToggleButton(3, &skipsBtn);
  setupToggleButton(4, &subsBtn);
  Serial.begin(9600);
}

void printInfo(char* head) {
  Serial.print(head);
  Serial.print("\tbpm=\t");
  Serial.print(60000 / pulseRate);
  Serial.print("\tskips=\t");
  Serial.print(skips);
  Serial.print("\tcnt=\t");
  Serial.print(skipCounter);
  Serial.print("\tsubs=\t");
  Serial.print(subs);
  Serial.print("\tcnt=\t");
  Serial.print(subCounter);
  Serial.print("\tfac=\t");
  Serial.println(currentFac());
}

bool setStepVal(ToggleButton* pbtn, int* val, int minVal, int maxVal) {
  if (*val < minVal) *val = minVal;
  if (*val > maxVal) *val = maxVal;
  bool oldState = pbtn->state;
  pbtn->reading = digitalRead(pbtn->inPin);
  // if the input just went from LOW and HIGH and we've waited long enough
  // to ignore any noise on the circuit, toggle the output pin and remember
  // the time
  if (pbtn->reading == HIGH && pbtn->previous == LOW && millis() - pbtn->time > buttonDebounceTime) {
    if (pbtn->state == HIGH)
      pbtn->state = LOW;
    else
      pbtn->state = HIGH;

    (*val)++;
    if (*val > maxVal) {
      *val = minVal;
    }
    pbtn->time = millis();    
  }

  pbtn->previous = pbtn->reading;
  return oldState != pbtn->state;
}

float currentFac() {
  return ((float)(skips + 1)) / ((float) (subs + 1));
}

void pulseOutput() {
  digitalWrite(ledPin, HIGH);
  digitalWrite(syncOutPin, HIGH);
  delay(outputPulseTime);
  digitalWrite(ledPin, LOW);
  digitalWrite(syncOutPin, LOW);
}

void checkPulse() {
  if (audioValue > thresholdValue) {
    thisPulseTS = millis();
    if (!firstPulseSeen) {
      firstPulseSeen = true;
      lastPulseTS = thisPulseTS;
      skipCounter = skips;
    }
    else if ((thisPulseTS - lastPulseTS) > minDeltaPulseTS) {
      pulseRate = thisPulseTS - lastPulseTS;
      lastPulseTS = thisPulseTS;
      pulseRateSet = true;
      if (skipCounter == skips) {
        subCounter = 0;
        skipCounter = 0;
        pulseOutput();
        nextPulseTS = thisPulseTS + ((pulseRate * currentFac()) + 0.5f);
      }
      else {
        skipCounter++;
      }
      printInfo("P");
    }
  }
  else if (firstPulseSeen && stopOnNoPulse && (millis() - lastPulseTS) > stopDelay) {
    firstPulseSeen = false;
    pulseRateSet = false;
  }
}

void loop() {
  audioValue = analogRead(audioPin);
  thresholdValue = analogRead(thresholdPin);
  setStepVal(&skipsBtn, &skips, 0, maxSkips);
  setStepVal(&subsBtn, &subs, 0, maxSubs);
  checkPulse();
  if (pulseRateSet && millis() >= nextPulseTS && subs != subCounter) {
    pulseOutput();
    subCounter++;
    nextPulseTS = lastPulseTS + (unsigned long)(((subCounter + 1) * currentFac() * pulseRate) + .5f);
    printInfo("S");
  }
}
