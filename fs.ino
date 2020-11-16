#include <FS.h>

String readFile(const String filename) {
  if (!SPIFFS.exists(filename)) {
    Serial.println("File not found: " + filename);
    return "";
  }
  File file = SPIFFS.open(filename, "r");
  String content = file.readString();
  file.close();
  Serial.println("Read " + filename + ": \"" +
                 content + "\"");
  return content;
}

void writeFile(const String filename, String content) {
  File file = SPIFFS.open(filename, "w");
  file.print(content);
  file.close();
  Serial.println("Wrote " + filename + ": \"" + content + "\"");
}

void setupFS() {
  SPIFFS.begin();
}
