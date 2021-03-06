// drillpress


#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h> // includes the LiquidCrystal Library
LiquidCrystal_I2C	lcd(0x27,2,1,0,4,5,6,7); // 0x27 is the I2C bus address for an unmodified backpack
// Constants

const int MODE_MANUAL = 0;
const int MODE_SETMANSPEED = 1;
const int MODE_ZERO = 2;
const int MODE_DRILL = 3;
const int MODE_SETDRILL = 4;

const int MODE_AUTO = 5;
const int MODE_SETAUTOSPEED = 5;

const int UP = 1;
const int DOWN = 0;

const unsigned long repeatSpeedSlow = 500;
const unsigned long repeatSpeedFast = 100;

// Button Pins
const int buttonA = 9;
const int buttonB = 12;
const int buttonC = 11;
const int buttonD = 10;

// Stepper Pins
const int stepPin = 5;
const int dirPin = 4;
const int mode0Pin = A2;
const int mode1Pin = A1;
const int mode2Pin = A0;

// Touch probe pin
const int probePin = 13;


// Initial States
volatile long pulseCount = 0;
long drillDepth = -80;
int autoSpeed = 300;
int manSpeed = 300;
volatile int loopCount = 0;
int targetLoopCount = 0;
double stepsPermm = 80;

// 1/16 microsteps
int mode0State = LOW;
int mode1State = LOW;
int mode2State = HIGH;

boolean modeButtonHasReset = true;
boolean selectButtonHasReset = true;
boolean UPButtonHasReset = true;
boolean DOWNButtonHasReset = true;
boolean DOWNPressed = false;
boolean UPPressed = false;
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
unsigned long UPHeldSince = 0;
unsigned long DOWNHeldSince = 0;
unsigned long UPrepeatDelay = repeatSpeedSlow;
unsigned long DOWNrepeatDelay = repeatSpeedSlow;

unsigned long debounceDelay = 50;

unsigned long lastFrameTime = 0;

int mode = MODE_MANUAL;
int railDirection = UP;
int frame = 0;
long railPos = 0;


