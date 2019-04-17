#include <AccelStepper.h>
//#include <MultiStepper.h>

const byte disableStepperDriverPin = 8;  //Enable pin on DQ542MA, When high, the stepper driver goes into sleep mode reducing heat on the driver and stepper motor
const byte stepperDirectionPin = 9; // Direction pin on DQ542MA
const byte stepperStepPin = 10; //Pulse pin on DQ542MA
const byte homeSwitchNO = 11;// Normally open position of

AccelStepper stepperX(1, stepperStepPin, stepperDirectionPin);   // 1 = Driver interface
int manualMoveDistance = 512; // this is a 10th of an inch since 5120 pulses equals an inch
long initial_homing = 10; // Used to Home Stepper, when homing, the stepper moves this many steps, in each loop THEN looks for the limit switch, so don't make too big

long setRunMinutes = 60; //valid between 1 and 1440
int maxRunMinutes = 1440; //max run time or 1440 equals 24 hours

int elapsedMinutes = 0; // keeps track of elapsed min
unsigned long elapsedMillis = 0; // keeps track of elapsed min
unsigned long startMillis = 0;
int cycleCount = 0; // keeps track of current cycle
boolean homed = false;
long setBeginPositionPulses = 0.0;
long setEndPostionPulses = 74240; //1" = 5,120 pluses or 14.5" = 74240 pulses
float maxEndPosition = 74240;
int minutesMultiplier = 30; // How many minutes one push of the button increases the minutes,
byte selectMode = 3; // this is the start up postion, 0 = Set begin Location, 1 =Set End, 2 = Run Time (Min), 3 = Start Stop Screen, 4 = manual Operation
int pulsePerInch = 5120;
boolean  varChanged = true; // uesd to keep the refresh on LCD to only when a change has happened
volatile byte runStop = false; // This is a variable is global servo move or run, false = stop, true = run
float cyclesPerMin = .2;  // this is not correct, it should take into acount the difference between end and begin


#include <Bounce2.h>
const byte upButtonPin = 4;
const byte downButtonPin = 5;
const byte selectButtonPin = 6;
const byte startButtonPin = 7;
Bounce debouncerUpButton = Bounce();
Bounce debouncerDownButton = Bounce();
Bounce debouncerSelectButton = Bounce();
Bounce debouncerStartButton = Bounce();
int upButtonState = 0;
int downButtonState = 0;
int  selectButtonState = 0;
int  startButtonState = 0;


// include the library code:
#include "Wire.h"
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 20, 4); // set the LCD address to 0x3F for a 16 chars and 2 line display

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // set up the LCD's number of rows and columns:
  lcd.init();                      // initialize the lcd
  lcd.setBacklight(HIGH);
  debouncerUpButton.attach(upButtonPin);
  debouncerDownButton.attach(downButtonPin);
  debouncerSelectButton.attach(selectButtonPin);
  debouncerStartButton.attach(startButtonPin);
  debouncerUpButton.interval(25);
  debouncerDownButton.interval(25);
  debouncerSelectButton.interval(25);
  debouncerStartButton.interval(25);

  pinMode(homeSwitchNO, INPUT);
  pinMode(disableStepperDriverPin, OUTPUT);
  digitalWrite(disableStepperDriverPin, HIGH);  // Low enables the stepper, high disables the stepper
  stepperX.setMaxSpeed(1000.0);      //Set max speed of stepper
  stepperX.setAcceleration(1000.0);  // Set acceleration of stepper

}
void loop() {
  // put your main code here, to run repeatedly:  Super Loop
  upButton();
  downButton();
  startButton();
  selectButton();
  lcdUpDate();
  buttonUpDate();
}

void buttonUpDate() {
  debouncerUpButton.update();
  debouncerDownButton.update();
  debouncerSelectButton.update();
  debouncerStartButton.update();
  upButtonState = debouncerUpButton.rose();
  downButtonState = debouncerDownButton.rose();
  selectButtonState = debouncerSelectButton.rose();
  startButtonState = debouncerStartButton.rose();

}

