#include "Better-Buttons.h"

Button btn;

void shortPressFunc()
{
    Serial.println("Short press detected");
}

void longPressFunc()
{
    Serial.println("Long press detected");
}

void multiPressFunc(uint8_t count)
{
    Serial.print("Multi-press detected: ");
    Serial.println(count);
}

void setup()
{
    Serial.begin(115200);
    btn.begin(8);

    // Assign callbacks
    btn.shortPressCallback = shortPressFunc;
    btn.longPressCallback = longPressFunc;
    btn.multiPressCallback = multiPressFunc;
}

void loop()
{
    // Always call update in loop
    btn.update();
}
