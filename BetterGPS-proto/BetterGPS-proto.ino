#include "Better-GPS.h"

// ==================== GPS MODULE SELECTION ====================
// Uncomment ONLY the module you are using.
// This controls which features are available in the demo.
//
//  NEO-6M  : GPS only, no PVT, no NAV-SAT, no ODO, no RAWX, no multi-GNSS
//  NEO-7M  : GPS only hardware, PVT available, still no NAV-SAT / ODO / RAWX
//  NEO-8M  : Full feature set, multi-GNSS, NAV-SAT, ODO, RAWX, SFRBX

#define NEO6MV2
// #define NEO7MV2
// #define NEO8MV2

// ==================== FEATURE AVAILABILITY BY MODULE ====================
// These are derived automatically from the module selection above.
// Do NOT edit this block manually.

#if defined(NEO8MV2)
  #define HAS_NAV_SAT       // NAV-SAT  (0x35) - detailed satellite info
  #define HAS_NAV_PVT       // NAV-PVT  (0x07) - position + velocity + time in one
  #define HAS_NAV_ODO       // NAV-ODO  (0x09) - odometer
  #define HAS_RXM_RAWX      // RXM-RAWX (0x15) - raw pseudorange / carrier / Doppler
  #define HAS_RXM_SFRBX     // RXM-SFRBX(0x13) - navigation subframe data
  #define HAS_MULTI_GNSS    // CFG-GNSS - Galileo, GLONASS, BeiDou support
  #define HAS_MON_HW        // MON-HW   (0x09) - hardware / antenna / jamming status
  #define MODULE_NAME "NEO-8M v2"

#elif defined(NEO7MV2)
  #define HAS_NAV_PVT       // NAV-PVT available from u-blox 7
  #define HAS_MON_HW
  // No NAV-SAT, ODO, RAWX, SFRBX, multi-GNSS on NEO-7M
  #define MODULE_NAME "NEO-7M v2"

#elif defined(NEO6MV2)
  // NAV-SAT not available - use NAV-SVINFO (0x30) instead (not implemented here)
  // No PVT, ODO, RAWX, SFRBX, multi-GNSS
  #define HAS_MON_HW
  #define MODULE_NAME "NEO-6M v2"

#else
  #error "No GPS module selected. Uncomment one of: NEO6MV2 / NEO7MV2 / NEO8MV2"
#endif

// ==================== DEMO FEATURE FLAGS ====================
// Comment out any feature you do not want to test.
// Features that are unavailable on your selected module are
// automatically excluded via the HAS_xxx guards above.

// -- GNSS constellations (NEO-8M only) --
#ifdef HAS_MULTI_GNSS
  #define DEMO_GPS_ONLY           // GPS only
  // #define DEMO_GPS_GALILEO     // GPS + Galileo
  // #define DEMO_GPS_GLONASS     // GPS + GLONASS
  // #define DEMO_GPS_BEIDOU      // GPS + BeiDou
  // #define DEMO_ALL_GNSS        // All constellations
#endif

// -- Dynamic platform model --
#define DEMO_MODEL_PORTABLE
// #define DEMO_MODEL_STATIONARY
// #define DEMO_MODEL_PEDESTRIAN
// #define DEMO_MODEL_AUTOMOTIVE

// -- Update rate --
#define DEMO_RATE_1HZ
// #define DEMO_RATE_5HZ
// #define DEMO_RATE_10HZ

// -- Data requests --
#define DEMO_POSITION             // NAV-POSLLH
#define DEMO_STATUS               // NAV-STATUS
#define DEMO_DOP                  // NAV-DOP
#define DEMO_VELOCITY             // NAV-VELNED
#define DEMO_TIMEUTC              // NAV-TIMEUTC
#define DEMO_VERSION              // MON-VER
#define DEMO_CACHE_GETTERS        // getLatitude(), getSpeedKmph(), etc.

#ifdef HAS_NAV_PVT
  #define DEMO_PVT                // NAV-PVT
#endif

#ifdef HAS_NAV_SAT
  #define DEMO_SATELLITES         // NAV-SAT
#endif

#ifdef HAS_NAV_ODO
  #define DEMO_ODOMETER           // NAV-ODO
#endif

#ifdef HAS_RXM_RAWX
  #define DEMO_RAW_MEASUREMENTS   // RXM-RAWX
#endif

#ifdef HAS_RXM_SFRBX
  #define DEMO_SUBFRAME           // RXM-SFRBX
#endif

#ifdef HAS_MON_HW
  #define DEMO_HW_STATUS          // MON-HW (antenna, jamming, noise)
#endif

// -- Time and timezone --
#define DEMO_LOCAL_TIME
#define DEMO_TIMEZONE   1.0       // UTC+1 (Central European Time)
#define DEMO_AUTO_DST   true      // Automatic EU daylight saving time

