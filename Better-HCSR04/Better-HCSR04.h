class BetterHCSR04
{
private:
    const unsigned long TRIGGER_PULSE_INTERVAL = 60000UL;
    const unsigned long ECHO_TIMEOUT = 30000UL;
    uint8_t TRIG = 0;
    uint8_t ECHO = 0;
    unsigned long echoStart = 0;
    unsigned long lastTrigger = 0;
    float distance = 0;

public:
    void begin(uint8_t trigPin, uint8_t echoPin)
    {
        this->TRIG = trigPin;
        this->ECHO = echoPin;
        pinMode(this->TRIG, OUTPUT);
        pinMode(this->ECHO, INPUT);
        digitalWrite(this->TRIG, LOW);
    }

    void update()
    {
        unsigned long now = micros();

        // Send trigger pulse
        if (now - this->lastTrigger >= this->TRIGGER_PULSE_INTERVAL)
        {
            this->lastTrigger = now;
            digitalWrite(this->TRIG, HIGH);
            delayMicroseconds(10);
            digitalWrite(this->TRIG, LOW);
        }

        // Rising edge
        if (digitalRead(this->ECHO) == HIGH && this->echoStart == 0)
        {
            this->echoStart = now;
        }

        // Falling edge
        if (this->echoStart > 0 && digitalRead(this->ECHO) == LOW)
        {
            unsigned long duration = micros() - this->echoStart;
            this->echoStart = 0;
            this->distance = (duration * 0.0343f) / 2.0f;
        }

        // Timeout safety
        if (this->echoStart > 0 && (now - this->echoStart) > this->ECHO_TIMEOUT)
        {
            this->echoStart = 0;
        }
    }

    float getDistance()
    {
        return this->distance;
    }
};
