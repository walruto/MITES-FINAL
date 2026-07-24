// SENDER (Bluetooth MASTER) — watches its ultrasonic sensor and sends
// the signal over Bluetooth (HC-05 wired to Serial1) instead of a wire
// on pin 10. Wire the HC-05: VCC->5V, GND->GND, TX->Nano RX0 (D0),
// RX->Nano TX1 (D1) (use a voltage divider on this line — HC-05 RX is
// 3.3V logic). This HC-05 must be AT-command-configured as MASTER and
// bound to the receiver's HC-05 MAC address before this sketch runs.
 
const int trigPin = 3;
const int echoPin = 5;

long duration;
int distanceCm;

// tetting the length like MITES values
const int tooClose = 5;
const int tooFar   = 75;

bool personAtGate = false;
unsigned long lastTriggerTime = 0;
const unsigned long triggerCooldown = 1500; // ms — tune to your runners' stride time

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  Serial1.begin(9600); // HC-05 (Bluetooth master), data mode baud
  Serial.begin(9600);  // USB debug monitor for this board
}

void loop() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 30000);
  distanceCm = duration * 0.034 / 2;

  // // link heartbeat: send 'H' every second so it never goes quiet
  // if (millis() - lastHeartbeatSend >= heartbeatInterval) {
  //   lastHeartbeatSend = millis();
  //   Serial1.write('H');
  // }

 

  // ignore anything too far / no echo entirely, don't count it
  if (distanceCm >= tooClose && distanceCm <= tooFar) {

    if (!personAtGate) {
      unsigned long now = millis();
      
      if (now - lastTriggerTime >= triggerCooldown) {
        personAtGate = true; // edge trigger, only fires once per pass
        lastTriggerTime = now;

        Serial1.write('1'); // object detected -> over Bluetooth instead of pin 10 HIGH
        Serial.println("Object detected -> sent '1' over Bluetooth");
        delay(200); // same pacing as the old pulse width
        Serial1.write('0'); // pin 10 LOW equivalent
      }
    } 
  }
  else {
      personAtGate = false; // re-arm for the next pass
  }

  delay(60); // HC-SR04 needs ~60ms between pings or echoes from the last
             // reading bleed into the next one and cause false triggers
}
