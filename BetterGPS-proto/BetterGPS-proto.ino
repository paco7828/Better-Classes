#include "Better-GPS-commands.h"

// ==================== CONFIGURATION ====================
#define GPS_RX_PIN 6
#define GPS_TX_PIN 10
#define GPS_BAUD 38400
#define DEBUG_SERIAL Serial
#define RESPONSE_TIMEOUT 2000

HardwareSerial GPS_SERIAL(1);

// ==================== BUFFER MANAGEMENT ====================
#define BUFFER_SIZE 512
uint8_t rxBuffer[BUFFER_SIZE];
uint16_t bufferIndex = 0;

bool monitorMode = false;
bool rawHexMode = false;

// ==================== UBX PACKET STRUCTURE ====================
struct UBX_Packet {
  uint8_t cls;
  uint8_t id;
  uint16_t len;
  uint8_t payload[BUFFER_SIZE - 8];
  uint8_t checksumA;
  uint8_t checksumB;
};

// ==================== SETUP ====================
void setup() {
  DEBUG_SERIAL.begin(115200);
  GPS_SERIAL.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  
  delay(1000);
  DEBUG_SERIAL.println(F("\n=== GPS Diagnostic & UBX Tester ==="));
  DEBUG_SERIAL.println(F("HTS18 GPS Module"));
  DEBUG_SERIAL.print(F("RX Pin: ")); DEBUG_SERIAL.print(GPS_RX_PIN);
  DEBUG_SERIAL.print(F(" | TX Pin: ")); DEBUG_SERIAL.print(GPS_TX_PIN);
  DEBUG_SERIAL.print(F(" | Baud: ")); DEBUG_SERIAL.println(GPS_BAUD);
  
  DEBUG_SERIAL.println(F("\nDiagnostic Commands:"));
  DEBUG_SERIAL.println(F("  M - Toggle NMEA monitor (see raw GPS output)"));
  DEBUG_SERIAL.println(F("  X - Toggle hex mode (see raw bytes)"));
  DEBUG_SERIAL.println(F("  B - Try different baud rates"));
  DEBUG_SERIAL.println(F("  D - Disable all NMEA messages"));
  DEBUG_SERIAL.println(F("  ? - Show full menu"));
  
  printMenu();
}

// ==================== MAIN LOOP ====================
void loop() {
  if (DEBUG_SERIAL.available()) {
    char cmd = DEBUG_SERIAL.read();
    
    if (cmd == 'M' || cmd == 'm') {
      monitorMode = !monitorMode;
      DEBUG_SERIAL.print(F("\n>>> Monitor mode: "));
      DEBUG_SERIAL.println(monitorMode ? F("ON") : F("OFF"));
      if (monitorMode) {
        DEBUG_SERIAL.println(F("(Press M again to stop)"));
      }
      return;
    }
    
    if (cmd == 'X' || cmd == 'x') {
      rawHexMode = !rawHexMode;
      DEBUG_SERIAL.print(F("\n>>> Raw hex mode: "));
      DEBUG_SERIAL.println(rawHexMode ? F("ON") : F("OFF"));
      return;
    }
    
    if (!monitorMode) {
      handleCommand(cmd);
    }
  }
  
  // Monitor GPS output
  if (monitorMode || rawHexMode) {
    while (GPS_SERIAL.available()) {
      uint8_t c = GPS_SERIAL.read();
      if (rawHexMode) {
        if (c < 0x10) DEBUG_SERIAL.print(F("0"));
        DEBUG_SERIAL.print(c, HEX);
        DEBUG_SERIAL.print(F(" "));
      } else {
        DEBUG_SERIAL.write(c);
      }
    }
  } else {
    // Silently drain buffer
    while (GPS_SERIAL.available()) {
      GPS_SERIAL.read();
    }
  }
}

