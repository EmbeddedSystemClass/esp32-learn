#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

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
int value = 0;

void setup_wifi();
void callback(char *topic, byte *message, unsigned int length);


void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    // default settings
    // (you can also pass in a Wire library object like &Wire2)

    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

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
        }
        else if (messageTemp == "off")
        {
            Serial.println("off");
            digitalWrite(ledPin, LOW);
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
        char tempString[8];
        dtostrf(temperature, 1, 2, tempString);
        Serial.print("Temperature: ");
        Serial.println(tempString);
        client.publish("esp32/temperature", tempString);

        humidity = 55;

        // Convert the value to a char array
        char humString[8];
        dtostrf(humidity, 1, 2, humString);
        Serial.print("Humidity: ");
        Serial.println(humString);
        client.publish("esp32/humidity", humString);
    }
}