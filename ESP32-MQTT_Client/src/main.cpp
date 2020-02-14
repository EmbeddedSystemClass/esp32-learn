#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <PubSubClient.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// Replace the next variables with your SSID/Password combination
const char *ssid = "Easy_AES";
const char *password = "29062007";

// Add your MQTT Broker IP address, example:
//const char* mqtt_server = "192.168.1.144";
const char *mqtt_server = "172.104.35.200";

// LED Pin
const int ledPin = 2;
float temperature = 0;
float humidity = 0;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
char tempString[8];
char humString[8];
char ledString[8];

int value = 0;

void setup_wifi();
void callback(char *topic, byte *message, unsigned int length);

TaskHandle_t hMQQT_Task = NULL;
TaskHandle_t hOLED_Task = NULL;
char m_str[3];
WiFiMulti wifiMulti;

void OLED_Task(void *pvParam)
{
    uint8_t m = 24;
    uint8_t nextpos = 0;
    U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
    u8g2.begin();
    while (1)
    {
        // update OLED

        strcpy(m_str, u8x8_u8toa(m, 2)); /* convert m to a string with two digits */
        u8g2.firstPage();
        do
        {
            // u8g2.setFont(u8g2_font_logisoso42_tn);
            u8g2.setFont(u8g2_font_8x13_me);
            u8g2.drawStr(0, 15, ">");
            u8g2.drawStr(10, 15, mqtt_server);
            nextpos = u8g2.drawStr(0, 30, "LED: ");
            u8g2.drawStr(nextpos, 30, ledString);
            nextpos = u8g2.drawStr(0, 45, "Te : ");
            u8g2.drawStr(nextpos, 45, tempString);
            nextpos = u8g2.drawStr(0, 60, "Hu : ");
            u8g2.drawStr(nextpos, 60, humString);

        } while (u8g2.nextPage());
        delay(100);
        m++;

        if (m == 60)
            m = 0;
    }
}

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    // default settings
    // (you can also pass in a Wire library object like &Wire2)

    //setup_wifi();

    delay(10);

    wifiMulti.addAP("Easy_AES", "29062007");
    wifiMulti.addAP("Koson note", "agow7185");
    wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

    Serial.println("Connecting Wifi...");
    if (wifiMulti.run() == WL_CONNECTED)
    {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }

    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    xTaskCreate(
        OLED_Task,
        "OLED Task",
        3000,
        NULL,
        1,
        &hOLED_Task);

    pinMode(ledPin, OUTPUT);
}

void setup_wifi()
{
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *message, unsigned int length)
{
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++)
    {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println();

    // Feel free to add more if statements to control more GPIOs with MQTT

    // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
    // Changes the output state according to the message
    if (String(topic) == "esp32/output")
    {
        Serial.print("Changing output to ");
        if (messageTemp == "on")
        {
            Serial.println("on");
            digitalWrite(ledPin, HIGH);
            strcpy(ledString, "ON");
        }
        else if (messageTemp == "off")
        {
            Serial.println("off");
            digitalWrite(ledPin, LOW);
            strcpy(ledString, "OFF");
        }
    }
}

void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("ESP8266Client"))
        {
            Serial.println("connected");
            // Subscribe
            client.subscribe("esp32/output");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    long now = millis();
    if (now - lastMsg > 5000)
    {
        lastMsg = now;

        // Temperature in Celsius
        temperature = 29;
        // Uncomment the next line to set temperature in Fahrenheit
        // (and comment the previous temperature line)
        //temperature = 1.8 * bme.readTemperature() + 32; // Temperature in Fahrenheit

        // Convert the value to a char array
        dtostrf(temperature, 1, 2, tempString);
        Serial.print("Temperature: ");
        Serial.println(tempString);
        client.publish("esp32/temperature", tempString);

        humidity = 55;

        // Convert the value to a char array
        dtostrf(humidity, 1, 2, humString);
        Serial.print("Humidity: ");
        Serial.println(humString);
        client.publish("esp32/humidity", humString);
    }
}