// ==================== COMMAND HANDLER ====================
void handleCommand(char cmd) {
  DEBUG_SERIAL.print(F("\n>>> Command: "));
  DEBUG_SERIAL.println(cmd);
  
  switch (cmd) {
    // Diagnostic commands
    case 'B': case 'b': tryBaudRates(); break;
    case 'D': case 'd': disableAllNMEA(); break;
    
    // Poll commands
    case '1': sendAndReceive(UBX_POLL_POSLLH, sizeof(UBX_POLL_POSLLH), "NAV-POSLLH"); break;
    case '2': sendAndReceive(UBX_POLL_STATUS, sizeof(UBX_POLL_STATUS), "NAV-STATUS"); break;
    case '3': sendAndReceive(UBX_POLL_SAT, sizeof(UBX_POLL_SAT), "NAV-SAT"); break;
    case '4': sendAndReceive(UBX_POLL_VERSION, sizeof(UBX_POLL_VERSION), "MON-VER"); break;
    case '5': sendAndReceive(UBX_POLL_DOP, sizeof(UBX_POLL_DOP), "NAV-DOP"); break;
    case '6': sendAndReceive(UBX_POLL_PVT, sizeof(UBX_POLL_PVT), "NAV-PVT"); break;
    case '7': sendAndReceive(UBX_POLL_ODO, sizeof(UBX_POLL_ODO), "NAV-ODO"); break;
    case '8': sendAndReceive(UBX_POLL_VELNED, sizeof(UBX_POLL_VELNED), "NAV-VELNED"); break;
    case '9': sendAndReceive(UBX_POLL_TIMEUTC, sizeof(UBX_POLL_TIMEUTC), "NAV-TIMEUTC"); break;
    case 'a': sendAndReceive(UBX_POLL_RAWX, sizeof(UBX_POLL_RAWX), "RXM-RAWX"); break;
    case 'c': sendAndReceive(UBX_POLL_HW, sizeof(UBX_POLL_HW), "MON-HW"); break;
    
    // Rate commands
    case 'r': sendAndReceive(UBX_RATE_1HZ, sizeof(UBX_RATE_1HZ), "RATE 1Hz"); break;
    case 't': sendAndReceive(UBX_RATE_5HZ, sizeof(UBX_RATE_5HZ), "RATE 5Hz"); break;
    case 'y': sendAndReceive(UBX_RATE_10HZ, sizeof(UBX_RATE_10HZ), "RATE 10Hz"); break;
    
    // Power modes
    case 'p': sendAndReceive(UBX_POWER_CONTINUOUS, sizeof(UBX_POWER_CONTINUOUS), "POWER CONTINUOUS"); break;
    case 'o': sendAndReceive(UBX_POWER_SAVE, sizeof(UBX_POWER_SAVE), "POWER SAVE"); break;
    
    // Save commands
    case 's': sendAndReceive(UBX_SAVE_FLASH, sizeof(UBX_SAVE_FLASH), "SAVE FLASH"); break;
    case 'f': sendAndReceive(UBX_SAVE_ALL, sizeof(UBX_SAVE_ALL), "SAVE ALL"); break;
    
    // NMEA disable
    case 'g': sendAndReceive(UBX_DISABLE_GGA, sizeof(UBX_DISABLE_GGA), "DISABLE GGA"); break;
    case 'h': sendAndReceive(UBX_DISABLE_GLL, sizeof(UBX_DISABLE_GLL), "DISABLE GLL"); break;
    case 'j': sendAndReceive(UBX_DISABLE_GSA, sizeof(UBX_DISABLE_GSA), "DISABLE GSA"); break;
    case 'k': sendAndReceive(UBX_DISABLE_GSV, sizeof(UBX_DISABLE_GSV), "DISABLE GSV"); break;
    case 'l': sendAndReceive(UBX_DISABLE_RMC, sizeof(UBX_DISABLE_RMC), "DISABLE RMC"); break;
    case 'z': sendAndReceive(UBX_DISABLE_VTG, sizeof(UBX_DISABLE_VTG), "DISABLE VTG"); break;
    
    // GNSS config
    case 'n': sendAndReceive(UBX_GNSS_GPS_GALILEO, sizeof(UBX_GNSS_GPS_GALILEO), "GNSS GPS+Galileo"); break;
    case 'm': sendAndReceive(UBX_GNSS_ALL, sizeof(UBX_GNSS_ALL), "GNSS ALL"); break;
    case 'v': sendAndReceive(UBX_GNSS_GPS_ONLY, sizeof(UBX_GNSS_GPS_ONLY), "GNSS GPS Only"); break;
    
    // Navigation modes
    case 'q': sendAndReceive(UBX_NAV5_PORTABLE, sizeof(UBX_NAV5_PORTABLE), "NAV5 PORTABLE"); break;
    case 'w': sendAndReceive(UBX_NAV5_STATIONARY, sizeof(UBX_NAV5_STATIONARY), "NAV5 STATIONARY"); break;
    case 'e': sendAndReceive(UBX_NAV5_PEDESTRIAN, sizeof(UBX_NAV5_PEDESTRIAN), "NAV5 PEDESTRIAN"); break;
    case 'u': sendAndReceive(UBX_NAV5_AUTOMOTIVE, sizeof(UBX_NAV5_AUTOMOTIVE), "NAV5 AUTOMOTIVE"); break;
    
    // Smoothing
    case 'i': sendAndReceive(UBX_NO_SMOOTHING, sizeof(UBX_NO_SMOOTHING), "NO SMOOTHING"); break;
    
    // Reset commands
    case 'H': 
      DEBUG_SERIAL.println(F("Sending HOT RESET"));
      sendCommand(UBX_RESET_HOT, sizeof(UBX_RESET_HOT));
      delay(3000);
      break;
    case 'W': 
      DEBUG_SERIAL.println(F("Sending WARM RESET"));
      sendCommand(UBX_RESET_WARM, sizeof(UBX_RESET_WARM));
      delay(5000);
      break;
    case 'C': 
      DEBUG_SERIAL.println(F("Sending COLD RESET"));
      sendCommand(UBX_RESET_COLD, sizeof(UBX_RESET_COLD));
      delay(10000);
      break;
    
    case '?': printMenu(); break;
    
    default:
      DEBUG_SERIAL.println(F("Unknown command. Press '?' for menu."));
      break;
  }
}

