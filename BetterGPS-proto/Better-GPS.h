#pragma once

#include <HardwareSerial.h>
#include <queue>
#include "Better-GPS-commands.h"
#include "Better-GPS-structs.h"
#include "Better-GPS-config.h"

class BetterGPS
{
private:
  HardwareSerial gpsSerial;

  // UBX response buffers
  NAV_POSLLH_Response posllhData;
  NAV_STATUS_Response statusData;
  NAV_DOP_Response dopData;
  NAV_PVT_Response pvtData;
  NAV_ODO_Response odoData;
  NAV_VELNED_Response velnedData;
  NAV_TIMEUTC_Response timeutcData;
  NAV_SAT_Response satData;
  RXM_RAWX_Response rawxData;
  RXM_SFRBX_Response sfrbxData;
  MON_HW_Response hwData;

  // UBX receive buffer
  uint8_t ubxBuffer[512];
  uint16_t ubxBufferIndex;
  bool ubxReceiving;

  // Command queue for reliable communication
  std::queue<UBXCommand> commandQueue;
  bool processingCommand;

  // Timezone configuration
  float timezoneOffset;
  bool autoDST;

  // Geofence
  Geofence geofence;
  bool lastGeofenceState;

  // Current position cache (updated from UBX messages)
  struct PositionCache
  {
    double latitude;
    double longitude;
    double altitude;
    double speedKmph;
    double speedMps;
    double course;
    uint8_t satellites;
    double hdop;
    bool valid;
    unsigned long lastUpdate;
  } posCache;

  // ==================== UBX PARSING ====================

  void calculateChecksum(const uint8_t *data, uint16_t len, uint8_t &ckA, uint8_t &ckB)
  {
    ckA = 0;
    ckB = 0;
    for (uint16_t i = 0; i < len; i++)
    {
      ckA += data[i];
      ckB += ckA;
    }
  }

  void sendUBX(const uint8_t *cmd, uint16_t len)
  {
    for (uint16_t i = 0; i < len; i++)
    {
      gpsSerial.write(cmd[i]);
    }
    gpsSerial.flush();
  }

  void updatePositionCache()
  {
    // Update from NAV-POSLLH if available
    if (posllhData.valid)
    {
      posCache.latitude = posllhData.lat / 1e7;
      posCache.longitude = posllhData.lon / 1e7;
      posCache.altitude = posllhData.hMSL / 1000.0;
      posCache.valid = true;
      posCache.lastUpdate = millis();
    }

    // Update from NAV-PVT if available (more comprehensive)
    if (pvtData.validData && (pvtData.fixType >= 2))
    {
      posCache.latitude = pvtData.lat / 1e7;
      posCache.longitude = pvtData.lon / 1e7;
      posCache.altitude = pvtData.hMSL / 1000.0;
      posCache.speedMps = pvtData.gSpeed / 1000.0;
      posCache.speedKmph = posCache.speedMps * 3.6;
      posCache.course = pvtData.headMot / 1e5;
      posCache.satellites = pvtData.numSV;
      posCache.hdop = pvtData.pDOP / 100.0;
      posCache.valid = true;
      posCache.lastUpdate = millis();
    }

    // Update from NAV-STATUS
    if (statusData.valid)
    {
      // Fix type validation
      if (statusData.gpsFix >= 2)
      { // 2D or 3D fix
        if (!posCache.valid)
        {
          // Request position data if we have a fix but no position yet
          requestPosition();
        }
      }
      else
      {
        posCache.valid = false;
      }
    }
  }

