#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <Time.h>

SoftwareSerial mySerial(5, 2);
Adafruit_GPS GPS(&mySerial);

#define GPSECHO  false // Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
#define PIN_CYCLE 4

#define CYCLING_MODE  1
#define STAND_BY_MODE 2

uint8_t currentMode;

uint32_t timer = 0;
const long unsigned timeToSync = 295000; // 290000 - 4' 55''
const boolean pulseGreaterThanSecond = true;

//const int unsigned powerOnPin = 5; // LED 'on'
const int unsigned waitingGPSPin = 7; //LED 'waiting'
const int unsigned SyncUTCPin = 8; // LED showing the sync

const uint8_t SATURDAY = 7;
const uint8_t SUNDAY = 1;

int standByStartHour = 19;
int standByStartMinute = 0;
int standByEndHour = 7;
int standByEndMinute = 0;

uint16_t UTCoffset = -3; // Diff with UTC

uint8_t volatile secondsAcumHigh = 0;
uint8_t volatile secondsAcumDown = 0;
boolean volatile validateStatusHigh = true;

const uint8_t timeLapseHigh = 4;
const uint8_t timeLapseDown = 1;


void setup()
{

  Serial.begin(115200);
  Serial.println("UTC sync POC (by aler) v1.0.4");

  initGPS();
  //digitalWrite(powerOnPin, HIGH);

  Serial.println("Waiting for GPS signal, pleas wait...");
  waitGPSSignal();
  digitalWrite(waitingGPSPin, HIGH);

  printInitialGPSData();

  syncUTC();

  setInitialTime();

  currentMode = CYCLING_MODE;
  
  Serial.println("End Setup.");
}



void loop()
{
  checkStatus();

  if ( currentMode == CYCLING_MODE &&
        pulseGreaterThanSecond && 
        (millis() - timer) >= timeToSync) 
      {
      detachInterrupt(1);
      digitalWrite(PIN_CYCLE, LOW);
      syncUTC();
      setInterrupt();
  }

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

void  setInterrupt() {
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

void checkStatus() {
  if ( (currentMode == CYCLING_MODE) &&
      ( weekday() == SATURDAY || 
        weekday() == SUNDAY ||
        (hour() == standByStartHour && minute() == standByStartMinute)
      )
     ) { // change to stand by mode, leave PIN as HIGH
      detachInterrupt(1);
      digitalWrite(PIN_CYCLE, HIGH);
      currentMode = STAND_BY_MODE;
      Serial.println("STAND BY mode...");
  }
  else if (
    (weekday() != SATURDAY && weekday() != SUNDAY) &&
    hour() == standByEndHour && minute() == standByEndMinute) { // Wake up from stand by
      digitalWrite(PIN_CYCLE, LOW);
      currentMode = CYCLING_MODE;
      setInterrupt();
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

  timer = millis();
  Serial.println("Synchronized successfully.");

}