// ==================== DIAGNOSTIC: TRY BAUD RATES ====================
void tryBaudRates() {
  uint32_t rates[] = {4800, 9600, 19200, 38400, 57600, 115200};
  
  DEBUG_SERIAL.println(F("\nTesting baud rates..."));
  
  for (int i = 0; i < 6; i++) {
    DEBUG_SERIAL.print(F("Trying ")); 
    DEBUG_SERIAL.print(rates[i]);
    DEBUG_SERIAL.print(F(" baud... "));
    
    GPS_SERIAL.updateBaudRate(rates[i]);
    delay(100);
    
    // Clear buffer
    while (GPS_SERIAL.available()) GPS_SERIAL.read();
    
    // Wait for data
    unsigned long start = millis();
    bool gotData = false;
    while (millis() - start < 1000) {
      if (GPS_SERIAL.available()) {
        gotData = true;
        break;
      }
    }
    
    if (gotData) {
      DEBUG_SERIAL.print(F("✓ DATA! "));
      
      // Show first few characters
      int count = 0;
      while (GPS_SERIAL.available() && count < 40) {
        char c = GPS_SERIAL.read();
        if (c >= 32 && c <= 126) DEBUG_SERIAL.write(c);
        else DEBUG_SERIAL.print(F("."));
        count++;
      }
      DEBUG_SERIAL.println();
      
      // Try to get a UBX response
      sendCommand(UBX_POLL_VERSION, sizeof(UBX_POLL_VERSION));
      delay(200);
      
      UBX_Packet pkt;
      if (receiveUBXPacket(&pkt)) {
        DEBUG_SERIAL.println(F("  ✓ UBX SUPPORTED!"));
      } else {
        DEBUG_SERIAL.println(F("  ✗ No UBX response (NMEA only?)"));
      }
    } else {
      DEBUG_SERIAL.println(F("✗ No data"));
    }
    
    // Clear buffer
    while (GPS_SERIAL.available()) GPS_SERIAL.read();
  }
  
  // Restore original baud
  GPS_SERIAL.updateBaudRate(GPS_BAUD);
  DEBUG_SERIAL.println(F("\nRestore to 9600 baud"));
}

