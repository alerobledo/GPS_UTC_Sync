void readComandRequest() {
  if (Serial.available() > 0)
  {
    detachInterrupt(1); //in order to read the command, which its suppossed to have more priority.
    
    char input[INPUT_SIZE + 1];
    byte size = Serial.readBytes(input, INPUT_SIZE); // read input
    input[size] = 0;
    char* command = strtok(input, " "); // split input

    String commandToExecute;
    commandToExecute = command;

    char* params[2];// store params if exists, up to 2 params
    command = strtok(NULL, " ");
    if (command != NULL) {
      params[0] = command;
      command = strtok(NULL, " ");
      if (command != NULL) {
        params[1] = command;
      }
    }

    if (params[0] != NULL) {
      Serial.print("param 1:"); Serial.println(params[0]);
    }
    if (params[1] != NULL) {
      Serial.print("param 2:"); Serial.println(params[1]);
    }

    Serial.print("command:"); Serial.println(commandToExecute);
    
    if (commandToExecute == "RESET") {
      detachInterrupt(1);
      digitalWrite(PIN_ON, LOW);
      digitalWrite(PIN_SYNC_UTC, LOW);
      digitalWrite(PIN_CYCLE, LOW);
      delay(5000);
      reset();
    }
    else if (commandToExecute == "FORCE-CYCLE") {
      checkStatusEnabled = false; // in order to not to change to "stand by" mode
      currentMode = CYCLING_MODE;
    }
    else if (commandToExecute == "STAND-BY-RANGE"){
      String p1 = params[0];
      String p2 = params[1];
      configValues.standByStartHour = p1.toInt();
      configValues.standByEndHour = p2.toInt();
      EEPROM.put(0, configValues);
    }
    else if (commandToExecute == "CYCLE"){
      String p1 = params[0];
      String p2 = params[1];
      configValues.timeLapseHigh = p1.toInt();
      configValues.timeLapseDown = p2.toInt();
      EEPROM.put(0, configValues);
      reset();
    }
    else if (commandToExecute == "STATUS") {
      printStatus();
    }

    //re enable cycle
    setCycleInterrupt();
  }
}


void reset() {
  wdt_enable(WDTO_15MS);
}

void printStatus(){
  Serial.println("Current Status: --------------------->>>");

  Serial.print(" initValuesConfigured: "); Serial.println(configValues.initValuesConfigured);
  Serial.print(" UTCoffset: "); Serial.println(configValues.UTCoffset);
  Serial.print(" timeLapseHigh: "); Serial.print(configValues.timeLapseHigh);
  Serial.print(" - timeLapseDown: "); Serial.print(configValues.timeLapseDown);
  Serial.print(" - syncFreqMillis: "); Serial.println(configValues.syncFreqMillis);
  Serial.print(" standByStartHour: "); Serial.print(configValues.standByStartHour);
  Serial.print(" - standByEndHour: "); Serial.println(configValues.standByEndHour);

  Serial.print(" current time: "); printCurrentTime();
  Serial.print(" isInCycleTimeRange: "); Serial.println(isInCycleTimeRange());
  Serial.print(" current mode: "); Serial.println(currentMode);
  Serial.print(" checkStatusEnabled: "); Serial.println(checkStatusEnabled);
}

