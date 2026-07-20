// SENDER — watches its ultrasonic sensor and pulses pin 10 HIGH
// whenever something passes within range. Wire pin 10 -> receiver's
// pin 10, plus a shared GND between the two boards.
// Also blinks heartbeatPin so the receiver can confirm the two boards
// are actually wired together, not just floating.

const int trigPin = 3;
const int echoPin = 5;
const int signalPin = 10;
const int heartbeatPin = 9; // wire this to the receiver's pin 9

long duration;
int distanceCm;

// tetting the length like MITES values
const int tooClose = 30;
const int tooFar   = 240;

bool personAtGate = false;
unsigned long lastTriggerTime = 0;
const unsigned long triggerCooldown = 1500; // ms — tune to your runners' stride time

// troubleshooting heartbeat: print a scan reading every 5 sec regardless of detections
unsigned long lastScanPrint = 0;
const unsigned long scanPrintInterval = 5000;

// link heartbeat: toggle every second so the receiver can tell the wire is alive
bool heartbeatState = false;
unsigned long lastHeartbeatToggle = 0;
const unsigned long heartbeatInterval = 1000;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(signalPin, OUTPUT);
  digitalWrite(signalPin, LOW);
  pinMode(heartbeatPin, OUTPUT);
  digitalWrite(heartbeatPin, LOW);
  Serial.begin(9600); // USB debug monitor for this board
}

void loop() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 30000);
  distanceCm = duration * 0.034 / 2;

  // link heartbeat: flip the pin every second so it never sits still
  if (millis() - lastHeartbeatToggle >= heartbeatInterval) {
    lastHeartbeatToggle = millis();
    heartbeatState = !heartbeatState;
    digitalWrite(heartbeatPin, heartbeatState);
  }

  // troubleshooting heartbeat: prove the sensor + loop are alive every 5 sec
  if (millis() - lastScanPrint >= scanPrintInterval) {
    lastScanPrint = millis();
    Serial.print("Scanning... distance: ");
    Serial.print(distanceCm);
    Serial.println(" cm");
  }

  // ignore anything too far / no echo entirely, don't count it
  if (distanceCm != 0 && distanceCm <= tooFar) {
    if (distanceCm < tooClose) {
      if (!personAtGate) {
        unsigned long now = millis();

        if (now - lastTriggerTime >= triggerCooldown) {
          personAtGate = true; // edge trigger, only fires once per pass
          lastTriggerTime = now;

          digitalWrite(signalPin, HIGH);
          Serial.println("Object detected -> pin 10 HIGH");
          delay(200); // hold the pulse long enough for the receiver to catch it
          digitalWrite(signalPin, LOW);
        }
      }
    } else {
      personAtGate = false; // re-arm for the next pass
    }
  }

  delay(50);
}