// -- Position utilities --
#define DEMO_UTM                  // UTM coordinate conversion
#define DEMO_DISTANCE             // Haversine distance calculation

// -- Geofence --
#define DEMO_GEOFENCE
#define DEMO_GEOFENCE_LAT     47.4979   // Budapest, Heroes' Square
#define DEMO_GEOFENCE_LON     19.0402
#define DEMO_GEOFENCE_RADIUS  500.0     // metres

// -- Power and signal --
#define DEMO_POWER_MODE
#define DEMO_SMOOTHING

// -- Validation --
#define DEMO_VALIDATE
#ifdef HAS_MON_HW
  #define DEMO_ANTENNA
  #define DEMO_JAMMING
#endif

// -- Save / reset (disabled by default, uncomment with care) --
// #define DEMO_SAVE_CONFIG
// #define DEMO_RESET_HOT
// #define DEMO_RESET_WARM
// #define DEMO_RESET_COLD

// ==================== HARDWARE PINS ====================
#define GPS_RX_PIN  16
#define GPS_TX_PIN  17

// ==================== LOOP TIMING ====================
#define DEMO_LOOP_INTERVAL_MS  3000
#define DEMO_WAIT_FOR_FIX_MS   15000

// ==================== GLOBAL OBJECT ====================
BetterGPS gps;

// ==================== HELPER FUNCTIONS ====================

void printSeparator(const char* title) {
  Serial.println();
  Serial.print(F("===== "));
  Serial.print(title);
  Serial.println(F(" ====="));
}

void printFixType(FixType ft) {
  switch (ft) {
    case FixType::NoFix:            Serial.print(F("No Fix"));                break;
    case FixType::DeadReckoning:    Serial.print(F("Dead Reckoning"));        break;
    case FixType::Fix2D:            Serial.print(F("2D Fix"));                break;
    case FixType::Fix3D:            Serial.print(F("3D Fix"));                break;
    case FixType::GPSDeadReckoning: Serial.print(F("GPS + Dead Reckoning"));  break;
    case FixType::TimeOnly:         Serial.print(F("Time Only"));             break;
    default:                        Serial.print(F("Unknown"));               break;
  }
}

void printGnssName(uint8_t gnssId) {
  switch (gnssId) {
    case 0: Serial.print(F("GPS"));     break;
    case 1: Serial.print(F("SBAS"));    break;
    case 2: Serial.print(F("Galileo")); break;
    case 3: Serial.print(F("BeiDou"));  break;
    case 5: Serial.print(F("QZSS"));    break;
    case 6: Serial.print(F("GLONASS")); break;
    default: Serial.print(F("Unknown")); break;
  }
}

void printPadded2(uint8_t v) {
  if (v < 10) Serial.print(F("0"));
  Serial.print(v);
}

// ==================== DEMO FUNCTIONS ====================

void demoVersion() {
#ifdef DEMO_VERSION
  printSeparator("MON-VER: Firmware version");
  gps.requestVersion();
  delay(300);
  gps.update();
  Serial.println(F("  Version request sent - response visible on serial monitor"));
#endif
}

void demoStatus() {
#ifdef DEMO_STATUS
  printSeparator("NAV-STATUS: Fix status");
  gps.requestStatus();
  delay(300);
  gps.update();

  if (gps.isStatusReady()) {
    const NAV_STATUS_Response& s = gps.getStatus();
    Serial.print(F("  Fix type  : ")); printFixType(gps.getFixType()); Serial.println();
    Serial.print(F("  gpsFix    : ")); Serial.println(s.gpsFix);
    Serial.print(F("  flags     : 0x")); Serial.println(s.flags, HEX);
    Serial.print(F("  fixStat   : 0x")); Serial.println(s.fixStat, HEX);
    Serial.print(F("  TTFF      : ")); Serial.print(s.ttff); Serial.println(F(" ms"));
    Serial.print(F("  Uptime    : ")); Serial.print(s.msss / 1000); Serial.println(F(" s"));
    Serial.print(F("  hasFix()  : ")); Serial.println(gps.hasFix() ? F("YES") : F("NO"));
  } else {
    Serial.println(F("  No response"));
  }
#endif
}

