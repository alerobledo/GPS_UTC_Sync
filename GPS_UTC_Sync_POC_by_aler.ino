#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <Time.h>
#include <avr/wdt.h>
#include <EEPROM.h>

SoftwareSerial mySerial(5, 9);
Adafruit_GPS GPS(&mySerial);

#define GPSECHO  false // Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console

#define PIN_CYCLE 4
#define PIN_ON 6 // LED 'on'
#define PIN_SYNC_UTC 7 // LED showing the sync

#define CYCLING_MODE  1
#define STAND_BY_MODE 2

#define INPUT_SIZE 30

// *** Configurable params ***
struct ConfigValues {
  char initValuesConfigured;
  int UTCoffset;
  int unsigned timeLapseHigh;
  int unsigned timeLapseDown;

  long unsigned syncFreqMillis;

  int unsigned standByStartHour;
  int unsigned standByStartMinute;
  int unsigned standByEndHour;
  int unsigned standByEndMinute;
};
ConfigValues configValues;

// *** Program variables ***
uint8_t currentMode = CYCLING_MODE;
boolean checkStatusEnabled = true; // used when force cycle during "stand by" mode period



// *** Program constants ***

const uint8_t SATURDAY = 7;
const uint8_t SUNDAY = 1;

uint8_t volatile secondsAcum = 0;
boolean volatile validateStatusHigh = true;

int volatile millisecondsAcum = 0;
uint8_t volatile pulseLessThanSecond=0;


void setup()
{
  pinMode(PIN_CYCLE, OUTPUT);
  Serial.begin(115200);
  Serial.println("UTC sync POC (by aler) v0.1");

  loadInitialValues();

  initGPS();
  digitalWrite(PIN_ON, HIGH);

  Serial.println("Waiting for GPS Fix, pleas wait...");
  waitGPSFix();

  syncUTC();

  setCycleInterrupt();
  Serial.println("End Setup.");
}


void loop()
{
  if (checkStatusEnabled)
    checkStatus();

  readComandRequest();

}



void sendPulseGreaterThanSecond() {

  if (validateStatusHigh) {
    if (secondsAcum < configValues.timeLapseHigh) {
      digitalWrite(PIN_CYCLE, HIGH);
      secondsAcum++;
    } else {
      digitalWrite(PIN_CYCLE, LOW);
      validateStatusHigh = false;
      secondsAcum = 0;
    }
  } else if (secondsAcum < configValues.timeLapseDown) {
    secondsAcum++;
  } else {
    digitalWrite(PIN_CYCLE, HIGH);
    secondsAcum++;
    validateStatusHigh = true;
  }
}

void sendPulseLessThanSecond() {
  digitalWrite(PIN_CYCLE, HIGH);
  millisecondsAcum = 0;
}



void  setCycleInterrupt() {

  if (configValues.timeLapseHigh<100) { // timeLapseHigh is defined in seconds
    secondsAcum = 0;
    validateStatusHigh = true;
    attachInterrupt(1, sendPulseGreaterThanSecond, RISING);
        
  } else {// timeLapseHigh is defined in milliseconds
    pulseLessThanSecond = 1;    
    attachInterrupt(1, sendPulseLessThanSecond, RISING);
  }
}

/* TODO: modify it in order to use interrupts with timer */
void checkStatus() {
  if ( (currentMode == CYCLING_MODE) &&
       !isInCycleTimeRange()
     ) { // change to stand by mode, leave PIN as HIGH
    detachInterrupt(1);
    digitalWrite(PIN_CYCLE, HIGH);
    currentMode = STAND_BY_MODE;
    Serial.println("STAND BY mode...");
  }
  else if (
    (currentMode == STAND_BY_MODE) &&
    isInCycleTimeRange()) { // Wake up from stand by
    reset();
  }
}

boolean isInCycleTimeRange() {
             //  weekday() != SATURDAY &&
           //  weekday() != SUNDAY &&
 
  if (configValues.standByStartHour>configValues.standByEndHour){ //stand-by ends on next day
        return
           (hour() < configValues.standByStartHour) &&
           (hour() >= configValues.standByEndHour);
  }else{      //stand-by ends on the same day
        return !(
         (hour() >= configValues.standByStartHour) &&
         (hour() <= configValues.standByEndHour));
  }
}

void syncUTC() {

  digitalWrite(PIN_SYNC_UTC, HIGH);
  Serial.println("Synchronizing with second 0 from UTC...");
  while (true) {
    if (GPS.newNMEAreceived()      && GPS.parse(GPS.lastNMEA())            && GPS.seconds == 0 && GPS.milliseconds == 0){
      setTime(GPS.hour, GPS.minute, GPS.seconds, GPS.day, GPS.month, GPS.year);
      adjustTime(configValues.UTCoffset * SECS_PER_HOUR);      
      break;
    }      
  }

  Serial.println("Synchronized successfully.");
  digitalWrite(PIN_SYNC_UTC, LOW);
}

void loadInitialValues() {

  EEPROM.get(0, configValues);

  Serial.println("InitialValues: --------------------->>>");

  printStatus();
  
  if (configValues.initValuesConfigured != 'T') {
    Serial.println("Initial values has not be configured. Configuring default values...");
    configValues.initValuesConfigured = 'T';
    configValues.UTCoffset = -3;
    configValues.timeLapseHigh = 4;
    configValues.timeLapseDown = 1;
    configValues.syncFreqMillis = 295000; // 4' 55''
    configValues.standByStartHour = 19;
    configValues.standByEndHour = 7;
    EEPROM.put(0, configValues);
    //configValues.standByEndMinute =;
    Serial.println("Default values configured. Reset in 5 seconds...");
    delay(5000);
    reset();
  }

  Serial.println("InitialValues: ---------------------<<<");
}


