// BB420 alpha version


#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h> // includes the LiquidCrystal Library
LiquidCrystal_I2C	lcd(0x27,2,1,0,4,5,6,7); // 0x27 is the I2C bus address for an unmodified backpack
// Constants
const int MODE_AUTO = 0;
const int MODE_MANUAL = 1;
const int MODE_SETAUTOSPEED = 2;
const int MODE_SETMANSPEED = 3;
const int LEFT = 1;
const int RIGHT = 0;

// Button Pins
const int buttonA = 5;
const int buttonB = 4;
const int buttonC = 3;
const int buttonD = 2;

// Stepper Pins
const int stepPin = 6;
const int dirPin = 7;
const int mode0Pin = 8;
const int mode1Pin = 9;
const int mode2Pin = 10;


// Initial States
volatile long pulseCount = 0;
int autoSpeed = 500;
int manSpeed = 3000;

int mode0State = LOW;
int mode1State = LOW;
int mode2State = HIGH;

boolean modeButtonHasReset = true;
boolean selectButtonHasReset = true;
boolean leftButtonHasReset = true;
boolean rightButtonHasReset = true;
boolean rightPressed = false;
boolean leftPressed = false;
boolean autoPaused = true;
boolean emergencyStop = true;

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
int railDirection = LEFT;
int frame = 0;
long railPos = 0;


void setup() {
  //setup Timer1
  cli();
  TCCR1A = 0b00000000;
  TCCR1B = 0b00001001;
  TIMSK1 |= 0b00000010;       //set for output compare interrupt
  OCR1A = 16000000/autoSpeed - 1; 
  sei();                      //enables interrups. Use cli() to turn them off
  
  lcd.begin(16,2); // Initializes the interface to the LCD screen, and specifies the dimensions (width and height) of the display
  lcd.setBacklightPin(3,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.noCursor();
  pinMode(buttonA, INPUT_PULLUP);
  pinMode(buttonB, INPUT_PULLUP);
  pinMode(buttonC, INPUT_PULLUP);
  pinMode(buttonD, INPUT_PULLUP);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(mode0Pin, OUTPUT);
  pinMode(mode1Pin, OUTPUT);
  pinMode(mode2Pin, OUTPUT);
  digitalWrite(mode0Pin, mode0State);
  digitalWrite(mode1Pin, mode1State);
  digitalWrite(mode2Pin, mode2State);
  digitalWrite(dirPin, railDirection);
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
  
  //
  // Actions for MODE button
  if(buttonStateA==LOW){
    if(modeButtonHasReset){
      mode++;
    }
    modeButtonHasReset = false;
    if(mode > MODE_SETMANSPEED){
      mode = MODE_AUTO;
    }
  }
  if(buttonStateA==HIGH){
    modeButtonHasReset = true;
  }
  
  //
  // Actions for LEFT button
  if(buttonStateB==LOW){
    if(leftButtonHasReset){
      if(mode==MODE_AUTO || mode==MODE_MANUAL){
        railDirection = LEFT;
        digitalWrite(dirPin, railDirection);
      }
      if(mode==MODE_SETAUTOSPEED){
          autoSpeed-=10;
          OCR1A = 16000000/autoSpeed - 1;
      }
      if(mode==MODE_SETMANSPEED){
          manSpeed-=10;
          //OCR1A = 16000000/manSpeed - 1;
      }
    }
    if(mode==MODE_MANUAL){
       OCR1A = 16000000/manSpeed - 1;
       emergencyStop = false;
       leftPressed = true;
    }
    leftButtonHasReset = false;
  }
  if(buttonStateB==HIGH){
    leftButtonHasReset = true;
  }
  if(buttonStateB==HIGH && leftPressed ){
     OCR1A = 16000000/autoSpeed - 1;
     emergencyStop = true;
     leftPressed = false;
  }
  
  //
  // Actions for RIGHT button
  if(buttonStateC==LOW){
    if(rightButtonHasReset){
      if(mode==MODE_AUTO || mode==MODE_MANUAL){
        railDirection = RIGHT;
        digitalWrite(dirPin, railDirection);
      }
      if(mode==MODE_SETAUTOSPEED){
          autoSpeed+=10;
          OCR1A = 16000000/autoSpeed - 1;
      }
      if(mode==MODE_SETMANSPEED){
          manSpeed+=10;
          //OCR1A = 16000000/manSpeed - 1;
      }
    }
    if(mode==MODE_MANUAL){
      OCR1A = 16000000/manSpeed - 1;
      emergencyStop = false;
      rightPressed = true;
    }
    rightButtonHasReset = false;
  }
  if(buttonStateC==HIGH){
    rightButtonHasReset = true;
  }
  if(buttonStateC==HIGH && rightPressed ){
     OCR1A = 16000000/autoSpeed - 1;
     emergencyStop = true;
     rightPressed = false;
  }
  
  //
  // Actions for SELECT button
  if(buttonStateD==LOW){
    if(selectButtonHasReset){
      // invert stop flag
      emergencyStop = !emergencyStop;
     
    }
    selectButtonHasReset = false;
    
  }
  if(buttonStateD==HIGH){
    selectButtonHasReset = true;
  }
  // Mode actions
  if(drawFrame){
    lcd.setCursor(0,1);
  }
  if(mode==MODE_AUTO){
    if(drawFrame){
      lcd.print("AUTO ");
      if(autoPaused){
        lcd.print("PAUSED");
      }
      else{
        lcd.print("RUNNING");
      }
    }
    if(!autoPaused){
      moveRail();
    }
  }
  if(mode==MODE_MANUAL){
    if(drawFrame){
      lcd.print("MAN POS: ");
      lcd.print(railPos);
    }
  }
  if(mode==MODE_SETAUTOSPEED){
    if(drawFrame){
      lcd.print("AUTO SPEED: ");
      lcd.print(autoSpeed);
    }
  }
  if(mode==MODE_SETMANSPEED){
    if(drawFrame){
      lcd.print("MAN SPEED: ");
      lcd.print(manSpeed);
    }
  }
  if(drawFrame){
    lcd.setCursor(0,0);
    if(railDirection == LEFT){
      lcd.print("<");
    }
    if(railDirection == RIGHT){
      lcd.print(">");
    }
    lcd.print(" X: ");
    lcd.print(pulseCount);
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

ISR(TIMER1_COMPA_vect) {
    if(!emergencyStop){
      digitalWrite(stepPin, HIGH);       // Driver only looks for rising edge
      digitalWrite(stepPin, LOW);        //  DigitalWrite executes in 16 us  
      if(railDirection == LEFT){
        pulseCount++;
      }
      else{
        pulseCount--;
      }
      
      //Generate Rising Edge
      //PORTL =  PORTL |= 0b00001000;   //Direct Port manipulation executes in 450 ns  => 16x faster!
      //PORTL =  PORTL &= 0b11110111;
      //Location = Location + 250 * DirFlag ;  //Updates Location (based on 4000 Pulses/mm)
    }
}