// ==================== DIAGNOSTIC: DISABLE ALL NMEA ====================
void disableAllNMEA() {
  DEBUG_SERIAL.println(F("Disabling all NMEA messages..."));
  
  const uint8_t* cmds[] = {
    UBX_DISABLE_GGA, UBX_DISABLE_GLL, UBX_DISABLE_GSA,
    UBX_DISABLE_GSV, UBX_DISABLE_RMC, UBX_DISABLE_VTG
  };
  const char* names[] = {"GGA", "GLL", "GSA", "GSV", "RMC", "VTG"};
  
  for (int i = 0; i < 6; i++) {
    while (GPS_SERIAL.available()) GPS_SERIAL.read();
    
    DEBUG_SERIAL.print(F("  ")); 
    DEBUG_SERIAL.print(names[i]);
    DEBUG_SERIAL.print(F("... "));
    
    sendCommand(cmds[i], 16);
    delay(100);
    
    UBX_Packet pkt;
    if (receiveUBXPacket(&pkt) && pkt.cls == 0x05) {
      if (pkt.id == 0x01) {
        DEBUG_SERIAL.println(F("✓"));
      } else {
        DEBUG_SERIAL.println(F("✗ NAK"));
      }
    } else {
      DEBUG_SERIAL.println(F("✗ No ACK"));
    }
  }
  
  DEBUG_SERIAL.println(F("\nSaving to flash..."));
  sendCommand(UBX_SAVE_FLASH, sizeof(UBX_SAVE_FLASH));
  delay(500);
  
  DEBUG_SERIAL.println(F("Done! GPS should now be quiet."));
}

// ==================== SEND AND RECEIVE ====================
void sendAndReceive(const uint8_t* cmd, size_t len, const char* name) {
  while (GPS_SERIAL.available()) GPS_SERIAL.read();
  
  DEBUG_SERIAL.print(F("Sending: "));
  DEBUG_SERIAL.print(name);
  DEBUG_SERIAL.print(F(" ... "));
  
  sendCommand(cmd, len);
  delay(100);
  
  UBX_Packet packet;
  if (receiveUBXPacket(&packet)) {
    DEBUG_SERIAL.println(F("✓"));
    parseResponse(&packet, name);
  } else {
    DEBUG_SERIAL.println(F("✗ No response"));
    DEBUG_SERIAL.println(F("  Hint: Try 'M' to monitor GPS output"));
    DEBUG_SERIAL.println(F("        Try 'B' to test baud rates"));
  }
  
  DEBUG_SERIAL.println();
}

// ==================== SEND COMMAND ====================
void sendCommand(const uint8_t* cmd, size_t len) {
  GPS_SERIAL.write(cmd, len);
  GPS_SERIAL.flush();
}

// ==================== RECEIVE UBX PACKET ====================
bool receiveUBXPacket(UBX_Packet* packet) {
  unsigned long startTime = millis();
  uint8_t state = 0;
  
  while (millis() - startTime < RESPONSE_TIMEOUT) {
    if (GPS_SERIAL.available()) {
      uint8_t c = GPS_SERIAL.read();
      
      switch (state) {
        case 0: if (c == 0xB5) state = 1; break;
        case 1: if (c == 0x62) state = 2; else state = 0; break;
        case 2: packet->cls = c; state = 3; break;
        case 3: packet->id = c; state = 4; break;
        case 4: packet->len = c; state = 5; break;
        case 5: packet->len |= (c << 8); bufferIndex = 0; state = 6; break;
        case 6:
          if (bufferIndex < packet->len && bufferIndex < BUFFER_SIZE - 8) {
            packet->payload[bufferIndex++] = c;
            if (bufferIndex == packet->len) state = 7;
          } else {
            state = 0;
          }
          break;
        case 7: packet->checksumA = c; state = 8; break;
        case 8: packet->checksumB = c; return verifyChecksum(packet);
        default: state = 0;
      }
    }
  }
  return false;
}

