//pins get the wins
const int trigPin = 3;
const int echoPin = 5;
const int signalPin = 10; // reads the HIGH pulse from the sender board

// variables for duration and distance (I gues)
long duration;
int distanceCm;
// tetting the length like MITES values
const int tooClose = 30;
const int tooFar   = 240;

// lap timer state
bool personAtGate = false;  // stay false until the huzz approach the block
bool waitingForFinish = false; // true from the start signal until the object passes again
int lapNumber = 0;
unsigned long lapStartTime = 0;

// debounce / refractory period so a second foot doesn't retrigger the lap
unsigned long lastTriggerTime = 0;
const unsigned long triggerCooldown = 1500; // ms — tune to your runners' stride time

// debounce for the incoming start signal, so one pulse doesn't get read as several starts
unsigned long lastStartSignalTime = 0;
const unsigned long startSignalCooldown = 1500;

void setup() {
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);  // Sets the echoPin as an Input
  pinMode(signalPin, INPUT); // reads the sender board's HIGH pulse
  Serial.begin(9600);       // Starts the serial communication (USB debug)
}

void loop() {
  // Clear the trigPin by setting it LOW:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Trigger the sensor by setting the trigPin HIGH for 10 microseconds:
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the echoPin, returns the sound wave travel time in microseconds:
  duration = pulseIn(echoPin, HIGH, 30000); // 30 ms timeout so a missed echo doesn't stall

  // Calculate the distance in centimeters:
  distanceCm = duration * 0.034 / 2;

  // check for the start signal coming in from the sender board
  if (digitalRead(signalPin) == HIGH) {
    unsigned long now = millis();
    if (now - lastStartSignalTime >= startSignalCooldown) {
      lastStartSignalTime = now;

      // if a lap was already running, this signal closes it out before starting the next one
      if (waitingForFinish) {
        unsigned long lapTime = now - lapStartTime;
        lapNumber++;
        Serial.print("Lap ");
        Serial.print(lapNumber);
        Serial.print(" completed in ");
        Serial.print(lapTime / 1000.0);
        Serial.println(" sec");
      }

      lapStartTime = now;
      waitingForFinish = true;
      Serial.println("Lap started, waiting for finish...");
    }
  }

  // lap timer: ignore anything too far / no echo entirely, don't count it
  if (distanceCm != 0 && distanceCm <= tooFar) {
    if (distanceCm < tooClose) {
      // person is at the gate right now
      if (!personAtGate) {
        unsigned long now = millis();

        // debounce: ignore this "arrival" if it's too soon after the last one
        // (this is almost certainly just the second foot of the same pass)
        if (now - lastTriggerTime >= triggerCooldown) {
          personAtGate = true; // edge trigger, only fires once per pass
          lastTriggerTime = now;

          // this is the finish pass: only counts if a lap is actually in progress
          if (waitingForFinish) {
            unsigned long lapTime = now - lapStartTime;
            lapNumber++;
            Serial.print("Lap ");
            Serial.print(lapNumber);
            Serial.print(" completed in ");
            Serial.print(lapTime / 1000.0);
            Serial.println(" sec");
            waitingForFinish = false;
          }
        }
        // else: too soon after last trigger — treat as the same pass, ignore
      }
    } else {
      // person stepped away from the gate, re-arm for the next pass
      personAtGate = false;
    }
  }

  delay(50); // was 1000 — sample fast enough to actually catch a foot passing
}