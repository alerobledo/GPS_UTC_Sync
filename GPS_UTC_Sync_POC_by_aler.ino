// http://www.engblaze.com/microcontroller-tutorial-avr-and-arduino-timer-interrupts/

#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(5, 2);
Adafruit_GPS GPS(&mySerial);

#define GPSECHO  false // Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console

uint32_t timer = 0;
const long unsigned timeToSync=290000; // 290000 - 4' 50''
const boolean pulseGreaterThanSecond = true;

//const int unsigned powerOnPin = 5; // LED 'on'
const int unsigned waitingGPSPin = 7; //LED 'waiting'
const int unsigned SyncUTCPin = 8; // LED showing the sync 

boolean usingInterrupt = false;// this keeps track of whether we're using the interrupt - off by default!
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

void setup()  
{
    
  Serial.begin(115200);
  Serial.println("UTC sync POC (by aler) v1.1.0");
 
  initGPS();
  //digitalWrite(powerOnPin, HIGH);
  
  Serial.println("Waiting for GPS signal, pleas wait...");
  waitGPSSignal();  
  digitalWrite(waitingGPSPin, HIGH);

  printInitialGPSData();
  
  syncUTC();

  if(pulseGreaterThanSecond){
    //attachInterrupt(1, sendPulseGreaterThanSecond, RISING);
          /*   Param 1 means the interrupts will check the PIN 3 (in Arduino UNO)
               RISING means interrupt will be thrown when PIN value change from LOW to HIGH.
               PIN 3 will be connected to PPS (GPS PIN module).
               It means interrupt wil be thrown at every second start.
          */
  }else{
    attachInterrupt(1, sendPulseLessThanSecond, RISING);
  }
  
  Serial.println("End Setup with GPS Signal.");
}

volatile unsigned int secondsAcumHigh =0;
volatile unsigned int secondsAcumDown =0;

volatile int statusToValidate =HIGH;

const unsigned int timeLapseHigh = 4;
const unsigned int timeLapseDown = 1;

ISR(TIMER1_COMPA_vect){

  Serial.println(digitalRead(4), DEC); 

  if(statusToValidate==HIGH){
      if(secondsAcumHigh<timeLapseHigh){
        digitalWrite(4, HIGH);
        ++secondsAcumHigh;
      }else{
        digitalWrite(4, LOW);
        statusToValidate=LOW;
        secondsAcumHigh =0;
        ++secondsAcumDown;
      }
  }else if(secondsAcumDown<timeLapseDown){
        ++secondsAcumDown;
   }else{
        digitalWrite(4, HIGH);
        ++secondsAcumHigh;
        secondsAcumDown =0;
        statusToValidate=HIGH;
   }
}


void sendPulseLessThanSecond(){
    Serial.println(1, DEC); 
    digitalWrite(4, HIGH);
    delay(200); // 4 seconds
    Serial.println(0, DEC); 
    digitalWrite(4, LOW);
    delay(800); // 1 second

}

void waitGPSSignal(){
  while(!GPS.fix){
  
    // if a sentence is received, we can check the checksum, parse it...
    if (GPS.newNMEAreceived()) {
    
      if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
        return;  // we can fail to parse a sentence in which case we should just wait for another
    }
  }
}

// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
}


void loop()
{
    
}



void syncUTC(){
    
    Serial.println("Synchronizing with second 0 from UTC...");

    while(true){ 
       if (GPS.newNMEAreceived()      && GPS.parse(GPS.lastNMEA())            && GPS.seconds==0 && GPS.milliseconds==0) 
            break;
    }


    // initialize Timer1
    cli();          // disable global interrupts
    TCCR1A = 0;     // set entire TCCR1A register to 0
    TCCR1B = 0;     // same for TCCR1B
 
    // set compare match register to desired timer count:
    OCR1A = 15624;
    // turn on CTC mode:
    TCCR1B |= (1 << WGM12);
    // Set CS10 and CS12 bits for 1024 prescaler:
    TCCR1B |= (1 << CS10);
    TCCR1B |= (1 << CS12);
    // enable timer compare interrupt:
    TIMSK1 |= (1 << OCIE1A);
    sei();          // enable global interrupts

    
    Serial.println("Synchronized with second 0 from UTC.");

}


void printInitialGPSData(){
  Serial.print("\n********* Time: ");
  Serial.print(GPS.hour, DEC); Serial.print(':');
  Serial.print(GPS.minute, DEC); Serial.print(':');
  Serial.print(GPS.seconds, DEC); Serial.print('.');
  Serial.println(GPS.milliseconds);
  Serial.print("Date: ");
  Serial.print(GPS.day, DEC); Serial.print('/');
  Serial.print(GPS.month, DEC); Serial.print("/20");
  Serial.println(GPS.year, DEC);
  Serial.print(" quality: "); Serial.println((int)GPS.fixquality); 

  Serial.print("Location: ");
  Serial.print(GPS.latitude, 4); Serial.print(GPS.lat);
  Serial.print(", "); 
  Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);  
  Serial.print("Satellites: "); Serial.println((int)GPS.satellites);
}

void initGPS(){
  GPS.begin(9600);
  
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);

  // the nice thing about this code is you can have a timer0 interrupt go off
  // every 1 millisecond, and read data from the GPS for you. that makes the
  // loop code a heck of a lot easier!
  useInterrupt(true);
  
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