// ==================== VERIFY CHECKSUM ====================
bool verifyChecksum(UBX_Packet* packet) {
  uint8_t ckA = 0, ckB = 0;
  
  ckA += packet->cls; ckB += ckA;
  ckA += packet->id; ckB += ckA;
  ckA += (packet->len & 0xFF); ckB += ckA;
  ckA += (packet->len >> 8); ckB += ckA;
  
  for (uint16_t i = 0; i < packet->len; i++) {
    ckA += packet->payload[i];
    ckB += ckA;
  }
  
  return (ckA == packet->checksumA && ckB == packet->checksumB);
}

// ==================== PARSE RESPONSE ====================
void parseResponse(UBX_Packet* packet, const char* cmdName) {
  if (packet->cls == 0x05) {
    if (packet->id == 0x01) {
      DEBUG_SERIAL.println(F("  ✓ ACK-ACK"));
    } else if (packet->id == 0x00) {
      DEBUG_SERIAL.println(F("  ✗ ACK-NAK (rejected)"));
    }
    return;
  }
  
  if (packet->cls == 0x01 && packet->id == 0x02) parseNavPosllh(packet);
  else if (packet->cls == 0x01 && packet->id == 0x03) parseNavStatus(packet);
  else if (packet->cls == 0x01 && packet->id == 0x35) parseNavSat(packet);
  else if (packet->cls == 0x0A && packet->id == 0x04) parseMonVer(packet);
  else if (packet->cls == 0x01 && packet->id == 0x04) parseNavDop(packet);
  else if (packet->cls == 0x01 && packet->id == 0x07) parseNavPvt(packet);
  else {
    DEBUG_SERIAL.print(F("  Class: 0x"));
    if (packet->cls < 0x10) DEBUG_SERIAL.print(F("0"));
    DEBUG_SERIAL.print(packet->cls, HEX);
    DEBUG_SERIAL.print(F(" ID: 0x"));
    if (packet->id < 0x10) DEBUG_SERIAL.print(F("0"));
    DEBUG_SERIAL.print(packet->id, HEX);
    DEBUG_SERIAL.print(F(" Len: "));
    DEBUG_SERIAL.println(packet->len);
  }
}

void parseNavPosllh(UBX_Packet* p) {
  int32_t lon = *((int32_t*)(p->payload + 4));
  int32_t lat = *((int32_t*)(p->payload + 8));
  int32_t hMSL = *((int32_t*)(p->payload + 16));
  uint32_t hAcc = *((uint32_t*)(p->payload + 20));
  uint32_t vAcc = *((uint32_t*)(p->payload + 24));
  
  DEBUG_SERIAL.print(F("  Lat: ")); DEBUG_SERIAL.print(lat / 10000000.0, 7); DEBUG_SERIAL.println(F("°"));
  DEBUG_SERIAL.print(F("  Lon: ")); DEBUG_SERIAL.print(lon / 10000000.0, 7); DEBUG_SERIAL.println(F("°"));
  DEBUG_SERIAL.print(F("  Alt: ")); DEBUG_SERIAL.print(hMSL / 1000.0, 1); DEBUG_SERIAL.println(F(" m"));
  DEBUG_SERIAL.print(F("  Acc: ")); DEBUG_SERIAL.print(hAcc / 1000.0, 1); DEBUG_SERIAL.print(F("m H, "));
  DEBUG_SERIAL.print(vAcc / 1000.0, 1); DEBUG_SERIAL.println(F("m V"));
}

void parseNavStatus(UBX_Packet* p) {
  uint8_t gpsFix = p->payload[4];
  DEBUG_SERIAL.print(F("  Fix: "));
  switch (gpsFix) {
    case 0: DEBUG_SERIAL.println(F("None")); break;
    case 2: DEBUG_SERIAL.println(F("2D")); break;
    case 3: DEBUG_SERIAL.println(F("3D")); break;
    default: DEBUG_SERIAL.println(gpsFix); break;
  }
}

