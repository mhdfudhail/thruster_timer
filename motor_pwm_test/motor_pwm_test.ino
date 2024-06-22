#include <Servo.h>

Servo myservo; 

int input = 0;
int pwm = 1500;

void setup() {
  Serial.begin(9600);
  myservo.attach(9);

  myservo.writeMicroseconds(1500);
  delay(8000);


}

void loop() {
  while(Serial.available()>0){
    input = Serial.parseInt();
    Serial.print(input);
    Serial.print("-->");

    if(input<1000 || input>2000){
      pwm = 1500; 
    }else{
      pwm = input;
    }
    Serial.print(pwm);
    Serial.print("\t");
  }
  Serial.print(pwm);
  Serial.print("\t");
  
  myservo.writeMicroseconds(pwm);
  delay(500);

}
