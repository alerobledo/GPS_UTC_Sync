boolean usingInterrupt = false;// this keeps track of whether we're using the interrupt - off by default!
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

void waitGPSFix() {
  while(GPS.fix!=1) {
      if (GPS.newNMEAreceived()) {  
        if (!GPS.parse(GPS.lastNMEA()))
          continue; 
      }
      delay(500);   
   }
}

void initGPS() {
  GPS.begin(9600);

  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  GPS.sendCommand(PGCMD_ANTENNA);

  useInterrupt(true); //TODO: ver de pasarla a false una vez que fixea

}


// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  GPS.read();
}

void useInterrupt(boolean v) {
  Serial.println("useInterrupt enter");
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