void demoPosition() {
#ifdef DEMO_POSITION
  printSeparator("NAV-POSLLH: Position");
  gps.requestPosition();
  delay(300);
  gps.update();

  if (gps.isPositionReady()) {
    const NAV_POSLLH_Response& p = gps.getPOSLLH();
    Serial.print(F("  Lat    : ")); Serial.print(p.lat / 1e7, 7);    Serial.println(F(" deg"));
    Serial.print(F("  Lon    : ")); Serial.print(p.lon / 1e7, 7);    Serial.println(F(" deg"));
    Serial.print(F("  hMSL   : ")); Serial.print(p.hMSL / 1000.0, 2); Serial.println(F(" m"));
    Serial.print(F("  height : ")); Serial.print(p.height / 1000.0, 2); Serial.println(F(" m (ellipsoid)"));
    Serial.print(F("  hAcc   : ")); Serial.print(p.hAcc / 1000.0, 2); Serial.println(F(" m"));
    Serial.print(F("  vAcc   : ")); Serial.print(p.vAcc / 1000.0, 2); Serial.println(F(" m"));
  } else {
    Serial.println(F("  No response"));
  }
#endif
}

void demoPVT() {
#ifdef DEMO_PVT
  printSeparator("NAV-PVT: Position + Velocity + Time");
  gps.requestPVT();
  delay(300);
  gps.update();

  if (gps.isPVTReady()) {
    const NAV_PVT_Response& p = gps.getPVT();
    Serial.print(F("  Fix type  : ")); Serial.println(p.fixType);
    Serial.print(F("  Satellites: ")); Serial.println(p.numSV);
    Serial.print(F("  Lat       : ")); Serial.print(p.lat / 1e7, 7);     Serial.println(F(" deg"));
    Serial.print(F("  Lon       : ")); Serial.print(p.lon / 1e7, 7);     Serial.println(F(" deg"));
    Serial.print(F("  hMSL      : ")); Serial.print(p.hMSL / 1000.0, 2); Serial.println(F(" m"));
    Serial.print(F("  hAcc      : ")); Serial.print(p.hAcc / 1000.0, 2); Serial.println(F(" m"));
    Serial.print(F("  vAcc      : ")); Serial.print(p.vAcc / 1000.0, 2); Serial.println(F(" m"));
    Serial.print(F("  Speed     : ")); Serial.print(p.gSpeed / 1000.0, 2); Serial.println(F(" m/s"));
    Serial.print(F("  Heading   : ")); Serial.print(p.headMot / 1e5, 2); Serial.println(F(" deg"));
    Serial.print(F("  pDOP      : ")); Serial.println(p.pDOP / 100.0, 2);
    Serial.print(F("  UTC       : "));
    Serial.print(p.year); Serial.print(F("-"));
    printPadded2(p.month);  Serial.print(F("-"));
    printPadded2(p.day);    Serial.print(F(" "));
    printPadded2(p.hour);   Serial.print(F(":"));
    printPadded2(p.minute); Serial.print(F(":"));
    printPadded2(p.second); Serial.println();
    Serial.print(F("  valid flags: 0x")); Serial.println(p.valid, HEX);
  } else {
    Serial.println(F("  No response"));
  }
#endif
}

void demoDOP() {
#ifdef DEMO_DOP
  printSeparator("NAV-DOP: Dilution of Precision");
  gps.requestDOP();
  delay(300);
  gps.update();

  if (gps.isDOPReady()) {
    const NAV_DOP_Response& d = gps.getDOP();
    Serial.print(F("  gDOP : ")); Serial.println(d.gDOP / 100.0, 2);
    Serial.print(F("  pDOP : ")); Serial.println(d.pDOP / 100.0, 2);
    Serial.print(F("  tDOP : ")); Serial.println(d.tDOP / 100.0, 2);
    Serial.print(F("  vDOP : ")); Serial.println(d.vDOP / 100.0, 2);
    Serial.print(F("  hDOP : ")); Serial.println(d.hDOP / 100.0, 2);
    Serial.print(F("  nDOP : ")); Serial.println(d.nDOP / 100.0, 2);
    Serial.print(F("  eDOP : ")); Serial.println(d.eDOP / 100.0, 2);
    Serial.print(F("  getHDOP(): ")); Serial.println(gps.getHDOP(), 2);
  } else {
    Serial.println(F("  No response"));
  }
#endif
}

