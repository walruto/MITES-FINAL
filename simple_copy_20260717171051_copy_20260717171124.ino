// Define Pin Connections
const int trigPin = 3;
const int echoPin = 5;

// Variables for duration and distance
long duration;
int distanceCm;

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
  duration = pulseIn(echoPin, HIGH);
 
  // Calculate the distance in centimeters:
  distanceCm = duration * 0.034 / 2;
  
 
  // Print the distance on the Serial Monitor
  Serial.print("Distance: ");
  Serial.print(distanceCm);
  Serial.println(" cm");
 
  delay(100);
}
