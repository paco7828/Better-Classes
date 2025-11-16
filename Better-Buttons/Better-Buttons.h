class Button {
private:
    // Pin
    uint8_t pin;
    
    // State
    bool inverted;
    bool lastState = HIGH;
    bool btnPressed = false;
    uint8_t pressCount = 0;

    // Time
    unsigned long pressStartTime = 0;
    unsigned long lastReleaseTime = 0;
    unsigned long currentTime = 0;

    // Interval
    unsigned long longPressTime;
    unsigned long multiPressTimeout;

public:
    // Callbacks
    void (*shortPressCallback)();
    void (*longPressCallback)();
    void (*multiPressCallback)(uint8_t);

    // Initialize
    void begin(uint8_t btnPin, bool invert = true, unsigned long longPress = 3000, unsigned long multiPress = 400) {
        this->pin = btnPin;
        this->inverted = invert;
        this->longPressTime = longPress;
        this->multiPressTimeout = multiPress;

        pinMode(pin, INPUT_PULLUP);

        shortPressCallback = nullptr;
        longPressCallback = nullptr;
        multiPressCallback = nullptr;
    }

    void update() {
        this->currentTime = millis();
        bool rawState = digitalRead(pin);
        bool btnState = this->inverted ? !rawState : rawState;

        // Button Pressed
        if (btnState == LOW && this->lastState == HIGH) {
            this->pressStartTime = this->currentTime;
            this->btnPressed = true;
        }

        // Button Released
        if (btnState == HIGH && this->lastState == LOW) {
            this->btnPressed = false;
            unsigned long pressTime = this->currentTime - this->pressStartTime;
            if (pressTime < this->longPressTime) {
                this->pressCount++;
                this->lastReleaseTime = this->currentTime;
            }
        }

        // Long press
        if (this->btnPressed && (this->currentTime - this->pressStartTime >= this->longPressTime)) {
            this->btnPressed = false;
            this->pressCount = 0;
            if (longPressCallback) longPressCallback();
        }

        // Multi-press
        if (this->pressCount > 0 && (this->currentTime - this->lastReleaseTime >= this->multiPressTimeout)) {
            if (multiPressCallback) multiPressCallback(this->pressCount);
            this->pressCount = 0;
        }

        this->lastState = btnState;
    }
};
