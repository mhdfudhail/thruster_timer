#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);

// Pin configurations 
const int encoderPinCLK = 2;
const int encoderPinDT = 3;
const int buttonPin = 4;
const int startButton = 6;
const int stopButton = 7;

// Settings configurations
int hours[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int minutes[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int speed[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool direction[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int cursor = 0;


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
int segments = 0;
int slide = 0 ;

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


void setup(){
  Serial.begin(9600);
  lcd.init();
  lcd.createChar(0, arrow);
  lcd.createChar(1, revArrow);

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
  
  delay(2000);

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
      Serial.println("NEXT PAGE!");
    }else{
      Serial.println("segment count Reached!!");
      for(int i=0; i<10; i++){
        Serial.print(minutes[i]);
        Serial.print("\t");
      }
      currentMenuState = START_STOP;
      updateMenu();
    }
  }else if(currentMenuState == SETTINGS_MENU && menuIndex==0 && digitalRead(buttonPin) == LOW){
    Serial.println("back button pressed!!");
    if(slide>0){
      Serial.println("BACK PAGE!");  
      slide--;
      currentMenuState = MAIN_MENU;
      updateMenu();
      Serial.println("BACK PAGE!");
    }else{
      Serial.println("segment count Reached!!");
      for(int i=0; i<10; i++){
        Serial.print(minutes[i]);
        Serial.print("\t");
      }
      currentMenuState = SEGMENT_MENU;
      updateMenu();
    }
  }else if(currentMenuState == START_STOP && digitalRead(buttonPin) == LOW && !startFlag){
    currentMenuState = SEGMENT_MENU;
    cursor = 0;
    updateMenu();
  }else if(currentMenuState == START_STOP && digitalRead(startButton) == LOW && !startFlag){
    // currentMenuState = SEGMENT_MENU;
    // updateMenu();
    startFlag = true;
    startTime = millis();
    
    if(pauseFlag){
      startTime=startTime;
    }else{
      totalTime();
      
      
    }
    

  }else if(currentMenuState == START_STOP && digitalRead(stopButton) == LOW && startFlag){
    pauseFlag = true;
    // pauseTime = millis();
    startFlag = false;
    Serial.println("Timer Paused!");
  }

// timer settings
  if(startFlag){
    elapsedTime = currentTime-startTime;
    remainingTime = countdownDuration-elapsedTime;
    // pauseTime=0;
    
    if(currentTime-startTime>=countdownDuration){
      Serial.println("TIME IS UP!");
      if(cursor<segments){
        cursor++;
        totalTime();
      }else{
        cursor=0;
        lcd.setCursor(0, 3);
        lcd.print("--- TIME IS UP!! ---");
        startFlag=false;
      }
        

      
      startTime = currentTime;
    }else{
      int hoursLeft = remainingTime/3600000UL;
      int minutesLeft = (remainingTime%3600000UL)/60000UL;
      int secondsLeft = (remainingTime%60000UL)/1000UL;

      lcd.setCursor(2,1);
      lcd.print("Time: ");
      // lcd.setCursor(7,1);
      lcd.print(hoursLeft);
      // lcd.setCursor(9,1);
      lcd.print(":");
      // lcd.setCursor(10,1);
      lcd.print(minutesLeft);
      lcd.print(":");
      // lcd.setCursor(10,1);
      lcd.print(secondsLeft);
      lcd.print(" ");

      lcd.setCursor(14,0);
      lcd.print(cursor);
      lcd.print("/");
      lcd.print(segments);

      lcd.setCursor(0,2);
      lcd.print("Speed:");
      lcd.print(speed[cursor]);
      lcd.print("% ");
      lcd.setCursor(11,2);
      lcd.print("Dir: ");
      if(direction[cursor]==0){
        lcd.print("CW");
      }else{
        lcd.print("CCW");
      }
        
      
      
      Serial.print(segments);
      Serial.print("\t");
      Serial.print(cursor);
      Serial.print("\t");
      Serial.print(hoursLeft);
      Serial.print(" , Mins: ");
      Serial.println(minutesLeft);
      delay(1000);

    }
    
  }else{
    // Serial.println("Timer is OFF!");
    delay(250);

  }
  
// Serial.print(currentMenuState);
// Serial.print("\t");
// Serial.print(slide);
// Serial.print("\t");
// Serial.println(menuIndex);
// delay(250);

}

void totalTime(){
  countdownDuration=0;
  countdownDuration = hours[cursor] * 3600000UL + minutes[cursor] *60000UL;

}

void encoderInterrupt() {
  // encoder interrupt handler
  int CLKState = digitalRead(encoderPinCLK);
  int DTState = digitalRead(encoderPinDT);

  if (CLKState != lastCLKState) {
    if (DTState != CLKState){
      lastValue++;
    } else {
      lastValue--;
    }
    lastCLKState = CLKState;
    encoderRotated = true; // Set the encoder rotation flag
  }
}
void handleEncoderRotation() {
  int encoded = lastValue;
  int dir = encoded / abs(encoded); // Extract direction from the value

  if (currentMenuState == SEGMENT_MENU){
    updateMenu();
    segments = normalizeValue(segments + dir, 0, 10);
    lcd.setCursor(15, 2);
    lcd.print("     "); // Clear previous value
    lcd.setCursor(15, 2);
    lcd.print(segments);
    lastValue = 0;
  }else if(currentMenuState == MAIN_MENU){
    menuIndex = (menuIndex + dir + menuItems) % menuItems;
    updateMenu();
  }else if(currentMenuState == SETTINGS_MENU){
    // if(menuIndex == 0){
    //   lcd.setCursor(1, 0);
    //   lcd.print(""); // Clear previous value

    // }
    if(menuIndex == 1){
      hours[slide] = normalizeValue(hours[slide] + dir, 0, 2);
      lcd.setCursor(8, 1);
      lcd.print("  "); // Clear previous value
      lcd.setCursor(8, 1);
      lcd.print(hours[slide]);
      lastValue = 0;
    }else if(menuIndex == 2){
      minutes[slide] = normalizeValue(minutes[slide] + dir, 0, 60);
      lcd.setCursor(16, 1);
      lcd.print("     "); // Clear previous value
      lcd.setCursor(16, 1);
      lcd.print(minutes[slide]);
      // lcd.print(" %");
      lastValue = 0;

    }else if(menuIndex == 3){
      speed[slide] = normalizeValue(speed[slide] + dir*5, 0, 100);
      lcd.setCursor(8, 2);
      lcd.print("   "); // Clear previous value
      lcd.setCursor(8, 2);
      lcd.print(speed[slide]);
      lcd.print("% ");
      lastValue = 0;

    }else if(menuIndex == 4){
      direction[slide] = normalizeValue(direction[slide] + dir, 0, 1);
      lcd.setCursor(16, 2);
      lcd.print("     "); // Clear previous value
      lcd.setCursor(16, 2);
      lcd.print(direction[slide]);
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
  // lcd.clear();
  if(currentMenuState == SEGMENT_MENU){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Select Time:-");
    lcd.setCursor(0, 2);
    lcd.print("Segments : ");
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
    // lcd.print("");
    lcd.print(slide);
    lcd.print("/");
    lcd.print(segments);
    // lcd.setCursor(7, 0);
    // lcd.print(menuIndex);
    if(menuIndex ==0){
      // lcd.print(" ");
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
    lcd.print(direction[slide]);

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
      lcd.print(direction[slide]);
    }else if(menuIndex == 5){
      lcd.setCursor(15, 0);
      lcd.print("Next ");
      Serial.println("menuIndex = 5");
    }else if(menuIndex == 0){
      lcd.setCursor(0, 0);
      lcd.print("Back ");
      Serial.println("menuIndex = 0");

    }
  }else if(currentMenuState == START_STOP){
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Start/Stop");
    
    lcd.setCursor(14,0);
    // lcd.print("Seg: ");
    lcd.print(cursor);
    lcd.print("/");
    lcd.print(segments);
    lcd.setCursor(0,2);
    lcd.print("Speed:");
    lcd.print(speed[cursor]);
    lcd.print("% ");
    lcd.setCursor(11,2);
    lcd.print("Dir: ");
  }
 }

