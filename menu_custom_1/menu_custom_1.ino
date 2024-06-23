#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);

// Pin configurations 
const int encoderPinCLK = 2;
const int encoderPinDT = 3;
const int buttonPin = 4;

// Settings configurations
int hours[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int minutes[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int speed[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool direction[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int cursor = 0;


// Menu Config
int menuIndex = 0;
const int menuItems = 5;
const char* menuOptions[] = {"Time:", "Speed:","Direction:", "-->","<--"};

enum MenuState {SEGMENT_MENU, MAIN_MENU, SETTINGS_MENU, START_STOP};
MenuState currentMenuState = SEGMENT_MENU; 

// Variables
volatile int lastCLKState;
volatile long lastValue = 0;
volatile bool encoderRotated = false;
int speedValue = 100;
int segments = 0;
int slide = 0 ;

void setup(){
  Serial.begin(9600);
  lcd.init();

  pinMode(encoderPinCLK, INPUT_PULLUP);
  pinMode(encoderPinDT, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);

  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("   Thruster-Timer   ");
  lcd.setCursor(0, 1);
  lcd.print("  Click Knobe to  ");
  lcd.setCursor(0, 2);
  lcd.print("    select Menu    ");
  delay(2000);

  attachInterrupt(digitalPinToInterrupt(encoderPinCLK), encoderInterrupt, CHANGE);

  lastCLKState = digitalRead(encoderPinCLK);


}
void loop() {
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
  }else if(currentMenuState == SETTINGS_MENU && digitalRead(buttonPin) == LOW && menuIndex!=3 && menuIndex!=4){
    currentMenuState = MAIN_MENU;
    updateMenu();
  }else if(currentMenuState == SETTINGS_MENU && menuIndex==3 && digitalRead(buttonPin) == LOW){
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
  }else if(currentMenuState == SETTINGS_MENU && menuIndex==4 && digitalRead(buttonPin) == LOW){
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
  }else if(currentMenuState == START_STOP && digitalRead(buttonPin) == LOW){
    currentMenuState = SEGMENT_MENU;
    updateMenu();
  }
  
Serial.print(currentMenuState);
Serial.print("\t");
Serial.print(slide);
Serial.print("\t");
Serial.println(menuIndex);
delay(250);

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
    if(menuIndex == 0){
      minutes[slide] = normalizeValue(minutes[slide] + dir, 0, 180);
      lcd.setCursor(15, menuIndex);
      lcd.print("     "); // Clear previous value
      lcd.setCursor(15, menuIndex);
      lcd.print(minutes[slide]);
    }else if(menuIndex == 1){
      speed[slide] = normalizeValue(speed[slide] + dir*5, 0, 100);
      lcd.setCursor(15, menuIndex);
      lcd.print("     "); // Clear previous value
      lcd.setCursor(15, menuIndex);
      lcd.print(speed[slide]);
      lcd.print(" %");

    }else if(menuIndex == 2){
      direction[slide] = normalizeValue(direction[slide] + dir, 0, 1);
      lcd.setCursor(15, menuIndex);
      lcd.print("     "); // Clear previous value
      lcd.setCursor(15, menuIndex);
      lcd.print(direction[slide]);
    }else if(menuIndex == 3){
      lcd.setCursor(15, menuIndex);
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
    lcd.print("Time segments:-");
    lcd.setCursor(0, 2);
    lcd.print("Segment count: ");
    lcd.print(segments);

  }else if (currentMenuState == MAIN_MENU) {
    lcd.clear();
    // lcd.setCursor(2, 0);
    // lcd.print("Menu:");
    lcd.setCursor(10, 3);
    lcd.print(slide);
   lcd.setCursor(7, 3);
    lcd.print(menuIndex);

    for (int i = 0; i < menuItems; i++) {
      lcd.setCursor(0, i);
      if(i==4 || i==3){
        lcd.setCursor(16, 3);
        lcd.print(menuOptions[3]);
        lcd.setCursor(1, 3);
        lcd.print(menuOptions[4]);
      }else{
        lcd.setCursor(1, i);
        lcd.print(menuOptions[i]);
      }
      if (i == menuIndex && i!=3 && i!=4) {
        lcd.setCursor(0, i);
        lcd.write(0);
        lcd.setCursor(19, 3);
    lcd.print(" ");
    lcd.setCursor(0, 3);
    lcd.print(" ");
        // lcd.setCursor(1, i);
        // lcd.write(1);
      } else if(i==3) {
        lcd.setCursor(19, 3);
        lcd.write(0);
      }else if(i==4 ){
        lcd.setCursor(0, 3);
        lcd.write(0);
      }else{
        lcd.print(" ");
    
        
      }
      
    }
    //  lcd.setCursor(19, 3);
    // lcd.print(" ");
    // lcd.setCursor(0, 3);
    // lcd.print(" ");
    lcd.setCursor(14, 0);
    lcd.print(minutes[slide]);
    lcd.setCursor(14, 1);
    lcd.print(speed[slide]);
    lcd.setCursor(14, 2);
    lcd.print(direction[slide]);

  } else if (currentMenuState == SETTINGS_MENU) {
    if(menuIndex == 0){
      lcd.setCursor(15, menuIndex);
      lcd.print(minutes[slide]);
    }else if(menuIndex == 1){
      lcd.setCursor(15, menuIndex);
      lcd.print(speed[slide]);
    }else if(menuIndex == 2){
      lcd.setCursor(15, menuIndex);
      lcd.print(direction[slide]);
    }else if(menuIndex == 3){
      lcd.setCursor(15, menuIndex);
      lcd.print("next ");
      Serial.println("menuIndex = 3");
    }else if(menuIndex == 4){
      lcd.setCursor(3, 3);
      lcd.print("back ");
      Serial.println("menuIndex = 4");

    }
  }else if(currentMenuState == START_STOP){
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Start/Stop");
  }
 }

