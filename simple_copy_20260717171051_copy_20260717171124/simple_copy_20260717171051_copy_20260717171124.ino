//pins get the wins
const int trigPin = 3;
const int echoPin = 5;

// variables for duration and distance (I gues)
long duration;
int distanceCm;
// tetting the length like MITES values
const int tooClose = 70;
const int tooFar   = 200;

// lap timer state
bool personAtGate = false;  // stay false until the huzz approach the block
int lapNumber = 0;
unsigned long lapStartTime = 0;

void setup() {
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);  // Sets the echoPin as an Input
  Serial.begin(9600);       // Starts the serial communication
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
  
 
  //print if your tuff or not lowkey
  //Serial.print("distance: ");
  //Serial.print(distanceCm);
  //Serial.print(" cm  ->  ");
  if (distanceCm ==0) {
   //Serial.println("nothing going through");
  } else if (distanceCm < tooClose){
    //Serial.println("too close buddy");
  } else if (distanceCm>tooFar){
    //Serial.println("too far buddy");
  } else {
    //Serial.println("OK esdee");
  }

  // lap timer: ignore anything too far / no echo entirely, don't count it
  if (distanceCm != 0 && distanceCm <= tooFar) {
    if (distanceCm < tooClose) {
      // person is at the gate right now
      if (!personAtGate) {
        personAtGate = true; // edge trigger, only fires once per pass
        unsigned long now = millis();
        if (lapNumber == 0) {
          lapNumber = 1;
          lapStartTime = now;
          Serial.println("Lap 1 started!");
        } else {
          unsigned long lapTime = now - lapStartTime;
          Serial.print("Lap ");
          Serial.print(lapNumber);
          Serial.print(" finished in ");
          Serial.print(lapTime / 1000.0);
          Serial.println(" sec");

          lapNumber++;
          lapStartTime = now;
          Serial.print("Lap ");
          Serial.print(lapNumber);
          Serial.println(" started!");
        }
      }
    } else {
      // person stepped away from the gate, re-arm for the next pass
      personAtGate = false;
    }
  }

  delay(100);
}
