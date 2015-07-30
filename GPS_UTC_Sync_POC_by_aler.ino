#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(5, 2);
Adafruit_GPS GPS(&mySerial);

#define GPSECHO  false // Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console

uint32_t timer = 0;
const long unsigned timeToSync=115000; // 290000 - 4' 50''

const int unsigned powerOnPin = 5; // LED 'on'
const int unsigned waitingGPSPin = 7; //LED 'waiting'
const int unsigned SyncUTCPin = 8; // LED showing the sync 

boolean usingInterrupt = false;// this keeps track of whether we're using the interrupt - off by default!
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

void setup()  
{
    
  Serial.begin(115200);
  Serial.println("UTC sync POC (by aler) v1.0.2");

  initGPS();
  digitalWrite(powerOnPin, HIGH);
  
  Serial.println("Waiting for GPS signal, pleas wait...");
  waitGPSSignal();  
  digitalWrite(waitingGPSPin, HIGH);

  printInitialGPSData();
  
  syncUTC();
  
  attachInterrupt(1, sendPulse, RISING);
        /*   Param 1 means the interrupts will check the PIN 3 (in Arduino UNO)
             RISING means interrupt will be thrown when PIN value change from LOW to HIGH.
             PIN 3 will be connected to PPS (GPS PIN module).
             It means interrupt wil be thrown at every second start.
        */
  Serial.println("End Setup with GPS Signal.");
}

unsigned int secondsAcumHigh =0;
unsigned int secondsAcumDown =0;

boolean validateStatusHigh =true;

const unsigned int timeLapseHigh = 4;
const unsigned int timeLapseDown = 1;

void sendPulse(){  

  if(validateStatusHigh){
      if(secondsAcumHigh<timeLapseHigh){
        Serial.println(1, DEC); 
        digitalWrite(4, HIGH);
        secondsAcumHigh++;
      }else{
        validateStatusHigh=false;
        secondsAcumHigh =0;
      }
  }else if(secondsAcumDown<timeLapseDown){
        Serial.println(0, DEC); 
        digitalWrite(4, LOW); 
        secondsAcumDown++;
   }else{
        validateStatusHigh=true;
        secondsAcumDown =0;
   }
}

/*
void initASecond(){
    Serial.println(1, DEC); 
    digitalWrite(4, HIGH);
    delay(200); // 4 seconds
    Serial.println(0, DEC); 
    digitalWrite(4, LOW);
    delay(800); // 1 second

}
*/
void waitGPSSignal(){
  while(!GPS.fix){
  
    // if a sentence is received, we can check the checksum, parse it...
    if (GPS.newNMEAreceived()) {
      // a tricky thing here is if we print the NMEA sentence, or data
      // we end up not listening and catching other sentences! 
      // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
      //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false
    
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

    if((millis()-timer)>=timeToSync){ 
      syncUTC();
    }
    
}



void syncUTC(){
    noInterrupts();
    Serial.print("Synchronizing with second 0 from UTC...at:");

    while(true){ 
       if (GPS.newNMEAreceived() 
            && GPS.parse(GPS.lastNMEA()
            && GPS.seconds==0 
            && GPS.milliseconds==0){
            break;
      }
    }
    timer =millis();
    Serial.print("Synchronized with second 0 from UTC.");
    interrupts();
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
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  
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
  Serial.println("useInterrupt");
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

