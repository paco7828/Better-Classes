const uint8_t OK_BTN = 8;
const bool INVERTED = true;
bool lastBtnState = HIGH;
bool btnPressed = false;

unsigned long pressStartTime = 0;
unsigned long lastReleaseTime = 0;
unsigned long currentTime = 0;

const unsigned long LONG_PRESS_TIME = 3000;
const unsigned long MULTI_PRESS_TIMEOUT = 400;
uint8_t pressCount = 0;

void setup(){
    Serial.begin(9600);
    pinMode(OK_BTN, INPUT_PULLUP);
}

void loop(){
    currentTime = millis();
    bool rawState = digitalRead(OK_BTN);
    bool btnState = INVERTED ? !rawState : rawState;

    // ---- Button Pressed ----
    if(btnState == LOW && lastBtnState == HIGH){  
        pressStartTime = currentTime;
        btnPressed = true;
    }

    // ---- Button Released ----
    if(btnState == HIGH && lastBtnState == LOW){
        btnPressed = false;
        unsigned long pressTime = currentTime - pressStartTime;

        if(pressTime < LONG_PRESS_TIME){
            pressCount++;
            lastReleaseTime = currentTime;
        }
    }

    // ---- Long press detection ----
    if(btnPressed && (currentTime - pressStartTime >= LONG_PRESS_TIME)){
        btnPressed = false;  // avoid repeated triggers
        pressCount = 0;      // long press is not counted as multi-press
        onLongPress();
    }

    // ---- Multi-press detection ----
    if(pressCount > 0 && (currentTime - lastReleaseTime >= MULTI_PRESS_TIMEOUT)){
        onMultiPress(pressCount);
        pressCount = 0;
    }

    lastBtnState = btnState;
}


// ====== CALLBACKS ======

void onShortPress(){
    Serial.println("Short press detected");
}

void onLongPress(){
    Serial.println("Long press (3s) detected");
}

void onMultiPress(uint8_t count){
    if(count == 1){
        onShortPress();
    } else {
        Serial.print("Multi-press detected: ");
        Serial.print(count);
        Serial.println(" clicks");
    }
}