void demoVelocity() {
#ifdef DEMO_VELOCITY
  printSeparator("NAV-VELNED: Velocity NED");
  gps.requestVelocity();
  delay(300);
  gps.update();

  if (gps.isVelocityReady()) {
    const NAV_VELNED_Response& v = gps.getVelocity();
    Serial.print(F("  velN      : ")); Serial.print(v.velN / 100.0, 2);  Serial.println(F(" m/s"));
    Serial.print(F("  velE      : ")); Serial.print(v.velE / 100.0, 2);  Serial.println(F(" m/s"));
    Serial.print(F("  velD      : ")); Serial.print(v.velD / 100.0, 2);  Serial.println(F(" m/s"));
    Serial.print(F("  3D speed  : ")); Serial.print(v.speed / 100.0, 2); Serial.println(F(" m/s"));
    Serial.print(F("  gSpeed    : ")); Serial.print(v.gSpeed / 100.0, 2); Serial.println(F(" m/s"));
    Serial.print(F("  heading   : ")); Serial.print(v.heading / 1e5, 2); Serial.println(F(" deg"));
    Serial.print(F("  sAcc      : ")); Serial.print(v.sAcc / 100.0, 2);  Serial.println(F(" m/s"));
    Serial.print(F("  getSpeedMps()  : ")); Serial.println(gps.getSpeedMps(), 2);
    Serial.print(F("  getSpeedKmph() : ")); Serial.println(gps.getSpeedKmph(), 2);
    Serial.print(F("  getCourse()    : ")); Serial.println(gps.getCourse(), 2);
  } else {
    Serial.println(F("  No response"));
  }
#endif
}

void demoTimeUTC() {
#ifdef DEMO_TIMEUTC
  printSeparator("NAV-TIMEUTC: UTC time");
  gps.requestTimeUTC();
  delay(300);
  gps.update();

  if (gps.isTimeUTCReady()) {
    const NAV_TIMEUTC_Response& t = gps.getTimeUTC();
    Serial.print(F("  UTC  : "));
    Serial.print(t.year); Serial.print(F("-"));
    printPadded2(t.month); Serial.print(F("-"));
    printPadded2(t.day);   Serial.print(F(" "));
    printPadded2(t.hour);  Serial.print(F(":"));
    printPadded2(t.min);   Serial.print(F(":"));
    printPadded2(t.sec);   Serial.println();
    Serial.print(F("  tAcc : ")); Serial.print(t.tAcc); Serial.println(F(" ns"));
    Serial.print(F("  nano : ")); Serial.print(t.nano);  Serial.println(F(" ns"));
    Serial.print(F("  valid: 0x")); Serial.println(t.valid, HEX);
  } else {
    Serial.println(F("  No response"));
  }
#endif
}

void demoLocalTime() {
#ifdef DEMO_LOCAL_TIME
  printSeparator("Local time (Timezone + DST)");
  gps.setTimezone(DEMO_TIMEZONE, DEMO_AUTO_DST);

  int yr, mo, dy, di, hr, mn, sc;
  gps.getLocalTime(yr, mo, dy, di, hr, mn, sc);

  const char* dayNames[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
  };

  if (yr > 0) {
    Serial.print(F("  Local : "));
    Serial.print(yr); Serial.print(F("-"));
    printPadded2(mo); Serial.print(F("-"));
    printPadded2(dy); Serial.print(F(" "));
    printPadded2(hr); Serial.print(F(":"));
    printPadded2(mn); Serial.print(F(":"));
    printPadded2(sc); Serial.println();
    Serial.print(F("  Day   : "));
    Serial.println((di >= 0 && di <= 6) ? dayNames[di] : "Unknown");
    Serial.print(F("  getYear()    : ")); Serial.println(gps.getYear());
    Serial.print(F("  getMonth()   : ")); Serial.println(gps.getMonth());
    Serial.print(F("  getDay()     : ")); Serial.println(gps.getDay());
    Serial.print(F("  getHour()    : ")); Serial.println(gps.getHour());
    Serial.print(F("  getMinute()  : ")); Serial.println(gps.getMinute());
    Serial.print(F("  getSecond()  : ")); Serial.println(gps.getSecond());
    Serial.print(F("  getDayIndex(): ")); Serial.println(gps.getDayIndex());
  } else {
    Serial.println(F("  No time data (fix required)"));
  }
#endif
}