void setup() {
  //setup Timer1
  cli();
  TCCR1A = 0b00000000;
  TCCR1B = 0b00001001;        // set prescalar to 1
  TIMSK1 |= 0b00000010;       // set for output compare interrupt
  setMotorSpeed(manSpeed); 
  sei();                      // enables interrupts. Use cli() to turn them off
  
  lcd.begin(16,2); // Initializes the interface to the LCD screen, and specifies the dimensions (width and height) of the display
  lcd.setBacklightPin(3,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.noCursor();
  pinMode(probePin, INPUT_PULLUP);
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
  //Serial.begin(9600);
  //Serial.print("OCR1A: ");
  //Serial.println(OCR1A);
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
  // Actions for MODE button -----------------------------------------------------------------------------
  if(buttonStateA==LOW){
    if(modeButtonHasReset){
      mode++;
    }
    modeButtonHasReset = false;
    if(mode > MODE_SETDRILL){
      mode = MODE_MANUAL;
    }
  }
  if(buttonStateA==HIGH){
    modeButtonHasReset = true;
  }
  
  //
  // Actions for UP button -----------------------------------------------------------------------------
  if(buttonStateB==LOW){
    if(UPButtonHasReset){
      // deal with repeat
      UPHeldSince = millis();
      UPrepeatDelay = repeatSpeedSlow;
      if(mode==MODE_AUTO || mode==MODE_MANUAL || mode==MODE_ZERO || mode==MODE_DRILL){
        railDirection = UP;
        digitalWrite(dirPin, railDirection);
      }
      if(mode==MODE_SETAUTOSPEED){
          autoSpeed += 10;
          setMotorSpeed(autoSpeed);
      }
      if(mode==MODE_SETMANSPEED){
          manSpeed += 10;
          setMotorSpeed(manSpeed);
      }
      if(mode==MODE_SETDRILL){
          drillDepth += 1;
      }
    }
    if((UPHeldSince+UPrepeatDelay)<millis()){
      UPHeldSince = millis();
      UPrepeatDelay = repeatSpeedFast;
      if(mode==MODE_SETMANSPEED){
          manSpeed += 10;
          setMotorSpeed(manSpeed);
      }
      if(mode==MODE_SETDRILL){
          drillDepth += 1;
      }
    }
    if(mode==MODE_MANUAL || mode==MODE_ZERO || mode==MODE_DRILL){
       //setMotorSpeed(manSpeed);
       emergencyStop = false;
       UPPressed = true;
    }
    UPButtonHasReset = false;
  }
  
  if(buttonStateB==HIGH){
    UPButtonHasReset = true;
  }
  
  if(buttonStateB==HIGH && UPPressed ){
     //setMotorSpeed(autoSpeed);
     emergencyStop = true;
     UPPressed = false;
  }
  
  //
  // Actions for DOWN button -----------------------------------------------------------------------------
  if(buttonStateC==LOW){
    if(DOWNButtonHasReset){
      // deal with repeat
      DOWNHeldSince = millis();
      DOWNrepeatDelay = repeatSpeedSlow;
      if(mode==MODE_AUTO || mode==MODE_MANUAL || mode==MODE_ZERO || mode==MODE_DRILL){
        railDirection = DOWN;
        digitalWrite(dirPin, railDirection);
      }
      if(mode==MODE_SETAUTOSPEED){
          autoSpeed -= 10;
          setMotorSpeed(autoSpeed);
      }
      if(mode==MODE_SETMANSPEED){
          manSpeed -= 10;
          setMotorSpeed(manSpeed);
      }
      if(mode==MODE_SETDRILL){
          drillDepth -= 1;
      }
    }
    if((DOWNHeldSince+DOWNrepeatDelay)<millis()){
      DOWNHeldSince = millis();
      DOWNrepeatDelay = repeatSpeedFast;
      if(mode==MODE_SETMANSPEED){
          manSpeed -= 10;
          setMotorSpeed(manSpeed);
      }
      if(mode==MODE_SETDRILL){
          drillDepth -= 1;
      }
    }
    if(mode==MODE_MANUAL || mode==MODE_ZERO|| mode==MODE_DRILL){
      //setMotorSpeed(manSpeed);
      emergencyStop = false;
      DOWNPressed = true;
    }
    DOWNButtonHasReset = false;
  }
  if(buttonStateC==HIGH){
    DOWNButtonHasReset = true;
  }
  if(buttonStateC==HIGH && DOWNPressed ){
     //setMotorSpeed(autoSpeed);
     emergencyStop = true;
     DOWNPressed = false;
  }
  
  //
  // Actions for SELECT button -----------------------------------------------------------------------------
  if(buttonStateD==LOW){
    if(selectButtonHasReset){
      // invert stop flag
      //emergencyStop = !emergencyStop;
     /*// step once
      digitalWrite(stepPin, HIGH);       // Driver only looks for rising edge
      digitalWrite(stepPin, LOW);        //  DigitalWrite executes in 16 us
      if(railDirection == UP){
        pulseCount++;
      }
      else{
        pulseCount--;
      }*/
      if(mode==MODE_ZERO){
        pulseCount = 0;
      }
    }
    selectButtonHasReset = false;
    
  }
  if(buttonStateD==HIGH){
    selectButtonHasReset = true;
  }
  // Mode actions ---------------------------------------------------------------------------------------------
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
      lcd.print("JOG: ");
      //lcd.print(railPos);
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
      lcd.print("SET S: ");
      lcd.print(manSpeed);
    }
  }
  if(mode==MODE_DRILL){
    if(drawFrame){
      lcd.print("DRILL: ");
      //lcd.print(drillDepth);
      //lcd.print(", ");
      double Dmm = drillDepth/stepsPermm;
      lcd.print(Dmm);
      lcd.print("mm");
    }
  }
  if(mode==MODE_ZERO){
    if(drawFrame){
      lcd.print("ZERO: ");
      //lcd.print(manSpeed);
    }
  }
 if(mode==MODE_SETDRILL){
    if(drawFrame){
      lcd.print("SET D: ");
      //lcd.print(drillDepth);
      //lcd.print(", ");
      double Dmm = drillDepth/stepsPermm;
      lcd.print(Dmm);
      lcd.print("mm");
    }
  }
  if(drawFrame){
    lcd.setCursor(0,0);
    /*if(railDirection == UP){
      lcd.print("U");
    }
    if(railDirection == DOWN){
      lcd.print("D");
    }*/
    lcd.print("X: ");
    lcd.print(pulseCount);
    lcd.print(", ");
    double Xmm = pulseCount/stepsPermm;
    lcd.print(Xmm);
    lcd.print("mm");
  }
  //delay(1);
}

void moveRail(){
  if(railDirection == UP){
    railPos--;
  }
  if(railDirection == DOWN){
    railPos++;
  }
}

void setMotorSpeed(int newMotorSpeed){
  long timerCount = 16000000/newMotorSpeed - 1;
  if(timerCount < 65536){ 
    loopCount = 0;
    targetLoopCount = loopCount;
    OCR1A = timerCount;
    /*Serial.println("------");
    Serial.print("OCR1A: ");
    Serial.println(OCR1A);
    Serial.print("loopCount: ");
    Serial.println(loopCount);
    Serial.print("newMotorSpeed: ");
    Serial.println(newMotorSpeed);*/
  }
  else{
    loopCount = floor(timerCount / 65535);
    targetLoopCount = loopCount;
    OCR1A = round(timerCount / (loopCount+1));
    /*Serial.println("------");
    Serial.print("OCR1A: ");
    Serial.println(OCR1A);
    Serial.print("loopCount: ");
    Serial.println(loopCount);
    Serial.print("newMotorSpeed: ");
    Serial.println(newMotorSpeed);*/
  }
}

ISR(TIMER1_COMPA_vect) {
   
  // Read probe to set zero
  if(digitalRead(probePin) == LOW && railDirection == DOWN && mode==MODE_ZERO ){
    emergencyStop = true;
    pulseCount = 0;
  }
  if(mode==MODE_DRILL){
    if(pulseCount <= drillDepth && railDirection == DOWN){
      emergencyStop = true;
    }
  }
  if(!emergencyStop){
    if(loopCount == 0){
      loopCount = targetLoopCount;
      digitalWrite(stepPin, HIGH);       // Driver only looks for rising edge
      digitalWrite(stepPin, LOW);        //  DigitalWrite executes in 16 us  
      if(railDirection == UP){
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
    else{
      loopCount-- ;
    }
  }
}
    

