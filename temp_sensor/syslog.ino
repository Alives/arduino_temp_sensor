String log_str;

void syslog(const __FlashStringHelper* line) {
  syslog((const char *)line);
}

void syslog(String line) {
  syslog(line.c_str());
}

void syslog(const char* line) {
  char * entry = (char *) malloc((20 + strlen(line)) * sizeof(char));
  sprintf(entry, (const char *)F("[%0.2f] %s\n"), (millis() / 1000.0), line);
  Serial.print(entry);
  log_str += String(entry);
}

String getSyslog() {
  return log_str;
}

void clearSyslog() {
  log_str = F("");
}
