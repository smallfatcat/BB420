// BB420 alpha version


#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h> // includes the LiquidCrystal Library
LiquidCrystal_I2C	lcd(0x27,2,1,0,4,5,6,7); // 0x27 is the I2C bus address for an unmodified backpack
// Constants
const int MODE_AUTO = 0;
const int MODE_MANUAL = 1;
const int MODE_SETTINGS = 2;
const int LEFT = 0;
const int RIGHT = 1;
// Button Pins
const int buttonA = 5;
const int buttonB = 4;
const int buttonC = 3;
const int buttonD = 2;

// Initial States
boolean modeButtonHasReset = true;

int buttonStateA;
int buttonStateB;
int buttonStateC;
int buttonStateD;

int lastButtonStateA = HIGH;
int lastButtonStateB = HIGH;
int lastButtonStateC = HIGH;
int lastButtonStateD = HIGH;

unsigned long lastDebounceTimeA = 0;
unsigned long lastDebounceTimeB = 0;
unsigned long lastDebounceTimeC = 0;
unsigned long lastDebounceTimeD = 0;
unsigned long debounceDelay = 50;

unsigned long lastFrameTime = 0;

int mode = MODE_AUTO;
int railDirection = RIGHT;
int frame = 0;
long railPos = 0;


void setup() {
  lcd.begin(16,2); // Initializes the interface to the LCD screen, and specifies the dimensions (width and height) of the display
  lcd.setBacklightPin(3,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.noCursor();
  pinMode(buttonA, INPUT_PULLUP);
  pinMode(buttonB, INPUT_PULLUP);
  pinMode(buttonC, INPUT_PULLUP);
  pinMode(buttonD, INPUT_PULLUP);
}

void loop() {
  unsigned long currentFrameTime = millis();
  boolean drawFrame = false;
  if(currentFrameTime - lastFrameTime > 100){
    drawFrame = true;
    lastFrameTime = currentFrameTime;
  }
  frame++;
  if(drawFrame){
    lcd.clear();
  }
  
  // Debounce buttons
  int readingA = digitalRead(buttonA);
  int readingB = digitalRead(buttonB);
  int readingC = digitalRead(buttonC);
  int readingD = digitalRead(buttonD);
  if (readingA != lastButtonStateA) {
    // reset the debouncing timer
    lastDebounceTimeA = millis();
  }
  if ((millis() - lastDebounceTimeA) > debounceDelay) {
    if (readingA != buttonStateA) {
      buttonStateA = readingA;
    }
  }
  lastButtonStateA = readingA;
  
  if (readingB != lastButtonStateB) {
    // reset the debouncing timer
    lastDebounceTimeB = millis();
  }
  if ((millis() - lastDebounceTimeB) > debounceDelay) {
    if (readingB != buttonStateB) {
      buttonStateB = readingB;
    }
  }
  lastButtonStateB = readingB;
  
  if (readingC != lastButtonStateC) {
    // reset the debouncing timer
    lastDebounceTimeC = millis();
  }
  if ((millis() - lastDebounceTimeC) > debounceDelay) {
    if (readingC != buttonStateC) {
      buttonStateC = readingC;
    }
  }
  lastButtonStateC = readingC;
  
  if (readingD != lastButtonStateD) {
    // reset the debouncing timer
    lastDebounceTimeD = millis();
  }
  if ((millis() - lastDebounceTimeD) > debounceDelay) {
    if (readingD != buttonStateD) {
      buttonStateD = readingD;
    }
  }
  lastButtonStateD = readingD;
  
  // Actions for MODE button
  if(buttonStateA==LOW){
    if(drawFrame){
      lcd.setCursor(0,0); 
      lcd.print("A");
    }
    if(modeButtonHasReset){
      mode++;
    }
    modeButtonHasReset = false;
    if(mode > MODE_SETTINGS){
      mode = MODE_AUTO;
    }
  }
  if(buttonStateA==HIGH){
    modeButtonHasReset = true;
  }
  // Actions for LEFT button
  if(buttonStateB==LOW){
    if(drawFrame){
      lcd.setCursor(1,0); 
      lcd.print("B");
    } 
    railDirection = LEFT;
    if(mode==MODE_MANUAL){
      moveRail();
    }
  }
  // Actions for RIGHT button
  if(buttonStateC==LOW){
    if(drawFrame){
      lcd.setCursor(2,0); 
      lcd.print("C");
    }
    railDirection = RIGHT;
    if(mode==MODE_MANUAL){
      moveRail();
    }
  }
  // Actions for SELECT button
  if(buttonStateD==LOW){
    if(drawFrame){
      lcd.setCursor(3,0); 
      lcd.print("D"); 
    }
  }
  // Mode actions
  if(drawFrame){
    lcd.setCursor(0,1);
  }
  if(mode==MODE_AUTO){
    if(drawFrame){
      lcd.print("MODE: AUTO");
    }
    moveRail();
  }
  if(mode==MODE_MANUAL){
    if(drawFrame){
      lcd.print("MODE: MANUAL");
    }
  }
  if(mode==MODE_SETTINGS){
    if(drawFrame){
      lcd.print("MODE: SETTINGS");
    }
  }
  if(drawFrame){
    lcd.setCursor(4,0);
    if(railDirection == LEFT){
      lcd.print("L");
    }
    if(railDirection == RIGHT){
      lcd.print("R");
    }
    lcd.print("POS: ");
    lcd.print(railPos);
  }
  //delay(1);
}

void moveRail(){
  if(railDirection == LEFT){
    railPos--;
  }
  if(railDirection == RIGHT){
    railPos++;
  }
}