void demoSatellites() {
#ifdef DEMO_SATELLITES
  printSeparator("NAV-SAT: Satellite info");
  gps.requestSatellites();
  delay(500);
  gps.update();

  if (gps.isSatellitesReady()) {
    const NAV_SAT_Response& s = gps.getSatellites();
    Serial.print(F("  Total visible  : ")); Serial.println(s.numSvs);
    Serial.print(F("  Used in fix    : ")); Serial.println(gps.getUsedSatelliteCount());
    Serial.print(F("  Average CNO    : ")); Serial.print(gps.getAverageCNO(), 1); Serial.println(F(" dBHz"));
    Serial.print(F("  Signal quality : ")); Serial.print(gps.getSignalQuality()); Serial.println(F("/100"));

    // Per-constellation breakdown
    Serial.print(F("  GPS     visible: ")); Serial.print(gps.getSatelliteCountByGNSS(0));
    Serial.print(F("  used: ")); Serial.println(gps.getUsedSatelliteCountByGNSS(0));
    Serial.print(F("  Galileo visible: ")); Serial.print(gps.getSatelliteCountByGNSS(2));
    Serial.print(F("  used: ")); Serial.println(gps.getUsedSatelliteCountByGNSS(2));
    Serial.print(F("  BeiDou  visible: ")); Serial.print(gps.getSatelliteCountByGNSS(3));
    Serial.print(F("  used: ")); Serial.println(gps.getUsedSatelliteCountByGNSS(3));
    Serial.print(F("  GLONASS visible: ")); Serial.print(gps.getSatelliteCountByGNSS(6));
    Serial.print(F("  used: ")); Serial.println(gps.getUsedSatelliteCountByGNSS(6));

    // Detailed list (up to 12 satellites with signal)
    Serial.println(F("  GNSS     PRN  CNO  Elev  Azim  Used"));
    uint8_t shown = 0;
    for (int i = 0; i < s.numSvs && shown < 12; i++) {
      const SatelliteInfo& sv = s.satellites[i];
      if (sv.cno == 0) continue;
      Serial.print(F("  "));
      printGnssName(sv.gnssId);
      Serial.print(F("  "));
      if (sv.svId < 10) Serial.print(F(" "));
      Serial.print(sv.svId);
      Serial.print(F("   "));
      if (sv.cno < 10) Serial.print(F(" "));
      Serial.print(sv.cno);
      Serial.print(F("   "));
      if (sv.elev >= 0 && sv.elev < 10) Serial.print(F(" "));
      Serial.print(sv.elev);
      Serial.print(F("    "));
      if (sv.azim < 100) Serial.print(F(" "));
      if (sv.azim < 10)  Serial.print(F(" "));
      Serial.print(sv.azim);
      Serial.print(F("    "));
      Serial.println(sv.used ? F("yes") : F("no"));
      shown++;
    }
  } else {
    Serial.println(F("  No response"));
  }
#endif
}

void demoOdometer() {
#ifdef DEMO_ODOMETER
  printSeparator("NAV-ODO: Odometer (NEO-8M only)");
  gps.requestOdometer();
  delay(300);
  gps.update();

  if (gps.isOdometerReady()) {
    const NAV_ODO_Response& o = gps.getOdometer();
    Serial.print(F("  Distance since reset : ")); Serial.print(o.distance); Serial.println(F(" m"));
    Serial.print(F("  Total distance       : ")); Serial.print(o.totalDistance); Serial.println(F(" m"));
    Serial.print(F("  Distance accuracy    : ")); Serial.print(o.distanceStd); Serial.println(F(" m"));
    Serial.print(F("  Message version      : ")); Serial.println(o.version);
  } else {
    Serial.println(F("  No response"));
  }
#endif
}

void demoRawMeasurements() {
#ifdef DEMO_RAW_MEASUREMENTS
  printSeparator("RXM-RAWX: Raw measurements (NEO-8M only)");
  gps.requestRawMeasurements();
  delay(500);
  gps.update();

  if (gps.isRawMeasurementsReady()) {
    const RXM_RAWX_Response& r = gps.getRawMeasurements();
    Serial.print(F("  rcvTow     : ")); Serial.print(r.rcvTow, 3); Serial.println(F(" s"));
    Serial.print(F("  GPS week   : ")); Serial.println(r.week);
    Serial.print(F("  Leap secs  : ")); Serial.println(r.leapS);
    Serial.print(F("  Measurements: ")); Serial.println(r.numMeas);
    Serial.print(F("  recStat    : 0x")); Serial.println(r.recStat, HEX);

    Serial.println(F("  --- First measurements ---"));
    for (int i = 0; i < r.numMeas && i < 4; i++) {
      const RawMeasurement& m = r.measurements[i];
      Serial.print(F("  ["));  Serial.print(i); Serial.print(F("] "));
      printGnssName(m.gnssId);
      Serial.print(F(" PRN:")); Serial.print(m.svId);
      Serial.print(F("  PR:")); Serial.print(m.prMes, 1); Serial.print(F("m"));
      Serial.print(F("  CP:")); Serial.print(m.cpMes, 1); Serial.print(F("cyc"));
      Serial.print(F("  DO:")); Serial.print(m.doMes, 1); Serial.print(F("Hz"));
      Serial.print(F("  CNO:")); Serial.print(m.cno);     Serial.print(F("dB"));
      Serial.print(F("  lock:")); Serial.print(m.locktime); Serial.println(F("ms"));
    }
  } else {
    Serial.println(F("  No response"));
  }
#endif
}

