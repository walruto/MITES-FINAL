// RECEIVER (Bluetooth SLAVE) — Arduino Nano Every
// HC-05 wiring: VCC->5V, GND->GND, HC-05 TX->Nano Every RX0 (Pin 0),
// HC-05 RX->Nano Every TX1 (Pin 1) via a voltage divider.

const int trigPin = 3;
const int echoPin = 5;

long duration;
int distanceCm;

const int tooClose = 5;
const int tooFar   = 75;

// Lap timer state
bool personAtGate = false;
bool waitingForFinish = false;
int lapNumber = 0;
unsigned long lapStartTime = 0;

// Debounce timing
unsigned long lastTriggerTime = 0;
const unsigned long triggerCooldown = 1500; // ms

unsigned long lastStartSignalTime = 0;
const unsigned long startSignalCooldown = 1500; // ms

unsigned long lastHeartbeatChangeTime = 0;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  Serial1.begin(9600); // Bluetooth hardware serial on Nano Every (Pins 0 & 1)
  Serial.begin(9600);  // USB debug monitor
}

void loop() {
  // -------------------------------------------------------------------
  // FIX FOR ISSUE 2: Process start signals immediately as single-shot
  // events inside the Bluetooth read loop instead of using latched state
  // -------------------------------------------------------------------
  while (Serial1.available()) {
    char c = Serial1.read();
    
    if (c == '1') {
      unsigned long now = millis();
      if (now - lastStartSignalTime >= startSignalCooldown) {
        lastStartSignalTime = now;

        if (waitingForFinish) {
          Serial.println("Already in a lap --- cannot start a new one yet.");
        } else {
          lapStartTime = now;
          waitingForFinish = true;
          Serial.println("Lap started, waiting for finish...");
        }
      }
    } else if (c == 'H') {
      lastHeartbeatChangeTime = millis();
    }
  }

  // Measure local finish-line ultrasonic distance
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // ISSUE 3 UNHANDLED: Unmodified pulseIn and distance calculation
  duration = pulseIn(echoPin, HIGH, 30000);
  distanceCm = duration * 0.034 / 2;

  // -------------------------------------------------------------------
  // FIX FOR ISSUE 1: Proper gate entry & exit re-arm bounds
  // -------------------------------------------------------------------
  if (distanceCm >= tooClose && distanceCm <= tooFar) {
    if (!personAtGate) {
      unsigned long now = millis();

      if (now - lastTriggerTime >= triggerCooldown) {
        personAtGate = true; // Edge trigger, only fires once per pass
        lastTriggerTime = now;

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
    }
  } else {
    // Re-arm ONLY when the person clears the 5cm–75cm gate completely
    personAtGate = false;
  }

  delay(60); // HC-SR04 echo decay delay
}