void upButton() {
  if (upButtonState) {
    //    Serial.println("Up Button");
    switch (selectMode) {
      case 0:  //  Set the Begin postion pulses
        varChanged = true;
        setBeginPositionPulses = setBeginPositionPulses + (pulsePerInch / 4);
        setBeginPositionPulses = constrain(setBeginPositionPulses, 0, setEndPostionPulses - pulsePerInch);

        break;

      case 1:// set end postion pulses
        varChanged = true;
        setEndPostionPulses = setEndPostionPulses + (pulsePerInch / 4);
        setEndPostionPulses = constrain(setEndPostionPulses, setBeginPositionPulses, maxEndPosition);


        break;

      case 2:
        //set run time in minutes
        if ( setRunMinutes < maxRunMinutes)  {
          varChanged = true;
          setRunMinutes = setRunMinutes + minutesMultiplier;
        }



        break;

      case 3:
        //the upbutton doesn't do anything on selectMode = 3
        break;

      case 4:
        //Manual operation of the slide up and down
        manualMove( pulsePerInch / 10);  // move backwards a 10th of an inch

        break;
    }
  }
}

void downButton() {
  if (downButtonState) {
    //    Serial.println("Down Button");
    switch (selectMode) {
      case 0:  //
        varChanged = true;
        setBeginPositionPulses = setBeginPositionPulses - (pulsePerInch / 4);
        setBeginPositionPulses = constrain(setBeginPositionPulses, 0, setEndPostionPulses - pulsePerInch);

        break;
      case 1:
        varChanged = true;
        setEndPostionPulses = setEndPostionPulses - (pulsePerInch / 4) ;
        setEndPostionPulses = constrain(setEndPostionPulses, setBeginPositionPulses + pulsePerInch, maxEndPosition);


        break;

      case 2:
        //set run time in minutes
        if ( 35 < setRunMinutes)  {
          varChanged = true;
          setRunMinutes = setRunMinutes - minutesMultiplier;
        }

        break;

      case 3:
        //the down button doesn't do anything on selectMode = 3
        break;

      case 4:
        //Manual operation of the slide up and down
        manualMove(-1 * pulsePerInch / 10);  // move backwards a 10th of an inch

        break;
    }
  }
}

void startButton() {

  if (startButtonState) {
    if (selectMode == 3) {

      Serial.println("Run Started");


      digitalWrite(disableStepperDriverPin, LOW);  // LOW enables the stepper, HIGH disables the stepper  (this is for testing only remove
      //      homeStepper();  //  this function won't work until the home swith is connected
      startMillis = millis();


      stepperX.setMaxSpeed(3000.0);      //Set max speed of stepper
      stepperX.setAcceleration(2000.0);  // Set acceleration of stepper

      Serial.println(millis() - startMillis);
      unsigned long setRunMillis = setRunMinutes * 60000;
      Serial.println(setRunMillis);


      while ((millis() - startMillis) <= setRunMillis) { //To exit out of this while loop the eleaped time OR the start button triggers
        elapsedMinutes = ((millis() - startMillis) / 60000);
        Serial.print("Looping in the run loop going out - Elapsed minutes = ");
        Serial.println(elapsedMinutes);
        varChanged = true;
        lcdUpDate();


        
        // Going out to end postion starting from the home postion
        stepperX.runToNewPosition(setEndPostionPulses); //  1" = 5,120 pluses. This is a blocking function, such that it doesn't go to the next step until complete

        Serial.print("Looping in the run loop coming back - Elapsed minutes = ");
        Serial.println(elapsedMinutes);
        elapsedMinutes = ((millis() - startMillis) / 60000);
        varChanged = true;
        lcdUpDate();

        // Going to home postion starting from the end postion
        stepperX.runToNewPosition(setBeginPositionPulses); //This is a blocking function, such that it doesn't go to the next step until complete

      }
      digitalWrite(disableStepperDriverPin, HIGH);  // LOW enables the stepper, HIGH disables the stepper
      Serial.println("exited out of run loop");
    }
  }
}


void selectButton() {
  if (selectButtonState) {
    varChanged = true;
    if (3 < selectMode) {
      selectMode = 0;
    }
    else {
      selectMode++;
    }

  }
}