  void parseUBXResponse(uint8_t msgClass, uint8_t msgId, uint8_t *payload, uint16_t payloadLen)
  {
    if (msgClass == 0x01 && msgId == 0x02 && payloadLen >= 28)
    {
      // NAV-POSLLH
      posllhData.iTOW = *((uint32_t *)(payload + 0));
      posllhData.lon = *((int32_t *)(payload + 4));
      posllhData.lat = *((int32_t *)(payload + 8));
      posllhData.height = *((int32_t *)(payload + 12));
      posllhData.hMSL = *((int32_t *)(payload + 16));
      posllhData.hAcc = *((uint32_t *)(payload + 20));
      posllhData.vAcc = *((uint32_t *)(payload + 24));
      posllhData.valid = true;
      updatePositionCache();
    }
    else if (msgClass == 0x01 && msgId == 0x03 && payloadLen >= 16)
    {
      // NAV-STATUS
      statusData.iTOW = *((uint32_t *)(payload + 0));
      statusData.gpsFix = payload[4];
      statusData.flags = payload[5];
      statusData.fixStat = payload[6];
      statusData.flags2 = payload[7];
      statusData.ttff = *((uint32_t *)(payload + 8));
      statusData.msss = *((uint32_t *)(payload + 12));
      statusData.valid = true;
      updatePositionCache();
    }
    else if (msgClass == 0x01 && msgId == 0x04 && payloadLen >= 18)
    {
      // NAV-DOP
      dopData.iTOW = *((uint32_t *)(payload + 0));
      dopData.gDOP = *((uint16_t *)(payload + 4));
      dopData.pDOP = *((uint16_t *)(payload + 6));
      dopData.tDOP = *((uint16_t *)(payload + 8));
      dopData.vDOP = *((uint16_t *)(payload + 10));
      dopData.hDOP = *((uint16_t *)(payload + 12));
      dopData.nDOP = *((uint16_t *)(payload + 14));
      dopData.eDOP = *((uint16_t *)(payload + 16));
      dopData.valid = true;
    }
    else if (msgClass == 0x01 && msgId == 0x07 && payloadLen >= 92)
    {
      // NAV-PVT
      pvtData.iTOW = *((uint32_t *)(payload + 0));
      pvtData.year = *((uint16_t *)(payload + 4));
      pvtData.month = payload[6];
      pvtData.day = payload[7];
      pvtData.hour = payload[8];
      pvtData.minute = payload[9];
      pvtData.second = payload[10];
      pvtData.valid = payload[11];
      pvtData.tAcc = *((uint32_t *)(payload + 12));
      pvtData.nano = *((int32_t *)(payload + 16));
      pvtData.fixType = payload[20];
      pvtData.flags = payload[21];
      pvtData.flags2 = payload[22];
      pvtData.numSV = payload[23];
      pvtData.lon = *((int32_t *)(payload + 24));
      pvtData.lat = *((int32_t *)(payload + 28));
      pvtData.height = *((int32_t *)(payload + 32));
      pvtData.hMSL = *((int32_t *)(payload + 36));
      pvtData.hAcc = *((uint32_t *)(payload + 40));
      pvtData.vAcc = *((uint32_t *)(payload + 44));
      pvtData.velN = *((int32_t *)(payload + 48));
      pvtData.velE = *((int32_t *)(payload + 52));
      pvtData.velD = *((int32_t *)(payload + 56));
      pvtData.gSpeed = *((int32_t *)(payload + 60));
      pvtData.headMot = *((int32_t *)(payload + 64));
      pvtData.sAcc = *((uint32_t *)(payload + 68));
      pvtData.headAcc = *((uint32_t *)(payload + 72));
      pvtData.pDOP = *((uint16_t *)(payload + 76));
      pvtData.headVeh = *((int32_t *)(payload + 84));
      pvtData.validData = true;
      updatePositionCache();
    }
    else if (msgClass == 0x01 && msgId == 0x09 && payloadLen >= 20)
    {
      // NAV-ODO
      odoData.version = payload[0];
      odoData.iTOW = *((uint32_t *)(payload + 4));
      odoData.distance = *((uint32_t *)(payload + 8));
      odoData.totalDistance = *((uint32_t *)(payload + 12));
      odoData.distanceStd = *((uint32_t *)(payload + 16));
      odoData.valid = true;
    }
    else if (msgClass == 0x01 && msgId == 0x12 && payloadLen >= 36)
    {
      // NAV-VELNED
      velnedData.iTOW = *((uint32_t *)(payload + 0));
      velnedData.velN = *((int32_t *)(payload + 4));
      velnedData.velE = *((int32_t *)(payload + 8));
      velnedData.velD = *((int32_t *)(payload + 12));
      velnedData.speed = *((uint32_t *)(payload + 16));
      velnedData.gSpeed = *((uint32_t *)(payload + 20));
      velnedData.heading = *((int32_t *)(payload + 24));
      velnedData.sAcc = *((uint32_t *)(payload + 28));
      velnedData.cAcc = *((uint32_t *)(payload + 32));
      velnedData.valid = true;
    }
    else if (msgClass == 0x01 && msgId == 0x21 && payloadLen >= 20)
    {
      // NAV-TIMEUTC
      timeutcData.iTOW = *((uint32_t *)(payload + 0));
      timeutcData.tAcc = *((uint32_t *)(payload + 4));
      timeutcData.nano = *((int32_t *)(payload + 8));
      timeutcData.year = *((uint16_t *)(payload + 12));
      timeutcData.month = payload[14];
      timeutcData.day = payload[15];
      timeutcData.hour = payload[16];
      timeutcData.min = payload[17];
      timeutcData.sec = payload[18];
      timeutcData.valid = payload[19];
      timeutcData.validData = true;
    }
    else if (msgClass == 0x01 && msgId == 0x35 && payloadLen >= 8)
    {
      // NAV-SAT
      satData.iTOW = *((uint32_t *)(payload + 0));
      satData.numSvs = payload[5];
      uint16_t offset = 8;
      for (int i = 0; i < satData.numSvs && i < 32; i++)
      {
        if (offset + 12 <= payloadLen)
        {
          satData.satellites[i].gnssId = payload[offset + 0];
          satData.satellites[i].svId = payload[offset + 1];
          satData.satellites[i].cno = payload[offset + 2];
          satData.satellites[i].elev = (int8_t)payload[offset + 3];
          satData.satellites[i].azim = *((int16_t *)(payload + offset + 4));
          satData.satellites[i].prRes = *((int16_t *)(payload + offset + 6));
          satData.satellites[i].flags = *((uint32_t *)(payload + offset + 8));
          satData.satellites[i].used = (satData.satellites[i].flags & 0x08) != 0;
          offset += 12;
        }
      }
      satData.valid = true;
    }
    else if (msgClass == 0x02 && msgId == 0x15 && payloadLen >= 16)
    {
      // RXM-RAWX
      rawxData.rcvTow = *((double *)(payload + 0));
      rawxData.week = *((uint16_t *)(payload + 8));
      rawxData.leapS = (int8_t)payload[10];
      rawxData.numMeas = payload[11];
      rawxData.recStat = payload[12];
      uint16_t offset = 16;
      for (int i = 0; i < rawxData.numMeas && i < 32; i++)
      {
        if (offset + 32 <= payloadLen)
        {
          rawxData.measurements[i].prMes = *((double *)(payload + offset + 0));
          rawxData.measurements[i].cpMes = *((double *)(payload + offset + 8));
          rawxData.measurements[i].doMes = *((float *)(payload + offset + 16));
          rawxData.measurements[i].gnssId = payload[offset + 20];
          rawxData.measurements[i].svId = payload[offset + 21];
          rawxData.measurements[i].freqId = payload[offset + 22];
          rawxData.measurements[i].locktime = *((uint16_t *)(payload + offset + 23));
          rawxData.measurements[i].cno = payload[offset + 25];
          rawxData.measurements[i].prStdev = payload[offset + 26];
          rawxData.measurements[i].cpStdev = payload[offset + 27];
          rawxData.measurements[i].doStdev = payload[offset + 28];
          rawxData.measurements[i].trkStat = payload[offset + 29];
          offset += 32;
        }
      }
      rawxData.valid = true;
    }
    else if (msgClass == 0x02 && msgId == 0x13 && payloadLen >= 8)
    {
      // RXM-SFRBX
      sfrbxData.gnssId = payload[0];
      sfrbxData.svId = payload[1];
      sfrbxData.freqId = payload[3];
      sfrbxData.numWords = payload[4];
      sfrbxData.chn = payload[5];
      sfrbxData.version = payload[6];
      for (int i = 0; i < sfrbxData.numWords && i < 10; i++)
      {
        if (8 + i * 4 <= payloadLen)
        {
          sfrbxData.dwrd[i] = *((uint32_t *)(payload + 8 + i * 4));
        }
      }
      sfrbxData.valid = true;
    }
    else if (msgClass == 0x0A && msgId == 0x09 && payloadLen >= 60)
    {
      // MON-HW
      hwData.pinSel = *((uint32_t *)(payload + 0));
      hwData.pinBank = *((uint32_t *)(payload + 4));
      hwData.pinDir = *((uint32_t *)(payload + 8));
      hwData.pinVal = *((uint32_t *)(payload + 12));
      hwData.noisePerMS = *((uint16_t *)(payload + 16));
      hwData.agcCnt = *((uint16_t *)(payload + 18));
      hwData.aStatus = payload[20];
      hwData.aPower = payload[21];
      hwData.flags = payload[22];
      hwData.usedMask = *((uint32_t *)(payload + 24));
      hwData.jamInd = payload[35];
      hwData.valid = true;
    }
  }

