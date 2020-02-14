#include <Arduino.h>
// Potentiometer is connected to GPIO 34 (Analog ADC1_CH6)
const int potPin = 34;
const int TempPin = 34;
const int HumidPin = 35;

// variable for storing the potentiometer value
int TempValue = 0;
int HumidValue = 0;

void setup()
{
    Serial.begin(115200);
    delay(1000);
}

void loop()
{
    // Reading potentiometer value
    TempValue = analogRead(TempPin) / 40.96;
    Serial.print("Temp : ");
    Serial.print(TempValue);
    HumidValue = analogRead(HumidPin) / 40.96;
    Serial.print("Humid : ");
    Serial.println(HumidValue);

    delay(500);
}