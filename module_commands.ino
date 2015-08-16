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
    if (commandToExecute == "START-CYCLE") {
      checkStatusEnabled = false; // in order to not to change to "stand by" mode
      currentMode = CYCLING_MODE;
      syncUTC();
      setCycleInterrupt();
    }
  }
}


void reset() {
  wdt_enable(WDTO_15MS);
}