void parseNavSat(UBX_Packet* p) {
  uint8_t numSvs = p->payload[5];
  DEBUG_SERIAL.print(F("  Satellites: ")); DEBUG_SERIAL.println(numSvs);
  
  for (uint8_t i = 0; i < min(numSvs, (uint8_t)15); i++) {
    uint16_t offset = 8 + (i * 12);
    uint8_t gnssId = p->payload[offset];
    uint8_t svId = p->payload[offset + 1];
    uint8_t cno = p->payload[offset + 2];
    uint8_t elev = p->payload[offset + 3];
    
    if (cno > 0) {
      DEBUG_SERIAL.print(F("    "));
      switch (gnssId) {
        case 0: DEBUG_SERIAL.print(F("GPS")); break;
        case 2: DEBUG_SERIAL.print(F("GAL")); break;
        case 3: DEBUG_SERIAL.print(F("BDS")); break;
        case 6: DEBUG_SERIAL.print(F("GLO")); break;
        default: DEBUG_SERIAL.print(F("OTH")); break;
      }
      DEBUG_SERIAL.print(F(" "));
      DEBUG_SERIAL.print(svId);
      DEBUG_SERIAL.print(F(": "));
      DEBUG_SERIAL.print(cno);
      DEBUG_SERIAL.print(F("dB @ "));
      DEBUG_SERIAL.print(elev);
      DEBUG_SERIAL.println(F("°"));
    }
  }
}

void parseMonVer(UBX_Packet* p) {
  DEBUG_SERIAL.print(F("  SW: "));
  for (uint8_t i = 0; i < 30 && p->payload[i] != 0; i++) {
    DEBUG_SERIAL.write(p->payload[i]);
  }
  DEBUG_SERIAL.println();
  DEBUG_SERIAL.print(F("  HW: "));
  for (uint8_t i = 30; i < 40 && p->payload[i] != 0; i++) {
    DEBUG_SERIAL.write(p->payload[i]);
  }
  DEBUG_SERIAL.println();
}

void parseNavDop(UBX_Packet* p) {
  uint16_t hDOP = *((uint16_t*)(p->payload + 12));
  uint16_t vDOP = *((uint16_t*)(p->payload + 14));
  uint16_t pDOP = *((uint16_t*)(p->payload + 6));
  
  DEBUG_SERIAL.print(F("  HDOP: ")); DEBUG_SERIAL.print(hDOP / 100.0, 1);
  DEBUG_SERIAL.print(F(" VDOP: ")); DEBUG_SERIAL.print(vDOP / 100.0, 1);
  DEBUG_SERIAL.print(F(" PDOP: ")); DEBUG_SERIAL.println(pDOP / 100.0, 1);
}

void parseNavPvt(UBX_Packet* p) {
  uint8_t fixType = p->payload[20];
  uint8_t numSV = p->payload[23];
  int32_t lon = *((int32_t*)(p->payload + 24));
  int32_t lat = *((int32_t*)(p->payload + 28));
  int32_t hMSL = *((int32_t*)(p->payload + 36));
  
  DEBUG_SERIAL.print(F("  Fix: ")); DEBUG_SERIAL.print(fixType);
  DEBUG_SERIAL.print(F(" SVs: ")); DEBUG_SERIAL.println(numSV);
  DEBUG_SERIAL.print(F("  Lat: ")); DEBUG_SERIAL.print(lat / 10000000.0, 7);
  DEBUG_SERIAL.print(F("° Lon: ")); DEBUG_SERIAL.print(lon / 10000000.0, 7);
  DEBUG_SERIAL.println(F("°"));
  DEBUG_SERIAL.print(F("  Alt: ")); DEBUG_SERIAL.print(hMSL / 1000.0, 1);
  DEBUG_SERIAL.println(F(" m"));
}

void printMenu() {
  DEBUG_SERIAL.println(F("\n========== DIAGNOSTIC MENU =========="));
  DEBUG_SERIAL.println(F("M - Monitor GPS output (toggle)"));
  DEBUG_SERIAL.println(F("X - Show raw hex bytes (toggle)"));
  DEBUG_SERIAL.println(F("B - Test all baud rates"));
  DEBUG_SERIAL.println(F("D - Disable all NMEA messages"));
  DEBUG_SERIAL.println(F("\n========== QUICK COMMANDS =========="));
  DEBUG_SERIAL.println(F("1-Position 2-Status 3-Satellites 4-Version"));
  DEBUG_SERIAL.println(F("r-1Hz t-5Hz y-10Hz | s-Save g-DisGGA"));
  DEBUG_SERIAL.println(F("=====================================\n"));
}