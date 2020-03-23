/*
  Analog Input

  Demonstrates analog input by reading an analog sensor on analog pin 0 and
  turning on and off a light emitting diode(LED) connected to digital pin 13.
  The amount of time the LED will be on and off depends on the value obtained
  by analogRead().

  The circuit:
  - potentiometer
    center pin of the potentiometer to the analog input 0
    one side pin (either one) to ground
    the other side pin to +5V
  - LED
    anode (long leg) attached to digital output 13
    cathode (short leg) attached to ground

  - Note: because most Arduinos have a built-in LED attached to pin 13 on the
    board, the LED is optional.

  created by David Cuartielles
  modified 30 Aug 2011
  By Tom Igoe

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/AnalogInput
*/

int audioPin = A0;    // select the input pin for the audio
int thresholdPin = A1;    // select the input pin for the potentiometer
int ledPin = LED_BUILTIN;      // select the pin for the LED
int outputPin = 2;
int subDivInPin = 3;
int audioValue = 0;  // variable to store the value coming from the sensor
int thresholdValue = 0;
bool firstPulseSeen = false;
unsigned long tsLastPulse;
unsigned long tsThisPulse;
int minDeltaPulseTS = 150;
float pulseRate;
unsigned long nextSubDivTS = 0;
int subdivs = 4;
int subDivCounter = 1;
int outputPulseTime = 15;
int stopDelay = 3000;

int state = HIGH;      // the current state of the output pin
int reading;           // the current reading from the input pin
int previous = LOW;    // the previous reading from the input pin
unsigned long time = 0;         // the last time the output pin was toggled
int debounce = 200;   // the debounce time, increase if the output flickers
int subdivSettings[11] = {-4, -3, -2, 1, 2, 3, 4, 5, 6, 7, 8};
int subdivSettingsIndex = 6;

void pulseOutput();

void setup() {
  // declare the ledPin as an OUTPUT:
  pinMode(ledPin, OUTPUT);
  pinMode(outputPin, OUTPUT);
  pinMode(subDivInPin, INPUT);
  Serial.begin(9600);
}

void setSubDivs() {
  reading = digitalRead(subDivInPin);

  // if the input just went from LOW and HIGH and we've waited long enough
  // to ignore any noise on the circuit, toggle the output pin and remember
  // the time
  if (reading == HIGH && previous == LOW && millis() - time > debounce) {
    if (state == HIGH)
      state = LOW;
    else
      state = HIGH;

    subdivSettingsIndex++;
    if (subdivSettingsIndex >= 11) {
      subdivSettingsIndex = 0;
    }
    subdivs = subdivSettings[subdivSettingsIndex];
    time = millis();    
  }

  previous = reading;
}

void loop() {
  audioValue = analogRead(audioPin);
  thresholdValue = analogRead(thresholdPin);
  int sindex = subdivSettingsIndex; 
  setSubDivs();
  if (sindex != subdivSettingsIndex) {
    subDivCounter = 0;
  }
  if (audioValue > thresholdValue) {
    tsThisPulse = millis();
    if (!firstPulseSeen) {
      firstPulseSeen = true;
      tsLastPulse = tsThisPulse;
    }
    else if ((tsThisPulse - tsLastPulse) > minDeltaPulseTS) {
      if (subdivs > 0) {
        pulseRate = (float)(tsThisPulse - tsLastPulse) / (float)subdivs; // subdiv pulse rate
        Serial.print("bpm=\t");
        Serial.print(pulseRate);
        Serial.print("\t");
        Serial.println(60000 / (pulseRate * subdivs));
        nextSubDivTS = tsThisPulse + (int)(pulseRate + .5f);
        subDivCounter = 1;
        pulseOutput(); // output this pulse
      }
      else {
        pulseRate = tsThisPulse - tsLastPulse;
        Serial.print("bpm=\t");
        Serial.println((60000 * -subdivs) / pulseRate);
        if (subDivCounter == 0) {
          pulseOutput();
          subDivCounter = subdivs + 1;
        }
        else {
          ++subDivCounter;
        }
      }
      tsLastPulse = tsThisPulse;
    }
  }
  else if (firstPulseSeen && nextSubDivTS > 0 && subDivCounter < subdivs && millis() >= nextSubDivTS) {
    ++subDivCounter;
    nextSubDivTS = tsLastPulse + (int)((subDivCounter * pulseRate) + .5f);
    pulseOutput(); // output subdiv
  }
  else if (firstPulseSeen && (millis() - tsLastPulse) > stopDelay) {
    firstPulseSeen = false; 
    subDivCounter = 0;
  }
}

void pulseOutput() {
  digitalWrite(ledPin, HIGH);
  digitalWrite(outputPin, HIGH);
  delay(outputPulseTime);
  digitalWrite(ledPin, LOW);
  digitalWrite(outputPin, LOW);
}
