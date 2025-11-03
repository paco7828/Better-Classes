#include "BetterHCSR04.h"

const uint8_t TRIG = 9;
const uint8_t ECHO = 10;

BetterHCSR04 hcsr;

void setup() {
    Serial.begin(115200);
    hcsr.begin(TRIG, ECHO);
}

void loop() {
    hcsr.update();

    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 200) {
        lastPrint = millis();
        Serial.println(hcsr.getDistance());
    }
}
