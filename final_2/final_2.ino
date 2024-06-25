#include <Servo.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);
Servo thruster;

// Pin configurations 
const int encoderPinCLK = 2;
const int encoderPinDT = 3;
const int buttonPin = 4;
const int startButton = 6;
const int stopButton = 7;

// Settings configurations
int hours[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int minutes[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
int speed[] = {10, 20, 30, 40, 50, 0, 70, 20, 90, 10, 50};
bool direction[] = {1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1};

// Menu Config
int menuIndex = 0;
const int menuItems = 6;
const char* menuOptions[] = {"<-","Hours:","Mns:","Speed:","Dir:", "->"};

enum MenuState {SEGMENT_MENU, MAIN_MENU, SETTINGS_MENU, START_STOP};
MenuState currentMenuState = SEGMENT_MENU; 

// Variables
volatile int lastCLKState;
volatile long lastValue = 0;
volatile bool encoderRotated = false;
int speedValue = 100;
int pwm= 1500;
int lastPwm =1500;
bool lastDirection=0;

// pointers
int segments = 1;
int slide = 1 ;
int cursor = 0;

bool startFlag = false;
bool pauseFlag = false;

// timer variables
unsigned long currentTime;
unsigned long startTime;
unsigned long elapsedTime;
unsigned long pauseTime=0;
unsigned long remainingTime;
unsigned long countdownDuration=0;

byte arrow[] = {
  B00000,
  B00100,
  B00110,
  B11111,
  B00110,
  B00100,
  B00000,
  B00000
};
byte revArrow[] = {
  B00000,
  B00100,
  B01100,
  B11111,
  B01100,
  B00100,
  B00000,
  B00000
};
byte pause[] = {
  B00000,
  B11011,
  B11011,
  B11011,
  B11011,
  B11011,
  B11011,
  B00000
};


void setup(){
  Serial.begin(9600);
  thruster.attach(9);
  lcd.init();
  
  lcd.createChar(0, arrow);
  lcd.createChar(1, revArrow);
  lcd.createChar(2, pause);

  pinMode(encoderPinCLK, INPUT_PULLUP);
  pinMode(encoderPinDT, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(startButton, INPUT_PULLUP);
  pinMode(stopButton, INPUT_PULLUP);

  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("   Thruster-Timer   ");
  lcd.setCursor(0, 2);
  lcd.print("  Rotate Knobe to   ");
  lcd.setCursor(0, 3);
  lcd.print("    select Menu     ");
  
  thruster.writeMicroseconds(1500);
  delay(5000);
  
  attachInterrupt(digitalPinToInterrupt(encoderPinCLK), encoderInterrupt, CHANGE);

  lastCLKState = digitalRead(encoderPinCLK);
  
}
void loop() {
  currentTime = millis();
  if(encoderRotated){
    handleEncoderRotation();
    encoderRotated = false;
  }else if(currentMenuState == SEGMENT_MENU && digitalRead(buttonPin) == LOW){
    handleMenuSelection();
    delay(250);
  }else if(currentMenuState == MAIN_MENU && digitalRead(buttonPin) == LOW){
    currentMenuState = SETTINGS_MENU;
    updateMenu();
    delay(250);
  }else if(currentMenuState == SETTINGS_MENU && digitalRead(buttonPin) == LOW && menuIndex!=0 && menuIndex!=5){
    currentMenuState = MAIN_MENU;
    updateMenu();
  }else if(currentMenuState == SETTINGS_MENU && menuIndex==5 && digitalRead(buttonPin) == LOW){
    if(slide<segments){
      Serial.println("NEXT PAGE!");  
      slide++;
      currentMenuState = MAIN_MENU;
      updateMenu();
    }else{
      Serial.println("segment count Reached!!");
      currentMenuState = START_STOP;
      updateMenu();
    }
  }else if(currentMenuState == SETTINGS_MENU && menuIndex==0 && digitalRead(buttonPin) == LOW){
    Serial.println("back button pressed!!");
    if(slide>1){
      Serial.println("BACK PAGE!");  
      slide--;
      currentMenuState = MAIN_MENU;
      updateMenu();
      
    }else{
      Serial.println("segment count Reached!!");
      currentMenuState = SEGMENT_MENU;
      updateMenu();
    }
  }else if(currentMenuState == START_STOP && digitalRead(buttonPin) == LOW && !startFlag){
    currentMenuState = SEGMENT_MENU;
    cursor = 0;
    pauseFlag = false; // resetting time
    updateMenu();
  }else if(currentMenuState == START_STOP && digitalRead(startButton) == LOW && !startFlag){
    
    startFlag = true;

    if(pauseFlag){
      Serial.println("Timer Resumed!");
      startTime += (millis() - pauseTime);
      pauseFlag = false;
      pwm = lastPwm;
      lcd.setCursor(17, 1);
      lcd.print(" ");
    }else{
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      totalTime();
      startTime = millis();
    }
    

  }else if(currentMenuState == START_STOP && digitalRead(stopButton) == LOW && startFlag){
    pauseFlag = true;
    pauseTime = millis();
    startFlag = false;
    Serial.println("Timer Paused!");
    lcd.setCursor(17, 1);
    lcd.write(2);
    lastPwm = pwm;
    pwm = 1500;
    delay(250);
  }

// timer settings
  if(startFlag){
    elapsedTime = currentTime-startTime;
    remainingTime = countdownDuration-elapsedTime;
    
    if(currentTime-startTime>=countdownDuration){
      Serial.println("TIME IS UP!");
      if(cursor<segments){
        cursor = cursor+1;
        totalTime();
      }else{
        cursor=0;
        pwm = 1500;
        lcd.setCursor(1, 3);
        lcd.print("-- TIME IS UP!! -- ");
        startFlag=false;
      }
        
      startTime = currentTime;
    }else{
      
      int hoursLeft = remainingTime/3600000UL;
      int minutesLeft = (remainingTime%3600000UL)/60000UL;
      int secondsLeft = (remainingTime%60000UL)/1000UL;

      lcd.setCursor(2,1);
      lcd.print("Time: ");
      lcd.print(hoursLeft);
      lcd.print(":");
      lcd.print(minutesLeft);
      lcd.print(":");
      lcd.print(secondsLeft);
      lcd.print(" ");

      lcd.setCursor(15,0);
      lcd.print(cursor);
      lcd.print("/");
      lcd.print(segments);
      lcd.print(" ");

      lcd.setCursor(0,2);
      lcd.print("Speed:");
      lcd.print(speed[cursor]);
      lcd.print("% ");
      lcd.setCursor(11,2);
      lcd.print("Dir: ");
      if(direction[cursor]==0){
        lcd.print("CW  ");
      }else{
        lcd.print("CCW ");
      }
    } 
  }
// loop

Serial.print("PWM: ");
Serial.println(pwm);
// writing pwm
thruster.writeMicroseconds(pwm);
delay(200);


}

void totalTime(){
  countdownDuration=0;
  countdownDuration = hours[cursor] * 3600000UL + minutes[cursor] *60000UL;

  // PWM
  if(direction[cursor]==0){
    Serial.print("CW  ");
    pwm = map(speed[cursor], 0, 100, 1500,2000);
  }else{
    Serial.print("CCW ");
    pwm = map(speed[cursor], 0, 100, 1500,1000);

  }

  if(direction[cursor-1] !=direction[cursor]){
    thruster.writeMicroseconds(1500);
    Serial.println("Safe stopping--!");
    delay(500);
  }
  

}

void encoderInterrupt() {
  // encoder interrupt service routine
  int CLKState = digitalRead(encoderPinCLK);
  int DTState = digitalRead(encoderPinDT);

  if (CLKState != lastCLKState) {
    if (DTState != CLKState){
      lastValue++;
    } else {
      lastValue--;
    }
    lastCLKState = CLKState;
    encoderRotated = true; 
  }
}

void handleEncoderRotation() {
  int encoded = lastValue;
  int dir = encoded / abs(encoded); 

  if (currentMenuState == SEGMENT_MENU){
    updateMenu();
    segments = normalizeValue(segments + dir, 1, 10);
    lcd.setCursor(10, 2);
    lcd.print("     "); 
    lcd.setCursor(10, 2);
    lcd.print(segments);
    lastValue = 0;
  }else if(currentMenuState == MAIN_MENU){
    menuIndex = (menuIndex + dir + menuItems) % menuItems;
    lastValue = 0;
    updateMenu();
  }else if(currentMenuState == SETTINGS_MENU){
    if(menuIndex == 1){
      hours[slide] = normalizeValue(hours[slide] + dir, 0, 2);
      lcd.setCursor(8, 1);
      lcd.print("  "); 
      lcd.setCursor(8, 1);
      lcd.print(hours[slide]);
      lastValue = 0;
    }else if(menuIndex == 2){
      minutes[slide] = normalizeValue(minutes[slide] + dir, 0, 60);
      lcd.setCursor(16, 1);
      lcd.print("     ");
      lcd.setCursor(16, 1);
      lcd.print(minutes[slide]);
      lastValue = 0;

    }else if(menuIndex == 3){
      speed[slide] = normalizeValue(speed[slide] + dir*5, 0, 100);
      lcd.setCursor(8, 2);
      lcd.print("   "); 
      lcd.setCursor(8, 2);
      lcd.print(speed[slide]);
      lcd.print("% ");
      lastValue = 0;

    }else if(menuIndex == 4){
      direction[slide] = normalizeValue(direction[slide] + dir, 0, 1);
      lcd.setCursor(16, 2);
      lcd.print("     "); 
      lcd.setCursor(16, 2);
      if(direction[slide]==0){
        lcd.print("CW ");
      }else{
        lcd.print("CCW");
      }
        
      lastValue = 0;
    }
    else if(menuIndex == 5){
      lcd.setCursor(15, 0);
      lcd.print("");

    }
  }
}

void handleMenuSelection() {
  if (currentMenuState == SEGMENT_MENU) {
    currentMenuState = MAIN_MENU;
    updateMenu();
  }
}

int normalizeValue(long val, int minVal, int maxVal) {
  if (val < minVal) {
    return minVal;
  } else if (val > maxVal) {
    return maxVal;
  } else {
    return val;
  }
}

void updateMenu() {
  
  if(currentMenuState == SEGMENT_MENU){
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("Number of");
    lcd.setCursor(6, 1);
    lcd.print("Segments ");
    lcd.setCursor(10, 2);
    lcd.print(segments);

  }else if (currentMenuState == MAIN_MENU) {
    lcd.clear();

    lcd.setCursor(1,1);
    lcd.print(menuOptions[1]); //H
    lcd.setCursor(12, 1);
    lcd.print(menuOptions[2]); //M
    lcd.setCursor(1, 2);
    lcd.print(menuOptions[3]); //Speed
    lcd.setCursor(12, 2);
    lcd.print(menuOptions[4]); //Dir
    lcd.setCursor(0, 0);
    lcd.print(menuOptions[0]); //<-
    lcd.setCursor(18, 0);
    lcd.print(menuOptions[5]); //->

    lcd.setCursor(9, 0);
    lcd.print(slide);
    lcd.print("/");
    lcd.print(segments);
    if(menuIndex ==0){
      lcd.setCursor(1,0);
      lcd.write(1);
    }else if(menuIndex ==1){
      lcd.setCursor(0,1);
      lcd.write(0);
    }else if(menuIndex==2){
      lcd.setCursor(11, 1);
      lcd.write(0);
    }else if(menuIndex==3){
      lcd.setCursor(0, 2);
      lcd.write(0);
    }else if(menuIndex==4){
      lcd.setCursor(11, 2);
      lcd.write(0);
    }else if(menuIndex==5){
      lcd.setCursor(18,0);
      lcd.write(0);
    }else{
      lcd.print(" ");
    }

    lcd.setCursor(8, 1);
    lcd.print(hours[slide]);
    lcd.setCursor(17, 1);
    lcd.print(minutes[slide]);
    lcd.setCursor(8, 2);
    lcd.print(speed[slide]);
    lcd.setCursor(17, 2);
    if(direction[slide]==0){
        lcd.print("CW ");
      }else{
        lcd.print("CCW");
      }

  } else if (currentMenuState == SETTINGS_MENU) {
    if(menuIndex == 1){
      lcd.setCursor(8, 1);
      lcd.print(hours[slide]);
    }else if(menuIndex == 2){
      lcd.setCursor(17, 1);
      lcd.print(minutes[slide]);
    }else if(menuIndex == 3){
      lcd.setCursor(8, 2);
      lcd.print(speed[slide]);
    }else if(menuIndex == 4){
      lcd.setCursor(17, 2);
      if(direction[slide]==0){
        lcd.print("CW ");
      }else{
        lcd.print("CCW");
      }
    }else if(menuIndex == 5){
      lcd.setCursor(15, 0);
      lcd.print("Next ");
      
    }else if(menuIndex == 0){
      lcd.setCursor(0, 0);
      lcd.print("Back ");
    }
  }else if(currentMenuState == START_STOP){
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Start/Stop");
    lcd.setCursor(15,0);
    lcd.print(cursor);
    lcd.print("/");
    lcd.print(segments);
    lcd.print(" ");
    lcd.setCursor(0,2);
    lcd.print("Speed:");
    lcd.print(" ");
    lcd.print("% ");
    lcd.setCursor(11,2);
    lcd.print("Dir: ");
  }
 }