  void processUBX()
  {
    while (gpsSerial.available())
    {
      uint8_t c = gpsSerial.read();

      if (!ubxReceiving)
      {
        if (ubxBufferIndex == 0 && c == 0xB5)
        {
          ubxBuffer[ubxBufferIndex++] = c;
        }
        else if (ubxBufferIndex == 1 && c == 0x62)
        {
          ubxBuffer[ubxBufferIndex++] = c;
          ubxReceiving = true;
        }
        else
        {
          ubxBufferIndex = 0;
        }
      }
      else
      {
        ubxBuffer[ubxBufferIndex++] = c;

        if (ubxBufferIndex >= 6)
        {
          uint16_t payloadLen = ubxBuffer[4] | (ubxBuffer[5] << 8);
          uint16_t totalLen = 6 + payloadLen + 2;

          if (ubxBufferIndex >= totalLen)
          {
            uint8_t ckA, ckB;
            calculateChecksum(&ubxBuffer[2], payloadLen + 4, ckA, ckB);

            if (ckA == ubxBuffer[totalLen - 2] && ckB == ubxBuffer[totalLen - 1])
            {
              parseUBXResponse(ubxBuffer[2], ubxBuffer[3], &ubxBuffer[6], payloadLen);
            }

            ubxReceiving = false;
            ubxBufferIndex = 0;
          }

          if (ubxBufferIndex >= sizeof(ubxBuffer))
          {
            ubxReceiving = false;
            ubxBufferIndex = 0;
          }
        }
      }
    }
  }

  bool checkACKResponse()
  {
    unsigned long startTime = millis();

    while (millis() - startTime < UBX_RESPONSE_TIMEOUT)
    {
      if (gpsSerial.available() >= 10)
      {
        uint8_t header[10];
        for (int i = 0; i < 10; i++)
        {
          header[i] = gpsSerial.read();
        }

        if (header[0] == 0xB5 && header[1] == 0x62 && header[2] == 0x05)
        {
          if (header[3] == 0x00)
            return false; // ACK-NAK
          else if (header[3] == 0x01)
            return true; // ACK-ACK
        }
      }
      delay(10);
    }
    return false;
  }

  void processCommandQueue()
  {
    if (commandQueue.empty() || processingCommand)
      return;

    processingCommand = true;
    UBXCommand &cmd = commandQueue.front();

    if (millis() - cmd.timestamp > UBX_RESPONSE_TIMEOUT)
    {
      if (cmd.retries > 0)
      {
        sendUBX(cmd.data, cmd.length);
        cmd.retries--;
        cmd.timestamp = millis();
      }
      else
      {
        commandQueue.pop();
        processingCommand = false;
      }
    }

    if (checkACKResponse())
    {
      commandQueue.pop();
      processingCommand = false;
    }
  }