void demoSubframe() {
#ifdef DEMO_SUBFRAME
  printSeparator("RXM-SFRBX: Navigation subframe (NEO-8M only)");
  gps.requestSubframeData();
  delay(300);
  gps.update();

  if (gps.isSubframeDataReady()) {
    const RXM_SFRBX_Response& sf = gps.getSubframeData();
    Serial.print(F("  gnssId   : ")); printGnssName(sf.gnssId); Serial.println();
    Serial.print(F("  svId     : ")); Serial.println(sf.svId);
    Serial.print(F("  numWords : ")); Serial.println(sf.numWords);
    Serial.print(F("  channel  : ")); Serial.println(sf.chn);
    Serial.print(F("  version  : ")); Serial.println(sf.version);
    Serial.print(F("  dwrd[0]  : 0x")); Serial.println(sf.dwrd[0], HEX);
  } else {
    Serial.println(F("  No response"));
  }
#endif
}

void demoHWStatus() {
#ifdef DEMO_HW_STATUS
  printSeparator("MON-HW: Hardware status");
  gps.requestHardwareStatus();
  delay(300);
  gps.update();

  if (gps.isHardwareStatusReady()) {
    const MON_HW_Response& hw = gps.getHardwareStatus();

    Serial.print(F("  Antenna status : "));
    switch (hw.aStatus) {
      case 0: Serial.println(F("INIT"));        break;
      case 1: Serial.println(F("UNKNOWN"));     break;
      case 2: Serial.println(F("OK"));          break;
      case 3: Serial.println(F("SHORT"));       break;
      case 4: Serial.println(F("OPEN"));        break;
      default: Serial.println(hw.aStatus);      break;
    }

    Serial.print(F("  Antenna power  : "));
    switch (hw.aPower) {
      case 0: Serial.println(F("OFF"));         break;
      case 1: Serial.println(F("ON"));          break;
      case 2: Serial.println(F("UNKNOWN"));     break;
      default: Serial.println(hw.aPower);       break;
    }

    Serial.print(F("  Noise level    : ")); Serial.println(hw.noisePerMS);
    Serial.print(F("  AGC counter    : ")); Serial.println(hw.agcCnt);
    Serial.print(F("  Jamming index  : ")); Serial.print(hw.jamInd);
    if      (hw.jamInd < 50)  Serial.println(F(" (none)"));
    else if (hw.jamInd < 150) Serial.println(F(" (weak)"));
    else if (hw.jamInd < 200) Serial.println(F(" (moderate)"));
    else                       Serial.println(F(" (STRONG - warning)"));
    Serial.print(F("  flags          : 0x")); Serial.println(hw.flags, HEX);

    Serial.print(F("  isAntennaOK()   : ")); Serial.println(gps.isAntennaOK()    ? F("YES") : F("NO"));
    Serial.print(F("  getJammingLevel(): ")); Serial.println(gps.getJammingLevel());
  } else {
    Serial.println(F("  No response"));
  }
#endif
}

void demoCacheGetters() {
#ifdef DEMO_CACHE_GETTERS
  printSeparator("Position cache getters");
  Serial.print(F("  getLatitude()       : ")); Serial.println(gps.getLatitude(), 7);
  Serial.print(F("  getLongitude()      : ")); Serial.println(gps.getLongitude(), 7);
  Serial.print(F("  getAltitude()       : ")); Serial.print(gps.getAltitude(), 2);   Serial.println(F(" m"));
  Serial.print(F("  getSpeedKmph()      : ")); Serial.print(gps.getSpeedKmph(), 2);  Serial.println(F(" km/h"));
  Serial.print(F("  getSpeedMps()       : ")); Serial.print(gps.getSpeedMps(), 2);   Serial.println(F(" m/s"));
  Serial.print(F("  getCourse()         : ")); Serial.print(gps.getCourse(), 2);     Serial.println(F(" deg"));
  Serial.print(F("  getSatelliteCount() : ")); Serial.println(gps.getSatelliteCount());
  Serial.print(F("  getHDOP()           : ")); Serial.println(gps.getHDOP(), 2);
  Serial.print(F("  getFixType()        : ")); printFixType(gps.getFixType());       Serial.println();
  Serial.print(F("  hasFix()            : ")); Serial.println(gps.hasFix() ? F("YES") : F("NO"));
#endif
}

void demoUTM() {
#ifdef DEMO_UTM
  printSeparator("UTM coordinate conversion");
  if (gps.hasFix()) {
    UTMCoordinate utm = gps.getCurrentUTM();
    Serial.print(F("  Current position -> Zone "));
    Serial.print(utm.zone); Serial.println(utm.band);
    Serial.print(F("  Easting  : ")); Serial.print(utm.easting, 1);  Serial.println(F(" m"));
    Serial.print(F("  Northing : ")); Serial.print(utm.northing, 1); Serial.println(F(" m"));

    // Reference conversion: Budapest Hősök tere
    UTMCoordinate ref = gps.convertToUTM(47.4979, 19.0402);
    Serial.print(F("  Budapest Hosok tere -> "));
    Serial.print(ref.zone); Serial.print(ref.band);
    Serial.print(F("  E:")); Serial.print(ref.easting, 0);
    Serial.print(F("  N:")); Serial.println(ref.northing, 0);
  } else {
    Serial.println(F("  Fix required for UTM conversion"));
  }
#endif
}

