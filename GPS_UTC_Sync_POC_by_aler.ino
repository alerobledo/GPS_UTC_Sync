#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <Time.h>
#include <avr/wdt.h>
#include <EEPROM.h>

SoftwareSerial mySerial(5, 2);
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
uint32_t lastTimeSync = 0;
boolean checkStatusEnabled = true; // used when force cycle during "stand by" mode period



// *** Program constants ***

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
  digitalWrite(PIN_ON, HIGH);

  Serial.println("Waiting for GPS Fix, pleas wait...");
  waitGPSFix();

  syncUTC();

  setInitialTime();

  setCycleInterrupt();
  Serial.println("End Setup.");
}


void loop()
{
  if (checkStatusEnabled)
    checkStatus();

  // TODO: modify it in order to use a time interrupt
  if ( currentMode == CYCLING_MODE &&
       configValues.timeLapseHigh<100 && // timeLapseHigh is defined in seconds
       (millis() - lastTimeSync) >= configValues.syncFreqMillis)
  {
    detachInterrupt(1);
    digitalWrite(PIN_CYCLE, LOW);
    syncUTC();
    setCycleInterrupt();
  }

  readComandRequest();

}



void sendPulseGreaterThanSecond() {

  Serial.println(digitalRead(PIN_CYCLE), DEC);

  if (validateStatusHigh) {
    if (secondsAcumHigh < configValues.timeLapseHigh) {
      digitalWrite(PIN_CYCLE, HIGH);
      secondsAcumHigh++;
    } else {
      digitalWrite(PIN_CYCLE, LOW);
      validateStatusHigh = false;
      secondsAcumHigh = 0;
      secondsAcumDown++;
    }
  } else if (secondsAcumDown < configValues.timeLapseDown) {
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

  if (configValues.timeLapseHigh<100) { // timeLapseHigh is defined in seconds
    attachInterrupt(1, sendPulseGreaterThanSecond, RISING);
    /*   Param 1 means the interrupts will check the PIN 3 (in Arduino UNO)
         RISING means interrupt will be thrown when PIN value change from LOW to HIGH.
         PIN 3 will be connected to PPS (GPS PIN module).
         It means interrupt wil be thrown at every second start.
    */
  } else {// timeLapseHigh is defined in milliseconds
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
    if (GPS.newNMEAreceived()      && GPS.parse(GPS.lastNMEA())            && GPS.seconds == 0 && GPS.milliseconds == 0)
      break;
  }

  lastTimeSync = millis();
  Serial.println("Synchronized successfully.");
  digitalWrite(PIN_SYNC_UTC, LOW);
}

void loadInitialValues() {

  EEPROM.get(0, configValues);

  Serial.println("InitialValues: --------------------->>>");

  Serial.print("initValuesConfigured: "); Serial.println(configValues.initValuesConfigured);
  Serial.print("UTCoffset: "); Serial.println(configValues.UTCoffset);
  Serial.print("timeLapseHigh: "); Serial.print(configValues.timeLapseHigh);
  Serial.print(" - timeLapseDown: "); Serial.print(configValues.timeLapseDown);
  Serial.print(" - syncFreqMillis: "); Serial.println(configValues.syncFreqMillis);
  Serial.print("standByStartHour: "); Serial.print(configValues.standByStartHour);
  Serial.print(" - standByEndHour: "); Serial.println(configValues.standByEndHour);
  //Serial.print("standByEndMinute: ");Serial.println(configValues.standByEndMinute);

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