  void setBaudrate(uint8_t baudIndex)
  {
    if (baudIndex < 8)
    {
      sendUBX(UBX_BAUDRATES[baudIndex], sizeof(UBX_BAUDRATES[0]));
    }
  }

  void setAutobaudrate(uint8_t gpsRx, uint8_t gpsTx)
  {
    const int BAUDRATES[] = {4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800};
    const uint8_t TEST_POLL_MSG[] = {0xB5, 0x62, 0x06, 0x00, 0x00, 0x00, 0x06, 0x18};

    for (int i = 0; i < 8; i++)
    {
      gpsSerial.begin(BAUDRATES[i], SERIAL_8N1, gpsRx, gpsTx);
      delay(250);

      // Clear any existing data
      while (gpsSerial.available())
        gpsSerial.read();

      delay(50);

      // Send test message
      for (size_t j = 0; j < sizeof(TEST_POLL_MSG); j++)
      {
        gpsSerial.write(TEST_POLL_MSG[j]);
      }
      gpsSerial.flush();

      if (checkACKResponse())
      {
        // Found working baud rate, keep using it
        Serial.print("GPS found at ");
        Serial.print(BAUDRATES[i]);
        Serial.println(" baud");
        return;
      }
    }

    // If nothing worked, default to 9600
    Serial.println("GPS auto baud failed, defaulting to 9600");
    gpsSerial.begin(38400, SERIAL_8N1, gpsRx, gpsTx);
  }

  int calculateDayOfWeek(int y, int m, int d)
  {
    if (m < 3)
    {
      m += 12;
      y -= 1;
    }
    int k = y % 100, j = y / 100;
    int f = d + 13 * (m + 1) / 5 + k + k / 4 + j / 4 + 5 * j;
    return (f + 1) % 7;
  }

  bool isDaylightSavingTime(int year, int month, int day, int hour)
  {
    if (!autoDST)
      return false;

    if (month < 3 || month > 10)
      return false;
    if (month > 3 && month < 10)
      return true;

    if (month == 3)
    {
      int lastSunday = 31;
      while (calculateDayOfWeek(year, 3, lastSunday) != 0)
        lastSunday--;
      if (day < lastSunday)
        return false;
      if (day > lastSunday)
        return true;
      return hour >= 1;
    }

    if (month == 10)
    {
      int lastSunday = 31;
      while (calculateDayOfWeek(year, 10, lastSunday) != 0)
        lastSunday--;
      if (day < lastSunday)
        return true;
      if (day > lastSunday)
        return false;
      return hour < 1;
    }

    return false;
  }

  void convertToLocalTime(int &year, int &month, int &day, int &hour, int &minute, int &second)
  {
    float offset = timezoneOffset;
    if (isDaylightSavingTime(year, month, day, hour))
    {
      offset += 1.0;
    }

    int offsetHours = (int)offset;
    int offsetMinutes = (int)((offset - offsetHours) * 60);

    hour += offsetHours;
    minute += offsetMinutes;

    if (minute >= 60)
    {
      minute -= 60;
      hour++;
    }
    else if (minute < 0)
    {
      minute += 60;
      hour--;
    }

    if (hour >= 24)
    {
      hour -= 24;
      day++;
      int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
      bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
      if (isLeapYear)
        daysInMonth[1] = 29;

      if (day > daysInMonth[month - 1])
      {
        day = 1;
        month++;
        if (month > 12)
        {
          month = 1;
          year++;
        }
      }
    }
    else if (hour < 0)
    {
      hour += 24;
      day--;
      if (day < 1)
      {
        month--;
        if (month < 1)
        {
          month = 12;
          year--;
        }
        int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (isLeapYear)
          daysInMonth[1] = 29;
        day = daysInMonth[month - 1];
      }
    }
  }

  void updateTimeCache()
  {
    if (!hasFix())
    {
      timeCache.valid = false;
      return;
    }

    // Try to get time from PVT first (most comprehensive)
    int year, month, day, hour, minute, second;

    if (pvtData.validData && (pvtData.valid & 0x03) == 0x03)
    {
      year = pvtData.year;
      month = pvtData.month;
      day = pvtData.day;
      hour = pvtData.hour;
      minute = pvtData.minute;
      second = pvtData.second;
    }
    else if (timeutcData.validData && (timeutcData.valid & 0x03) == 0x03)
    {
      year = timeutcData.year;
      month = timeutcData.month;
      day = timeutcData.day;
      hour = timeutcData.hour;
      minute = timeutcData.min;
      second = timeutcData.sec;
    }
    else
    {
      timeCache.valid = false;
      return;
    }

    convertToLocalTime(year, month, day, hour, minute, second);

    timeCache.year = year;
    timeCache.month = month;
    timeCache.day = day;
    timeCache.hour = hour;
    timeCache.minute = minute;
    timeCache.second = second;
    timeCache.dayIndex = calculateDayOfWeek(year, month, day);
    timeCache.valid = true;
    timeCache.lastUpdate = millis();
  }

