

void setInitialTime() {
  setTime(GPS.hour+configValues.UTCoffset, GPS.minute, GPS.seconds, GPS.day, GPS.month, GPS.year);
  Serial.print("Local date/time set to:");

  Serial.print(weekday()); Serial.print(' ');
  Serial.print(day()); Serial.print('/');
  Serial.print(month()); Serial.print('/');
  Serial.print(year()); Serial.print(" at ");

  Serial.print(hour()); Serial.print(':');
  Serial.print(minute()); Serial.print(':');
  Serial.print(second()); Serial.println('.');

}
