#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1500);

  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP("Ballie_Test", "12345678");

  Serial.println(ok ? "AP STARTED" : "AP FAILED");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  delay(1000);
}