  bool checkGeofence()
  {
    if (!geofence.active || !hasFix())
      return false;

    double distance = calculateDistance(getLatitude(), getLongitude(),
                                        geofence.centerLat, geofence.centerLon);
    return distance <= geofence.radius;
  }

public:
  BetterGPS()
      : gpsSerial(1)
  {
    timeCache.valid = false;
    posllhData.valid = false;
    statusData.valid = false;
    dopData.valid = false;
    pvtData.validData = false;
    odoData.valid = false;
    velnedData.valid = false;
    timeutcData.validData = false;
    satData.valid = false;
    rawxData.valid = false;
    sfrbxData.valid = false;
    hwData.valid = false;
    ubxBufferIndex = 0;
    ubxReceiving = false;
    processingCommand = false;
    timezoneOffset = TIMEZONE_OFFSET;
    autoDST = AUTO_DST;
    geofence.active = false;
    lastGeofenceState = false;

    // Initialize position cache
    posCache.valid = false;
    posCache.latitude = 0;
    posCache.longitude = 0;
    posCache.altitude = 0;
    posCache.speedKmph = 0;
    posCache.speedMps = 0;
    posCache.course = 0;
    posCache.satellites = 0;
    posCache.hdop = 99.99;
  }

  void begin(int gpsRx, int gpsTx = -1)
  {
    setAutobaudrate(gpsRx, gpsTx);
    delay(500);

    setUpdateRate(UPDATE_RATE);
    delay(100);

    setPowerMode(POWER_SAVING_ON ? PowerMode::PowerSave : PowerMode::Continuous);
    delay(100);

    // Disable NMEA messages (we're using UBX only)
    if (DISABLE_GGA)
    {
      sendUBX(UBX_DISABLE_GGA, sizeof(UBX_DISABLE_GGA));
      delay(50);
    }
    if (DISABLE_GLL)
    {
      sendUBX(UBX_DISABLE_GLL, sizeof(UBX_DISABLE_GLL));
      delay(50);
    }
    if (DISABLE_GSA)
    {
      sendUBX(UBX_DISABLE_GSA, sizeof(UBX_DISABLE_GSA));
      delay(50);
    }
    if (DISABLE_GSV)
    {
      sendUBX(UBX_DISABLE_GSV, sizeof(UBX_DISABLE_GSV));
      delay(50);
    }
    if (DISABLE_RMC)
    {
      sendUBX(UBX_DISABLE_RMC, sizeof(UBX_DISABLE_RMC));
      delay(50);
    }
    if (DISABLE_VTG)
    {
      sendUBX(UBX_DISABLE_VTG, sizeof(UBX_DISABLE_VTG));
      delay(50);
    }

    setGNSS(GPS_ENABLED, GALILEO_ENABLED, GLONASS_ENABLED, BEIDOU_ENABLED);
    delay(100);

    setSmoothing(STARTUP_SMOOTHING);
    delay(100);

    if (STARTUP_POSITION)
    {
      requestPosition();
      delay(200);
    }
    if (STARTUP_STATUS)
    {
      requestStatus();
      delay(200);
    }
    if (STARTUP_SATELLITE_INFO)
    {
      requestSatellites();
      delay(200);
    }
    if (STARTUP_VERSION)
    {
      requestVersion();
      delay(200);
    }

    saveConfig(static_cast<SaveLocation>(DEFAULT_SAVE_LOCATION));
    delay(500);
  }

  void update()
  {
    processUBX();
    processCommandQueue();

    // Auto-request position updates if we have a fix
    static unsigned long lastAutoUpdate = 0;
    if (statusData.valid && statusData.gpsFix >= 2)
    {
      if (millis() - lastAutoUpdate > 1000)
      { // Request every second
        requestPosition();
        lastAutoUpdate = millis();
      }
    }

    if (posCache.valid && geofence.active)
    {
      bool currentState = checkGeofence();
      if (currentState != lastGeofenceState)
      {
        lastGeofenceState = currentState;
      }
    }
  }

  void setUpdateRate(UpdateRate rate)
  {
    switch (rate)
    {
    case UpdateRate::HZ_1:
      sendUBX(UBX_RATE_1HZ, sizeof(UBX_RATE_1HZ));
      break;
    case UpdateRate::HZ_5:
      sendUBX(UBX_RATE_5HZ, sizeof(UBX_RATE_5HZ));
      break;
    case UpdateRate::HZ_10:
      sendUBX(UBX_RATE_10HZ, sizeof(UBX_RATE_10HZ));
      break;
    }
    delay(100);
  }

  void requestPosition()
  {
    posllhData.valid = false;
    sendUBX(UBX_POLL_POSLLH, sizeof(UBX_POLL_POSLLH));
  }

  void requestStatus()
  {
    statusData.valid = false;
    sendUBX(UBX_POLL_STATUS, sizeof(UBX_POLL_STATUS));
  }

  void requestDOP()
  {
    dopData.valid = false;
    sendUBX(UBX_POLL_DOP, sizeof(UBX_POLL_DOP));
  }

  void requestPVT()
  {
    pvtData.validData = false;
    sendUBX(UBX_POLL_PVT, sizeof(UBX_POLL_PVT));
  }

  void requestOdometer()
  {
    odoData.valid = false;
    sendUBX(UBX_POLL_ODO, sizeof(UBX_POLL_ODO));
  }

  void requestVelocity()
  {
    velnedData.valid = false;
    sendUBX(UBX_POLL_VELNED, sizeof(UBX_POLL_VELNED));
  }

  void requestTimeUTC()
  {
    timeutcData.validData = false;
    sendUBX(UBX_POLL_TIMEUTC, sizeof(UBX_POLL_TIMEUTC));
  }

