#define photocellPin 0

int photocellReading;


void setup(void) {
  Serial.begin(9600);   
}
 
void loop(void) {
  photocellReading = analogRead(photocellPin);  
 
  Serial.print("Analog reading = ");
  Serial.println(photocellReading);
 
  delay(100);
}
