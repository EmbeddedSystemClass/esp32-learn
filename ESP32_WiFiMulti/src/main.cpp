/*
 *  This sketch trys to Connect to the best AP based on a given list
 *
 */

#include <WiFi.h>
#include <WiFiMulti.h>

WiFiMulti wifiMulti;

void setup()
{
    Serial.begin(115200);
    delay(10);

    wifiMulti.addAP("Easy_AES", "your_password_for_AP_1");
    wifiMulti.addAP("Koson note", "agow7185");
    wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

    Serial.println("Connecting Wifi...");
    if(wifiMulti.run() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }
}

void loop()
{
    if(wifiMulti.run() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
        delay(1000);
    }
}