  void requestSatellites()
  {
    satData.valid = false;
    sendUBX(UBX_POLL_SAT, sizeof(UBX_POLL_SAT));
  }

  void requestRawMeasurements()
  {
    rawxData.valid = false;
    sendUBX(UBX_POLL_RAWX, sizeof(UBX_POLL_RAWX));
  }

  void requestSubframeData()
  {
    sfrbxData.valid = false;
    sendUBX(UBX_POLL_SFRBX, sizeof(UBX_POLL_SFRBX));
  }

  void requestVersion()
  {
    sendUBX(UBX_POLL_VERSION, sizeof(UBX_POLL_VERSION));
  }

  void requestHardwareStatus()
  {
    hwData.valid = false;
    sendUBX(UBX_POLL_HW, sizeof(UBX_POLL_HW));
  }

  void setPowerMode(PowerMode mode)
  {
    if (mode == PowerMode::PowerSave)
      sendUBX(UBX_POWER_SAVE, sizeof(UBX_POWER_SAVE));
    else
      sendUBX(UBX_POWER_CONTINUOUS, sizeof(UBX_POWER_CONTINUOUS));
  }

  void setDynamicModel(DynamicModel model)
  {
    switch (model)
    {
    case DynamicModel::Portable:
      sendUBX(UBX_NAV5_PORTABLE, sizeof(UBX_NAV5_PORTABLE));
      break;
    case DynamicModel::Stationary:
      sendUBX(UBX_NAV5_STATIONARY, sizeof(UBX_NAV5_STATIONARY));
      break;
    case DynamicModel::Pedestrian:
      sendUBX(UBX_NAV5_PEDESTRIAN, sizeof(UBX_NAV5_PEDESTRIAN));
      break;
    case DynamicModel::Automotive:
      sendUBX(UBX_NAV5_AUTOMOTIVE, sizeof(UBX_NAV5_AUTOMOTIVE));
      break;
    }
    delay(100);
  }

  void setGNSS(bool gps, bool galileo, bool glonass, bool beidou)
  {
    if (gps && galileo && glonass && beidou)
      sendUBX(UBX_GNSS_ALL, sizeof(UBX_GNSS_ALL));
    else if (gps && galileo && !glonass && !beidou)
      sendUBX(UBX_GNSS_GPS_GALILEO, sizeof(UBX_GNSS_GPS_GALILEO));
    else if (gps && !galileo && !glonass && !beidou)
      sendUBX(UBX_GNSS_GPS_ONLY, sizeof(UBX_GNSS_GPS_ONLY));
    delay(100);
  }

  void setSmoothing(bool enable)
  {
    if (enable)
      sendUBX(UBX_ENABLE_SMOOTHING, sizeof(UBX_ENABLE_SMOOTHING));
    else
      sendUBX(UBX_NO_SMOOTHING, sizeof(UBX_NO_SMOOTHING));
    delay(100);
  }

  void saveConfig(SaveLocation target = SaveLocation::Both)
  {
    switch (target)
    {
    case SaveLocation::Flash:
      sendUBX(UBX_SAVE_FLASH, sizeof(UBX_SAVE_FLASH));
      break;
    case SaveLocation::BBR:
      sendUBX(UBX_SAVE_BBR, sizeof(UBX_SAVE_BBR));
      break;
    case SaveLocation::Both:
      sendUBX(UBX_SAVE_ALL, sizeof(UBX_SAVE_ALL));
      break;
    }
    delay(200);
  }

  void resetHot()
  {
    sendUBX(UBX_RESET_HOT, sizeof(UBX_RESET_HOT));
    delay(1000);
  }

  void resetWarm()
  {
    sendUBX(UBX_RESET_WARM, sizeof(UBX_RESET_WARM));
    delay(2000);
  }

  void resetCold()
  {
    sendUBX(UBX_RESET_COLD, sizeof(UBX_RESET_COLD));
    delay(3000);
  }

  void sendCustomUBX(const uint8_t *cmd, uint16_t len)
  {
    sendUBX(cmd, len);
  }

  bool hasFix()
  {
    // Check if we have a valid fix from status data
    if (statusData.valid && statusData.gpsFix >= 2)
    {
      return true;
    }

    // Also check PVT data
    if (pvtData.validData && pvtData.fixType >= 2)
    {
      return true;
    }

    // Check position cache
    return posCache.valid && (millis() - posCache.lastUpdate < 5000);
  }

  FixType getFixType()
  {
    return statusData.valid ? static_cast<FixType>(statusData.gpsFix) : FixType::NoFix;
  }

  double getLatitude()
  {
    return posCache.valid ? posCache.latitude : 0.0;
  }

  double getLongitude()
  {
    return posCache.valid ? posCache.longitude : 0.0;
  }

  double getAltitude()
  {
    return posCache.valid ? posCache.altitude : 0.0;
  }

  double getSpeedKmph()
  {
    return posCache.valid ? posCache.speedKmph : 0.0;
  }

  double getSpeedMps()
  {
    return posCache.valid ? posCache.speedMps : 0.0;
  }

  double getCourse()
  {
    return posCache.valid ? posCache.course : 0.0;
  }

  uint8_t getSatelliteCount()
  {
    if (pvtData.validData)
      return pvtData.numSV;
    if (satData.valid)
      return getUsedSatelliteCount();
    return 0;
  }