void demoDistance() {
#ifdef DEMO_DISTANCE
  printSeparator("Distance calculation (Haversine)");
  if (gps.hasFix()) {
    // Distance from current position to Budapest Heroes' Square
    double dist = gps.calculateDistance(
      gps.getLatitude(), gps.getLongitude(),
      47.4979, 19.0402
    );
    Serial.print(F("  Distance to Budapest Hosok tere: "));
    if (dist >= 1000.0) {
      Serial.print(dist / 1000.0, 2); Serial.println(F(" km"));
    } else {
      Serial.print(dist, 1); Serial.println(F(" m"));
    }

    // Known reference distance: Budapest to Debrecen (~185 km)
    double ref = gps.calculateDistance(47.4979, 19.0402, 47.5316, 21.6273);
    Serial.print(F("  Budapest - Debrecen: "));
    Serial.print(ref / 1000.0, 1);
    Serial.println(F(" km  (reference: ~185 km)"));
  } else {
    Serial.println(F("  Fix required for distance calculation"));
  }
#endif
}

void demoGeofence() {
#ifdef DEMO_GEOFENCE
  printSeparator("Geofence");
  gps.setGeofence(DEMO_GEOFENCE_LAT, DEMO_GEOFENCE_LON, DEMO_GEOFENCE_RADIUS);

  Serial.print(F("  Centre  : ")); Serial.print(DEMO_GEOFENCE_LAT, 4);
  Serial.print(F(", "));          Serial.println(DEMO_GEOFENCE_LON, 4);
  Serial.print(F("  Radius  : ")); Serial.print(DEMO_GEOFENCE_RADIUS); Serial.println(F(" m"));

  if (gps.hasFix()) {
    Serial.print(F("  Status  : ")); Serial.println(gps.isInGeofence() ? F("INSIDE") : F("OUTSIDE"));
    Serial.print(F("  Changed : ")); Serial.println(gps.geofenceStateChanged() ? F("YES (boundary crossed)") : F("NO"));
  } else {
    Serial.println(F("  Fix required for geofence check"));
  }

  gps.disableGeofence();
  Serial.println(F("  Geofence disabled"));
#endif
}

void demoPowerMode() {
#ifdef DEMO_POWER_MODE
  printSeparator("Power mode");
  Serial.println(F("  Switching to power save..."));
  gps.setPowerMode(PowerMode::PowerSave);
  delay(500);
  Serial.println(F("  Switching back to continuous..."));
  gps.setPowerMode(PowerMode::Continuous);
  delay(200);
  Serial.println(F("  Done"));
#endif
}

void demoSmoothing() {
#ifdef DEMO_SMOOTHING
  printSeparator("Position smoothing (CFG-NAVX5)");
  Serial.println(F("  Enabling smoothing..."));
  gps.setSmoothing(true);
  delay(200);
  Serial.println(F("  Disabling smoothing..."));
  gps.setSmoothing(false);
  delay(200);
  Serial.println(F("  Done"));
#endif
}

void demoGNSSConfig() {
  printSeparator("GNSS constellation configuration");

#if defined(DEMO_ALL_GNSS)
  Serial.println(F("  Setting GPS + Galileo + GLONASS + BeiDou..."));
  gps.setGNSS(true, true, true, true);
#elif defined(DEMO_GPS_GALILEO)
  Serial.println(F("  Setting GPS + Galileo..."));
  gps.setGNSS(true, true, false, false);
#elif defined(DEMO_GPS_GLONASS)
  Serial.println(F("  Setting GPS + GLONASS..."));
  gps.setGNSS(true, false, true, false);
#elif defined(DEMO_GPS_BEIDOU)
  Serial.println(F("  Setting GPS + BeiDou..."));
  gps.setGNSS(true, false, false, true);
#else
  Serial.println(F("  Setting GPS only..."));
  gps.setGNSS(true, false, false, false);
#endif
  delay(300);
  Serial.println(F("  Done"));
}

void demoNavModel() {
  printSeparator("Dynamic platform model (CFG-NAV5)");

#if defined(DEMO_MODEL_AUTOMOTIVE)
  Serial.println(F("  Setting Automotive..."));
  gps.setDynamicModel(DynamicModel::Automotive);
#elif defined(DEMO_MODEL_PEDESTRIAN)
  Serial.println(F("  Setting Pedestrian..."));
  gps.setDynamicModel(DynamicModel::Pedestrian);
#elif defined(DEMO_MODEL_STATIONARY)
  Serial.println(F("  Setting Stationary..."));
  gps.setDynamicModel(DynamicModel::Stationary);
#else
  Serial.println(F("  Setting Portable (default)..."));
  gps.setDynamicModel(DynamicModel::Portable);
#endif
  Serial.println(F("  Done"));
}

