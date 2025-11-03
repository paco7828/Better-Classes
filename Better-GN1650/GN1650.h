#pragma once

class GN1650 {
private:
  uint8_t DAT_PIN;
  uint8_t CLK_PIN;
  uint8_t numDigits;
  bool initialized = false;

  // System command
  static const uint8_t SYSTEM_CMD = 0x48;

  // Display control bits
  static const uint8_t DISP_ON = 0x01;
  static const uint8_t SEG_8 = 0x00;
  static const uint8_t WORK_MODE = 0x00;

  // Brightness
  static const uint8_t BRIGHTNESS_LEVELS = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x00 };

  // 7-segment digit patterns (common cathode only)
  const uint8_t segmentMap[11] = {
    0x3F,  // 0
    0x06,  // 1
    0x5B,  // 2
    0x4F,  // 3
    0x66,  // 4
    0x6D,  // 5
    0x7D,  // 6
    0x07,  // 7
    0x7F,  // 8
    0x6F,  // 9
    0x00   // 10 = blank
  };

  // Address mapping for up to 4 digits
  const uint8_t digitAddresses[4] = {
    0x68,  // DIG1
    0x6A,  // DIG2
    0x6C,  // DIG3
    0x6E   // DIG4
  };

  void startCondition() {
    digitalWrite(this->CLK_PIN, HIGH);
    digitalWrite(this->DAT_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(this->DAT_PIN, LOW);
    delayMicroseconds(10);
  }

  void stopCondition() {
    digitalWrite(this->CLK_PIN, LOW);
    digitalWrite(DAT_PIN, LOW);
    delayMicroseconds(10);
    digitalWrite(this->CLK_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(this->DAT_PIN, HIGH);
    delayMicroseconds(10);
  }

  void writeBit(bool bit) {
    digitalWrite(this->CLK_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(this->DAT_PIN, bit ? HIGH : LOW);
    delayMicroseconds(5);
    digitalWrite(this->CLK_PIN, HIGH);
    delayMicroseconds(5);
  }

  void writeByte(uint8_t data) {
    // Send 8 bits MSB first
    for (int i = 7; i >= 0; i--) {
      this->writeBit((data >> i) & 0x01);
    }

    // ACK bit (9th clock)
    digitalWrite(this->CLK_PIN, LOW);
    pinMode(this->DAT_PIN, INPUT);
    delayMicroseconds(5);
    digitalWrite(this->CLK_PIN, HIGH);
    delayMicroseconds(5);
    pinMode(this->DAT_PIN, OUTPUT);
    digitalWrite(this->CLK_PIN, LOW);
    delayMicroseconds(5);
  }

  void sendCommand(uint8_t cmd1, uint8_t cmd2) {
    this->startCondition();
    this->writeByte(cmd1);
    this->writeByte(cmd2);
    this->stopCondition();
    delayMicroseconds(100);
  }

  void writeDisplayData(uint8_t addr, uint8_t data) {
    this->startCondition();
    this->writeByte(addr);
    this->writeByte(data);
    this->stopCondition();
    delayMicroseconds(100);
  }

  void setDigit(uint8_t digitIndex, uint8_t value, bool decimalPoint = false) {
    if (digitIndex >= this->numDigits || digitIndex >= 4)
      return;

    uint8_t addr = this->digitAddresses[digitIndex];
    uint8_t dataByte;

    if (value <= 10) {
      dataByte = this->segmentMap[value];
      if (decimalPoint)
        dataByte |= 0x80;  // DP bit
    } else {
      dataByte = 0x00;  // Blank
    }

    this->writeDisplayData(addr, dataByte);
  }

public:
  void begin(uint8_t datPin, uint8_t clkPin, uint8_t digits, uint8_t brightness) {
    this->DAT_PIN = datPin;
    this->CLK_PIN = clkPin;
    if (digits > 4) {
      this->numDigits = 4;
    } else if (digits < 1) {
      this->numDigits = 1;
    }

    // Pin modes & default states
    pinMode(this->DAT_PIN, OUTPUT);
    pinMode(this->CLK_PIN, OUTPUT);
    digitalWrite(this->DAT_PIN, HIGH);
    digitalWrite(this->CLK_PIN, HIGH);

    delay(200);

    // Clear all RAM locations
    for (uint8_t i = 0; i < this->numDigits; i++) {
      this->writeDisplayData(this->digitAddresses[i], 0x00);
    }
    delay(10);

    // Enable display with brightness
    setBrightness(brightness);

    this->initialized = true;
  }

  void displayString(const char *str) {
    if (!this->initialized)
      return;

    int len = strlen(str);

    // Display from left to right
    for (int pos = 0; pos < this->numDigits; pos++) {
      if (pos < len) {
        char c = str[pos];

        // Handle digits 0-9
        if (c >= '0' && c <= '9') {
          this->setDigit(pos, c - '0');
        }
        // Handle space or dash as blank
        else if (c == ' ') {
          this->setDigit(pos, 10);  // blank
        } else {
          this->setDigit(pos, 10);  // blank for unknown characters
        }
      } else {
        this->setDigit(pos, 10);  // blank remaining digits
      }
    }
  }

  void clear() {
    if (!this->initialized)
      return;
    for (uint8_t i = 0; i < this->numDigits; i++) {
      this->writeDisplayData(this->digitAddresses[i], 0x00);
    }
  }

  void testSegments(uint16_t delayMs = 100) {
    if (!this->initialized)
      return;

    // Test each segment: A, B, C, D, E, F, G, DP
    for (int bit = 0; bit < 8; bit++) {
      uint8_t pattern = (1 << bit);
      for (uint8_t i = 0; i < this->numDigits; i++) {
        this->writeDisplayData(this->digitAddresses[i], pattern);
      }
      delay(delayMs);
    }
    clear();
  }

  void loading(uint16_t delayMs = 100) {
    if (!this->initialized)
      return;

    // Animate across all digits
    for (int digit = 0; digit < this->numDigits; digit++) {
      // Light up current segment on current digit
      this->writeDisplayData(this->digitAddresses[digit], SEG_DP);
      delay(delayMs);
    }

    for (int digit = this->numDigits - 1; digit >= 0; digit--) {
      // Light up current segment on current digit
      this->writeDisplayData(this->digitAddresses[digit], 0x00);
      delay(delayMs);
    }
  }

  void setBrightness(uint8_t level) {
    // Handle overflows
    if (level < 1)
      level = 1;
    if (level > 8)
      level = 8;

    this->sendCommand(this->SYSTEM_CMD, this->SEG_8 | this->WORK_MODE | this->BRIGHTNESS_LEVELS[level - 1] | this->DISP_ON);
    delay(10);
  }
};