  double getHDOP()
  {
    if (dopData.valid)
      return dopData.hDOP / 100.0;
    if (pvtData.validData)
      return pvtData.pDOP / 100.0;
    return 99.99;
  }

  NAV_POSLLH_Response getPOSLLH()
  {
    return posllhData;
  }

  NAV_STATUS_Response getStatus()
  {
    return statusData;
  }

  NAV_DOP_Response getDOP()
  {
    return dopData;
  }

  NAV_PVT_Response getPVT()
  {
    return pvtData;
  }

  NAV_ODO_Response getOdometer()
  {
    return odoData;
  }

  NAV_VELNED_Response getVelocity()
  {
    return velnedData;
  }

  NAV_TIMEUTC_Response getTimeUTC()
  {
    return timeutcData;
  }

  NAV_SAT_Response getSatellites()
  {
    return satData;
  }

  RXM_RAWX_Response getRawMeasurements()
  {
    return rawxData;
  }

  RXM_SFRBX_Response getSubframeData()
  {
    return sfrbxData;
  }

  MON_HW_Response getHardwareStatus()
  {
    return hwData;
  }

  bool isPositionReady()
  {
    return posllhData.valid;
  }

  bool isStatusReady()
  {
    return statusData.valid;
  }

  bool isDOPReady()
  {
    return dopData.valid;
  }

  bool isPVTReady()
  {
    return pvtData.validData;
  }

  bool isOdometerReady()
  {
    return odoData.valid;
  }

  bool isVelocityReady()
  {
    return velnedData.valid;
  }

  bool isTimeUTCReady()
  {
    return timeutcData.validData;
  }

  bool isSatellitesReady()
  {
    return satData.valid;
  }

  bool isRawMeasurementsReady()
  {
    return rawxData.valid;
  }

  bool isSubframeDataReady()
  {
    return sfrbxData.valid;
  }

  bool isHardwareStatusReady()
  {
    return hwData.valid;
  }

  bool waitForPosition(uint32_t timeout_ms)
  {
    unsigned long start = millis();
    while (!posllhData.valid && (millis() - start) < timeout_ms)
    {
      update();
      delay(10);
    }
    return posllhData.valid;
  }

  bool waitForSatellites(uint32_t timeout_ms)
  {
    unsigned long start = millis();
    while (!satData.valid && (millis() - start) < timeout_ms)
    {
      update();
      delay(10);
    }
    return satData.valid;
  }

  uint8_t getUsedSatelliteCount()
  {
    if (!satData.valid)
      return 0;
    uint8_t count = 0;
    for (int i = 0; i < satData.numSvs; i++)
      if (satData.satellites[i].used)
        count++;
    return count;
  }

  uint8_t getSatelliteCountByGNSS(uint8_t gnssId)
  {
    if (!satData.valid)
      return 0;
    uint8_t count = 0;
    for (int i = 0; i < satData.numSvs; i++)
      if (satData.satellites[i].gnssId == gnssId)
        count++;
    return count;
  }

  uint8_t getUsedSatelliteCountByGNSS(uint8_t gnssId)
  {
    if (!satData.valid)
      return 0;
    uint8_t count = 0;
    for (int i = 0; i < satData.numSvs; i++)
      if (satData.satellites[i].gnssId == gnssId && satData.satellites[i].used)
        count++;
    return count;
  }

  float getAverageCNO()
  {
    if (!satData.valid || satData.numSvs == 0)
      return 0.0;
    float sum = 0;
    for (int i = 0; i < satData.numSvs; i++)
      sum += satData.satellites[i].cno;
    return sum / satData.numSvs;
  }

  bool testCommunication()
  {
    requestVersion();
    delay(200);
    processUBX();
    return true;
  }

  bool isAntennaOK()
  {
    if (!hwData.valid)
    {
      requestHardwareStatus();
      delay(200);
      processUBX();
    }
    return hwData.valid && hwData.aStatus == 2;
  }

  uint8_t getJammingLevel()
  {
    if (!hwData.valid)
    {
      requestHardwareStatus();
      delay(200);
      processUBX();
    }
    return hwData.valid ? hwData.jamInd : 0;
  }

  uint8_t getSignalQuality()
  {
    if (!satData.valid)
      return 0;
    float avgCNO = getAverageCNO();
    uint8_t usedSats = getUsedSatelliteCount();
    uint8_t cnoScore = (avgCNO / 50.0) * 50;
    uint8_t satScore = (usedSats / 12.0) * 50;
    return min(100, cnoScore + satScore);
  }

  bool validateConfiguration()
  {
    if (!testCommunication())
      return false;

    // Wait for hardware status
    requestHardwareStatus();
    delay(300);
    processUBX();

    if (!isAntennaOK())
      return false;

    requestSatellites();
    delay(500);
    processUBX();

    if (!satData.valid || satData.numSvs == 0)
      return false;

    return true;
  }

  void setTimezone(float offsetHours, bool enableAutoDST = false)
  {
    timezoneOffset = offsetHours;
    autoDST = enableAutoDST;
    timeCache.valid = false;
  }

