#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <Time.h>
#include <avr/wdt.h>
#include <EEPROM.h>

SoftwareSerial mySerial(5, 2);
Adafruit_GPS GPS(&mySerial);

#define GPSECHO  false // Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console

#define PIN_CYCLE 4

#define CYCLING_MODE  1
#define STAND_BY_MODE 2

#define INPUT_SIZE 30

// *** Configurable params ***
long unsigned syncFreqMillis;

int standByStartHour, standByStartMinute, standByEndHour, standByEndMinute;
int unsigned timeLapseHigh, timeLapseDown;

uint16_t UTCoffset;

// *** Program variables ***
uint8_t currentMode = CYCLING_MODE;
uint32_t lastTimeSync = 0;
const boolean pulseGreaterThanSecond = true;
boolean checkStatusEnabled = true; // used when force cycle during "stand by" mode period



// *** Program constants ***
//const int unsigned powerOnPin = 5; // LED 'on'
const int unsigned waitingGPSPin = 7; //LED 'waiting'
const int unsigned SyncUTCPin = 8; // LED showing the sync

const uint8_t SATURDAY = 7;
const uint8_t SUNDAY = 1;

uint8_t volatile secondsAcumHigh = 0;
uint8_t volatile secondsAcumDown = 0;
boolean volatile validateStatusHigh = true;



void setup()
{

  Serial.begin(115200);
  Serial.println("UTC sync POC (by aler) v1.1.0");

  loadInitialValues();
  
  initGPS();
  //digitalWrite(powerOnPin, HIGH);

  Serial.println("Waiting for GPS Fix, pleas wait...");
  waitGPSFix();
  digitalWrite(waitingGPSPin, HIGH);

  syncUTC();

  setInitialTime();

  Serial.println("End Setup.");
}


void loop()
{
  if(checkStatusEnabled)
      checkStatus();

  // TODO: modify it in order to use a time interrupt
  if ( currentMode == CYCLING_MODE &&
        pulseGreaterThanSecond && 
        (millis() - lastTimeSync) >= syncFreqMillis) 
      {
      detachInterrupt(1);
      digitalWrite(PIN_CYCLE, LOW);
      syncUTC();
      setCycleInterrupt();
  }

  readComandRequest();
  
}

void readComandRequest(){
  if(Serial.available() > 0)
    {
      char input[INPUT_SIZE + 1];
      byte size = Serial.readBytes(input, INPUT_SIZE); // read input
      input[size] = 0;
      char* command = strtok(input, " "); // split input
         
      String commandToExecute;
      commandToExecute = command; 
  
      char* params[2];// store params if exists, up to 2 params
      command = strtok(NULL, " ");
      if(command!=NULL){
          params[0] = command;
          command = strtok(NULL, " ");
          if(command!=NULL){
            params[1] = command;
          }
       }     
       
       if(params[0]!=NULL){
        Serial.print("param 1:");Serial.println(params[0]);
       }     
       if(params[1]!=NULL){
        Serial.print("param 2:");Serial.println(params[1]);
       }

       if(commandToExecute=="RESET"){
          reset();
       }
       if(commandToExecute=="START-CYCLE"){
          checkStatusEnabled = false; // in order to not to change to "stand by" mode
          currentMode = CYCLING_MODE;
          syncUTC();
          setCycleInterrupt(); 
       }
  }  
}

void reset(){
  wdt_enable(WDTO_15MS);
}

void sendPulseGreaterThanSecond() {

  Serial.println(digitalRead(PIN_CYCLE), DEC);

  if (validateStatusHigh) {
    if (secondsAcumHigh < timeLapseHigh) {
      digitalWrite(PIN_CYCLE, HIGH);
      secondsAcumHigh++;
    } else {
      digitalWrite(PIN_CYCLE, LOW);
      validateStatusHigh = false;
      secondsAcumHigh = 0;
      secondsAcumDown++;
    }
  } else if (secondsAcumDown < timeLapseDown) {
    secondsAcumDown++;
  } else {
    digitalWrite(PIN_CYCLE, HIGH);
    secondsAcumHigh++;
    secondsAcumDown = 0;
    validateStatusHigh = true;
  }
}


void sendPulseLessThanSecond() {
  Serial.println(1, DEC);
  digitalWrite(PIN_CYCLE, HIGH);
  delay(200);
  Serial.println(0, DEC);
  digitalWrite(PIN_CYCLE, LOW);
  delay(800);

}

void  setCycleInterrupt() {
  secondsAcumHigh = 0;
  secondsAcumDown = 0;
  validateStatusHigh = true;

  if (pulseGreaterThanSecond) {
    attachInterrupt(1, sendPulseGreaterThanSecond, RISING);
    /*   Param 1 means the interrupts will check the PIN 3 (in Arduino UNO)
         RISING means interrupt will be thrown when PIN value change from LOW to HIGH.
         PIN 3 will be connected to PPS (GPS PIN module).
         It means interrupt wil be thrown at every second start.
    */
  } else {
    attachInterrupt(1, sendPulseLessThanSecond, RISING);
  }
}

/* TODO: modify it in order to use interrupts with timer */
void checkStatus() {
  if ( (currentMode == CYCLING_MODE) &&
      ( weekday() == SATURDAY || 
        weekday() == SUNDAY ||
        (hour() >= standByStartHour) ||
        (hour() < standByEndHour)
      )
     ) { // change to stand by mode, leave PIN as HIGH
      detachInterrupt(1);
      digitalWrite(PIN_CYCLE, HIGH);
      currentMode = STAND_BY_MODE;
      Serial.println("STAND BY mode...");
  }
  else if (
    (weekday() != SATURDAY && weekday() != SUNDAY) &&
    hour() >= standByEndHour) { // Wake up from stand by
      reset();
  }
}

void syncUTC() {

  Serial.println("Synchronizing with second 0 from UTC...");
  GPS.read();
  while (true) {
    if (GPS.newNMEAreceived()      && GPS.parse(GPS.lastNMEA())            && GPS.seconds == 0 && GPS.milliseconds == 0)
      break;
    GPS.read();
  }

  lastTimeSync = millis();
  Serial.println("Synchronized successfully.");
}

void loadInitialValues(){
  UTCoffset = -3;
  timeLapseHigh = 4;
  timeLapseDown = 1;

  syncFreqMillis = 295000; // 290000 - 4' 55''

  standByStartHour = 19;
  standByStartMinute = 0;
  standByEndHour = 7;
  standByEndMinute = 0;

}


