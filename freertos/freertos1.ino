int LED = 2;
void setup()
{
    Serial.begin(112500);
    /* we create a new task here */
    xTaskCreate(
        additionalTask,    /* Task function. */
        "additional Task", /* name of task. */
        10000,             /* Stack size of task */
        NULL,              /* parameter of the task */
        1,                 /* priority of the task */
        NULL);             /* Task handle to keep track of created task */
    pinMode(LED, OUTPUT);
}

/* the forever loop() function is invoked by Arduino ESP32 loopTask */
void loop()
{
    Serial.println("this is Arduino Task");
    delay(1000);
}
/* this function will be invoked when additionalTask was created */
void additionalTask(void *parameter)
{
    /* loop forever */
    for (;;)
    {
        Serial.println("this is additional Task");
        digitalWrite(LED, HIGH); // turn the LED on (HIGH is the voltage level)
        delay(100);              // wait for a second
        digitalWrite(LED, LOW);  // turn the LED off by making the voltage LOW
        delay(900);              // wait for a second
    }
    /* delete a task when finish, 
  this will never happen because this is infinity loop */
    vTaskDelete(NULL);
}