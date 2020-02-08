#include<WiFi.h>

#define WIFI_AP_NAME "ESP32 AP mode"
#define WIFI_AP_PASS NULL

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_NAME, WIFI_AP_PASS);

  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() { }