void lcdUpDate() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  //lcd.setCursor(0, 1);
  switch (selectMode) {
    case 0:  //
      if (varChanged == true) {
        varChanged = false;
        lcd.blink();
        lcd.clear();
        lcd.setCursor(0, 0);  // column, row
        lcd.print("SET BEGIN=");

        float tempBeginInches = (setBeginPositionPulses * 1.00 / pulsePerInch * 1.00);

        lcd.print(tempBeginInches); //add precition of 1 decimal place
        lcd.print('"');
        lcd.setCursor(0, 1);    // column, row
        lcd.print("    END=");
        lcd.print((setEndPostionPulses / pulsePerInch) * 1.0);
        lcd.print('"');
        lcd.setCursor(14, 0);    // column, row

      }
      break;

    case 1:
      if (varChanged == true) {
        varChanged = false;
        lcd.blink();
        lcd.clear();
        lcd.setCursor(0, 0);    // column, row
        lcd.print("    BEGIN=");
        float tempBeginInches = (setBeginPositionPulses * 1.00 / pulsePerInch * 1.00);
        lcd.print(tempBeginInches); //add precition of 1 decimal place
        lcd.print('"');
        lcd.setCursor(0, 1);  // column, row
        lcd.print("SET END=");
        float tempEndInches = (setEndPostionPulses * 1.00 / pulsePerInch * 1.00);
        lcd.print(tempEndInches);
        lcd.print('"');
        lcd.setCursor(14, 1);    // column, row
      }
      break;

    case 2:
      if (varChanged == true) {
        varChanged = false;
        lcd.blink();
        lcd.clear();
        lcd.setCursor(0, 0);  // column, row
        lcd.print("SET MINUTES=");
        lcd.print(setRunMinutes, 1); //Serial.print(val, format) format here means 1 decimal place
        lcd.setCursor(0, 1);  // column, row
        lcd.print("    ");

        lcd.print( setRunMinutes / 60.0, 1); //  this is an int, so decimal math is wrong, as well as not taking into consideration the distance traveled  need to test  to find
        lcd.print(" HRS");
        lcd.setCursor(15, 0);  // column, row
      }

      break;

    case 3:
      if (varChanged == true) {
        varChanged = false;
        lcd.noBlink();
        lcd.clear();
        lcd.setCursor(0, 0);  // column, row
        lcd.print("START  /  STOP");
        lcd.setCursor(0, 1);  // column, row
        //        lcd.print(cycleCount);
        lcd.print(elapsedMinutes);
        lcd.print(" OF ");
        lcd.print(setRunMinutes); //add precition of 0 decimal place
        lcd.print(" MIN");

      }
      break;

    case 4:
      if (varChanged == true) {
        varChanged = false;
        lcd.noBlink();
        lcd.clear();
        lcd.setCursor(0, 0);  // column, row
        lcd.print("MANUAL OPERATION");
        lcd.setCursor(0, 1);
        lcd.print("UP/DOWN TO MOVE");
      }
      break;
  }
}



void manualMove(long moveDistance) {
  // this function is a while loop that stays active for a couple of minutes then
  // allowing for the magnet to be moved by presing the buttions - dont home just move
  // a couple of pulese for each press in the desired direction

  digitalWrite(disableStepperDriverPin, LOW);  // LOW enables the stepper, HIGH disables the stepper

  stepperX.setCurrentPosition(0);    // Set the current postion as home position
  stepperX.setMaxSpeed(1000.0);      //Set max speed of stepper
  stepperX.setAcceleration(1000.0);  // Set acceleration of stepper
  stepperX.moveTo(moveDistance); // a positive moves in a positve direction, negative in a backwards direction
  homed = false;
  digitalWrite(disableStepperDriverPin, HIGH);  // LOW enables the stepper, HIGH disables the stepper
}


void homeStepper() {
  digitalWrite(disableStepperDriverPin, LOW);  // LOW enables the stepper, HIGH disables the stepper
  stepperX.setMaxSpeed(1000.0);      //Set max speed of stepper
  stepperX.setAcceleration(1000.0);  // Set acceleration of stepper
  stepperX.setCurrentPosition(0);    // Set the current postion as home position
  initial_homing = 1; //this changes based on if going backwards or forwards

  while ( digitalRead(homeSwitchNO) == true) {

    stepperX.moveTo(initial_homing);
    stepperX.setSpeed(-1000);
    initial_homing = initial_homing + 5; //moving more in each loop
    stepperX.run();   //Go to stepperX.moveTo position
    //    Serial.println(initial_homing);
  }

  stepperX.setCurrentPosition(0);    // reset the home position with it's current postion - this also requires a setmaxSpeed and setAcceleration
  stepperX.setMaxSpeed(1000.0);      //Set max speed of stepper (Slower to get better accuracy) 100 is a good starting number
  stepperX.setAcceleration(5000.0);  // Set acceleration of stepper
  initial_homing = 100000;  //this changes based on if going backwards or forwards

  while ( digitalRead(homeSwitchNO) == false) {
    stepperX.moveTo(initial_homing); //go home as defined in setup, it can be close to the microswitch of the plate in the middle
    stepperX.setSpeed(+1000);
    initial_homing = initial_homing  + 5 ; //
    stepperX.run();   //Go to set postion

  }

  stepperX.setCurrentPosition(0);    // reset the home position with it's current postion it is now homed - this also requires a setmaxSpeed and setAcceleration
  homed = true;
}
