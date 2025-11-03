#pragma once

#include <SPI.h>
#include "pre-coded-matrices.h"

class BetterMAX7219 {
private:
  // Registers
  const uint8_t REG_NOOP = 0x00;
  const uint8_t REG_DIGIT0 = 0x01;  // Row 0
  const uint8_t REG_DIGIT1 = 0x02;  // Row 1
  const uint8_t REG_DIGIT2 = 0x03;  // Row 2
  const uint8_t REG_DIGIT3 = 0x04;  // Row 3
  const uint8_t REG_DIGIT4 = 0x05;  // Row 4
  const uint8_t REG_DIGIT5 = 0x06;  // Row 5
  const uint8_t REG_DIGIT6 = 0x07;  // Row 6
  const uint8_t REG_DIGIT7 = 0x08;  // Row 7
  const uint8_t REG_DECODEMODE = 0x09;
  const uint8_t REG_INTENSITY = 0x0A;
  const uint8_t REG_SCANLIMIT = 0x0B;
  const uint8_t REG_SHUTDOWN = 0x0C;
  const uint8_t REG_DISPLAYTEST = 0x0F;

  // Pins
  uint8_t DIN = -1;
  uint8_t CLK = -1;
  uint8_t CS = -1;

  void sendByte(uint8_t reg, uint8_t data) {
    digitalWrite(this->CS, LOW);
    SPI.transfer(reg);
    SPI.transfer(data);
    digitalWrite(this->CS, HIGH);
  }

public:
  void begin(uint8_t MAX7219_DIN, uint8_t MAX7219_CLK, uint8_t MAX7219_CS) {
    // Assign params
    this->DIN = MAX7219_DIN;
    this->CLK = MAX7219_CLK;
    this->CS = MAX7219_CS;

    // Pin modes & default states
    SPI.begin(this->CLK, -1, this->DIN, this->CS);
    pinMode(this->CS, OUTPUT);
    digitalWrite(this->CS, HIGH);

    // Initialize MAX7219
    sendByte(REG_SHUTDOWN, 0x00);     // Shutdown mode
    sendByte(REG_DISPLAYTEST, 0x00);  // No display test
    sendByte(REG_DECODEMODE, 0x00);   // No decode mode (we control each LED)
    sendByte(REG_SCANLIMIT, 0x07);    // Scan all 8 digits (rows 0-7)
    setBrightness();
    sendByte(REG_SHUTDOWN, 0x01);  // Normal operation

    // Safety clear
    clearDisplay();
  }

  void setBrightness(const uint8_t brightnessLevel = 8) {
    // Clamp brightness to valid range (0x00 to 0x0F)
    uint8_t level = brightnessLevel > 0x0F ? 0x0F : brightnessLevel;
    sendByte(REG_INTENSITY, level);
  }

  void displayMatrix(const uint8_t customMatrix[8][8]) {
    // Display matrix
    for (int row = 0; row < 8; row++) {
      byte rowData = 0;

      // Build the byte for this row from the matrix array
      if (customMatrix[row][0] == 1)
        rowData |= (1 << 6);  // Col0 -> SegA (bit 6)
      if (customMatrix[row][1] == 1)
        rowData |= (1 << 5);  // Col1 -> SegB (bit 5)
      if (customMatrix[row][2] == 1)
        rowData |= (1 << 4);  // Col2 -> SegC (bit 4)
      if (customMatrix[row][3] == 1)
        rowData |= (1 << 3);  // Col3 -> SegD (bit 3)
      if (customMatrix[row][4] == 1)
        rowData |= (1 << 2);  // Col4 -> SegE (bit 2)
      if (customMatrix[row][5] == 1)
        rowData |= (1 << 1);  // Col5 -> SegF (bit 1)
      if (customMatrix[row][6] == 1)
        rowData |= (1 << 0);  // Col6 -> SegG (bit 0)
      if (customMatrix[row][7] == 1)
        rowData |= (1 << 7);  // Col7 -> SegDP (bit 7)

      // Send the row data to the MAX7219
      this->sendByte(this->REG_DIGIT0 + row, rowData);
    }
  }

  void animateMatrix(const uint8_t matrix[8][8], const uint8_t scanType = 0, const unsigned long delayMs = 10) {
    uint8_t tempMatrix[8][8] = { 0 };

    if (scanType == 0) {
      // Row-by-row reveal (fill by rows)
      for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
          tempMatrix[i][j] = matrix[i][j];
          displayMatrix(tempMatrix);
          delay(delayMs);
        }
      }
    } else if (scanType == 1) {
      // Column-by-column reveal (fill by columns)
      for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 8; i++) {
          tempMatrix[i][j] = matrix[i][j];
          displayMatrix(tempMatrix);
          delay(delayMs);
        }
      }
    }
  }

  void displayNumber(uint8_t num){
    uint8_t correctNum = num;
    if(num < 0){
      correctNum = 0;
    }
    else if(num > 9){
      correctNum = 9;
    }
    displayMatrix(number_matrices[correctNum]);
  }

  void clearDisplay() {
    for (int i = this->REG_DIGIT0; i <= this->REG_DIGIT7; i++) {
      sendByte(i, 0x00);
    }
  }
};