void demoUpdateRate() {
  printSeparator("Update rate (CFG-RATE)");

#if defined(DEMO_RATE_10HZ)
  Serial.println(F("  Setting 10 Hz..."));
  gps.setUpdateRate(UpdateRate::HZ_10);
#elif defined(DEMO_RATE_5HZ)
  Serial.println(F("  Setting 5 Hz..."));
  gps.setUpdateRate(UpdateRate::HZ_5);
#else
  Serial.println(F("  Setting 1 Hz (default)..."));
  gps.setUpdateRate(UpdateRate::HZ_1);
#endif
  Serial.println(F("  Done"));
}

void demoValidate() {
#ifdef DEMO_VALIDATE
  printSeparator("Configuration validation");
  Serial.print(F("  testCommunication()     : "));
  Serial.println(gps.testCommunication() ? F("OK") : F("FAILED"));
  Serial.print(F("  validateConfiguration() : "));
  Serial.println(gps.validateConfiguration() ? F("OK") : F("FAILED (fix or antenna required)"));
#endif
}

void demoSaveConfig() {
#ifdef DEMO_SAVE_CONFIG
  printSeparator("Save configuration (CFG-CFG)");
  Serial.println(F("  Saving to Flash + BBR..."));
  gps.saveConfig(SaveLocation::Both);
  Serial.println(F("  Done"));
#endif
}

void demoResets() {
#ifdef DEMO_RESET_HOT
  printSeparator("Hot reset (CFG-RST)");
  Serial.println(F("  Sending hot reset... (1 s delay)"));
  gps.resetHot();
  Serial.println(F("  Done"));
#endif

#ifdef DEMO_RESET_WARM
  printSeparator("Warm reset (CFG-RST)");
  Serial.println(F("  Sending warm reset... (2 s delay)"));
  gps.resetWarm();
  Serial.println(F("  Done"));
#endif

#ifdef DEMO_RESET_COLD
  printSeparator("Cold reset (CFG-RST)");
  Serial.println(F("  Sending cold reset... (3 s delay)"));
  gps.resetCold();
  Serial.println(F("  Done"));
#endif
}

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("\n========================================"));
  Serial.println(F("       BetterGPS - Full Feature Demo"));
  Serial.print(F("       Module: ")); Serial.println(F(MODULE_NAME));
  Serial.println(F("========================================"));
  Serial.print(F("  RX pin : ")); Serial.println(GPS_RX_PIN);
  Serial.print(F("  TX pin : ")); Serial.println(GPS_TX_PIN);

  Serial.println(F("\nInitializing GPS (autobaud detection)..."));
  gps.begin(GPS_RX_PIN, GPS_TX_PIN);
  Serial.println(F("GPS initialized."));

  // One-time configuration
  demoGNSSConfig();
  demoNavModel();
  demoUpdateRate();

  // Wait for fix
  Serial.println(F("\nWaiting for fix..."));
  unsigned long waitStart = millis();
  while (!gps.hasFix() && millis() - waitStart < DEMO_WAIT_FOR_FIX_MS) {
    gps.update();
    if ((millis() - waitStart) % 2000 < 50) Serial.print(F("."));
    delay(50);
  }
  Serial.println();

  if (gps.hasFix()) {
    Serial.println(F("Fix acquired."));
  } else {
    Serial.println(F("No fix within time limit. Demo will continue anyway."));
  }
}

// ==================== LOOP ====================

void loop() {
  gps.update();

  static unsigned long lastDemo = 0;
  if (millis() - lastDemo < DEMO_LOOP_INTERVAL_MS) return;
  lastDemo = millis();

  Serial.println(F("\n########################################"));
  Serial.println(F("           DEMO CYCLE START"));
  Serial.println(F("########################################"));

  demoVersion();
  demoStatus();
  demoPosition();
  demoPVT();
  demoDOP();
  demoVelocity();
  demoTimeUTC();
  demoLocalTime();
  demoSatellites();
  demoOdometer();
  demoRawMeasurements();
  demoSubframe();
  demoHWStatus();
  demoCacheGetters();
  demoUTM();
  demoDistance();
  demoGeofence();
  demoPowerMode();
  demoSmoothing();
  demoValidate();
  demoSaveConfig();
  demoResets();

  Serial.println(F("\n########################################"));
  Serial.println(F("            DEMO CYCLE END"));
  Serial.println(F("########################################"));
}