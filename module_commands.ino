void readComandRequest() {
  if (Serial.available() > 0)
  {
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

    if (commandToExecute == "RESET") {
      reset();
    }
    else if (commandToExecute == "FORCE-CYCLE") {
      checkStatusEnabled = false; // in order to not to change to "stand by" mode
      currentMode = CYCLING_MODE;
      syncUTC();
      setCycleInterrupt();
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
  }
}


void reset() {
  wdt_enable(WDTO_15MS);
}

