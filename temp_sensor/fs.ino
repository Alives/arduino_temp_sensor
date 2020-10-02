#include <FS.h>

String readFile(const String filename) {
  if (!SPIFFS.exists(filename)) {
    Serial.println(PSTR("File not found: ") + filename);
    return PSTR("");
  }
  File file = SPIFFS.open(filename, "r");
  String content = file.readString();
  file.close();
  Serial.println(PSTR("Read ") + filename + PSTR(": \"") +
                 content + PSTR("\""));
  return content;
}

void writeFile(const String filename, String content) {
  File file = SPIFFS.open(filename, "w");
  file.print(content);
  file.close();
  Serial.println(PSTR("Wrote ") + filename + PSTR(": \"") +
                 content + PSTR("\""));
}

void setupFS() {
  SPIFFS.begin();
}