  void getLocalTime(int &year, int &month, int &day, int &dayIndex, int &hour, int &minute, int &second)
  {
    if (!hasFix())
    {
      year = month = day = dayIndex = hour = minute = second = 0;
      return;
    }
    if (!timeCache.valid)
      updateTimeCache();
    if (timeCache.valid)
    {
      year = timeCache.year;
      month = timeCache.month;
      day = timeCache.day;
      dayIndex = timeCache.dayIndex;
      hour = timeCache.hour;
      minute = timeCache.minute;
      second = timeCache.second;
    }
    else
    {
      year = month = day = dayIndex = hour = minute = second = 0;
    }
  }

  int getYear()
  {
    if (!timeCache.valid)
      updateTimeCache();
    return timeCache.valid ? timeCache.year : 0;
  }

  uint8_t getMonth()
  {
    if (!timeCache.valid)
      updateTimeCache();
    return timeCache.valid ? timeCache.month : 0;
  }

  uint8_t getDay()
  {
    if (!timeCache.valid)
      updateTimeCache();
    return timeCache.valid ? timeCache.day : 0;
  }

  uint8_t getHour()
  {
    if (!timeCache.valid)
      updateTimeCache();
    return timeCache.valid ? timeCache.hour : 0;
  }

  uint8_t getMinute()
  {
    if (!timeCache.valid)
      updateTimeCache();
    return timeCache.valid ? timeCache.minute : 0;
  }

  uint8_t getSecond()
  {
    if (!timeCache.valid)
      updateTimeCache();
    return timeCache.valid ? timeCache.second : 0;
  }

  uint8_t getDayIndex()
  {
    if (!timeCache.valid)
      updateTimeCache();
    return timeCache.valid ? timeCache.dayIndex : 0;
  }

  double calculateDistance(double lat1, double lon1, double lat2, double lon2)
  {
    const double R = 6371000.0;
    double lat1Rad = lat1 * DEG_TO_RAD;
    double lat2Rad = lat2 * DEG_TO_RAD;
    double deltaLat = (lat2 - lat1) * DEG_TO_RAD;
    double deltaLon = (lon2 - lon1) * DEG_TO_RAD;
    double a = sin(deltaLat / 2.0) * sin(deltaLat / 2.0) + cos(lat1Rad) * cos(lat2Rad) * sin(deltaLon / 2.0) * sin(deltaLon / 2.0);
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return R * c;
  }

  void setGeofence(double centerLat, double centerLon, float radiusMeters)
  {
    geofence.centerLat = centerLat;
    geofence.centerLon = centerLon;
    geofence.radius = radiusMeters;
    geofence.active = true;
    lastGeofenceState = checkGeofence();
  }

  void disableGeofence()
  {
    geofence.active = false;
  }

  bool isInGeofence()
  {
    return checkGeofence();
  }

  bool geofenceStateChanged()
  {
    if (!geofence.active)
      return false;
    bool current = checkGeofence();
    bool changed = (current != lastGeofenceState);
    lastGeofenceState = current;
    return changed;
  }

  UTMCoordinate convertToUTM(double lat, double lon)
  {
    UTMCoordinate utm;
    utm.zone = (uint8_t)((lon + 180.0) / 6.0) + 1;
    const char bands[] = "CDEFGHJKLMNPQRSTUVWXX";
    int bandIndex = (int)((lat + 80.0) / 8.0);
    if (bandIndex < 0)
      bandIndex = 0;
    if (bandIndex > 20)
      bandIndex = 20;
    utm.band = bands[bandIndex];

    double latRad = lat * DEG_TO_RAD;
    double lonRad = lon * DEG_TO_RAD;
    double lon0 = ((utm.zone - 1) * 6.0 - 180.0 + 3.0) * DEG_TO_RAD;

    const double a = 6378137.0;
    const double e = 0.081819191;
    const double e2 = e * e;
    const double k0 = 0.9996;

    double N = a / sqrt(1.0 - e2 * sin(latRad) * sin(latRad));
    double T = tan(latRad) * tan(latRad);
    double C = e2 * cos(latRad) * cos(latRad) / (1.0 - e2);
    double A = (lonRad - lon0) * cos(latRad);

    double M = a * ((1.0 - e2 / 4.0 - 3.0 * e2 * e2 / 64.0 - 5.0 * e2 * e2 * e2 / 256.0) * latRad - (3.0 * e2 / 8.0 + 3.0 * e2 * e2 / 32.0 + 45.0 * e2 * e2 * e2 / 1024.0) * sin(2.0 * latRad) + (15.0 * e2 * e2 / 256.0 + 45.0 * e2 * e2 * e2 / 1024.0) * sin(4.0 * latRad) - (35.0 * e2 * e2 * e2 / 3072.0) * sin(6.0 * latRad));

    utm.easting = k0 * N * (A + (1.0 - T + C) * A * A * A / 6.0 + (5.0 - 18.0 * T + T * T + 72.0 * C - 58.0 * e2) * A * A * A * A * A / 120.0) + 500000.0;

    utm.northing = k0 * (M + N * tan(latRad) * (A * A / 2.0 + (5.0 - T + 9.0 * C + 4.0 * C * C) * A * A * A * A / 24.0 + (61.0 - 58.0 * T + T * T + 600.0 * C - 330.0 * e2) * A * A * A * A * A * A / 720.0));

    if (lat < 0)
      utm.northing += 10000000.0;
    return utm;
  }

  UTMCoordinate getCurrentUTM()
  {
    return convertToUTM(getLatitude(), getLongitude());
  }
};