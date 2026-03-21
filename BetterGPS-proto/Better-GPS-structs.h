#pragma once
#include <stdint.h>

// ==================== RESPONSE STRUCTURES ====================

struct NAV_POSLLH_Response {
  uint32_t iTOW;   // GPS time of week (ms)
  int32_t lon;     // Longitude (deg * 1e-7)
  int32_t lat;     // Latitude (deg * 1e-7)
  int32_t height;  // Height above ellipsoid (mm)
  int32_t hMSL;    // Height above MSL (mm)
  uint32_t hAcc;   // Horizontal accuracy (mm)
  uint32_t vAcc;   // Vertical accuracy (mm)
  bool valid;
};

struct NAV_STATUS_Response {
  uint32_t iTOW;
  uint8_t gpsFix;   // 0=no fix, 1=DR only, 2=2D, 3=3D, 4=GPS+DR, 5=time only
  uint8_t flags;    // Fix status flags
  uint8_t fixStat;  // Fix status info
  uint8_t flags2;   // Additional flags
  uint32_t ttff;    // Time to first fix (ms)
  uint32_t msss;    // Milliseconds since startup
  bool valid;
};

struct NAV_DOP_Response {
  uint32_t iTOW;
  uint16_t gDOP;  // Geometric DOP * 0.01
  uint16_t pDOP;  // Position DOP * 0.01
  uint16_t tDOP;  // Time DOP * 0.01
  uint16_t vDOP;  // Vertical DOP * 0.01
  uint16_t hDOP;  // Horizontal DOP * 0.01
  uint16_t nDOP;  // Northing DOP * 0.01
  uint16_t eDOP;  // Easting DOP * 0.01
  bool valid;
};

struct NAV_PVT_Response {
  uint32_t iTOW;
  uint16_t year;
  uint8_t month, day, hour, minute, second;
  uint8_t valid;     // Validity flags
  uint32_t tAcc;     // Time accuracy (ns)
  int32_t nano;      // Fraction of second (ns)
  uint8_t fixType;   // Fix type
  uint8_t flags;     // Fix status flags
  uint8_t flags2;    // Additional flags
  uint8_t numSV;     // Number of satellites used
  int32_t lon;       // Longitude (deg * 1e-7)
  int32_t lat;       // Latitude (deg * 1e-7)
  int32_t height;    // Height above ellipsoid (mm)
  int32_t hMSL;      // Height above MSL (mm)
  uint32_t hAcc;     // Horizontal accuracy (mm)
  uint32_t vAcc;     // Vertical accuracy (mm)
  int32_t velN;      // North velocity (mm/s)
  int32_t velE;      // East velocity (mm/s)
  int32_t velD;      // Down velocity (mm/s)
  int32_t gSpeed;    // Ground speed (mm/s)
  int32_t headMot;   // Heading of motion (deg * 1e-5)
  uint32_t sAcc;     // Speed accuracy (mm/s)
  uint32_t headAcc;  // Heading accuracy (deg * 1e-5)
  uint16_t pDOP;     // Position DOP * 0.01
  int32_t headVeh;   // Heading of vehicle (deg * 1e-5)
  bool validData;
};

struct NAV_ODO_Response {
  uint8_t version;
  uint32_t iTOW;
  uint32_t distance;       // Ground distance since reset (m)
  uint32_t totalDistance;  // Total cumulative distance (m)
  uint32_t distanceStd;    // Distance accuracy (m)
  bool valid;
};

struct NAV_VELNED_Response {
  uint32_t iTOW;
  int32_t velN;     // North velocity (cm/s)
  int32_t velE;     // East velocity (cm/s)
  int32_t velD;     // Down velocity (cm/s)
  uint32_t speed;   // 3D speed (cm/s)
  uint32_t gSpeed;  // Ground speed (cm/s)
  int32_t heading;  // Heading (deg * 1e-5)
  uint32_t sAcc;    // Speed accuracy (cm/s)
  uint32_t cAcc;    // Course accuracy (deg * 1e-5)
  bool valid;
};

struct NAV_TIMEUTC_Response {
  uint32_t iTOW;
  uint32_t tAcc;  // Time accuracy (ns)
  int32_t nano;   // Fraction of second (ns)
  uint16_t year;
  uint8_t month, day, hour, min, sec;
  uint8_t valid;  // Validity flags
  bool validData;
};

struct SatelliteInfo {
  uint8_t gnssId;  // 0=GPS, 1=SBAS, 2=Galileo, 3=BeiDou, 5=QZSS, 6=GLONASS
  uint8_t svId;    // Satellite ID
  uint8_t cno;     // Signal strength (dBHz)
  int8_t elev;     // Elevation (-90 to +90 deg)
  int16_t azim;    // Azimuth (0-360 deg)
  int16_t prRes;   // Pseudorange residual
  uint32_t flags;  // Status flags
  bool used;       // Used in navigation solution
};

struct NAV_SAT_Response {
  uint32_t iTOW;
  uint8_t numSvs;
  SatelliteInfo satellites[32];  // Max satellites
  bool valid;
};

struct RawMeasurement {
  double prMes;       // Pseudorange (m)
  double cpMes;       // Carrier phase (cycles)
  float doMes;        // Doppler (Hz)
  uint8_t gnssId;     // GNSS ID
  uint8_t svId;       // Satellite ID
  uint8_t freqId;     // GLONASS frequency slot
  uint16_t locktime;  // Carrier phase locktime (ms)
  uint8_t cno;        // C/N0 (dBHz)
  uint8_t prStdev;    // Pseudorange std dev
  uint8_t cpStdev;    // Carrier phase std dev
  uint8_t doStdev;    // Doppler std dev
  uint8_t trkStat;    // Tracking status
};

struct RXM_RAWX_Response {
  double rcvTow;    // Measurement time of week (s)
  uint16_t week;    // GPS week
  int8_t leapS;     // GPS leap seconds
  uint8_t numMeas;  // Number of measurements
  uint8_t recStat;  // Receiver tracking status
  RawMeasurement measurements[32];
  bool valid;
};

struct RXM_SFRBX_Response {
  uint8_t gnssId;     // GNSS ID
  uint8_t svId;       // Satellite ID
  uint8_t freqId;     // GLONASS frequency slot
  uint8_t numWords;   // Number of data words
  uint8_t chn;        // Channel number
  uint8_t version;    // Message version
  uint32_t dwrd[10];  // Data words
  bool valid;
};

struct MON_HW_Response {
  uint32_t pinSel;      // Pin selection mask
  uint32_t pinBank;     // Pin bank
  uint32_t pinDir;      // Pin direction
  uint32_t pinVal;      // Pin values
  uint16_t noisePerMS;  // Noise level
  uint16_t agcCnt;      // AGC monitor
  uint8_t aStatus;      // Antenna status (0=INIT, 1=DONTKNOW, 2=OK, 3=SHORT, 4=OPEN)
  uint8_t aPower;       // Antenna power (0=OFF, 1=ON, 2=DONTKNOW)
  uint8_t flags;        // Flags
  uint32_t usedMask;    // Pins used mask
  uint8_t jamInd;       // Jamming indicator (0-255)
  bool valid;
};

// ==================== GEOFENCE STRUCTURE ====================
// Geofence => virtual perimeter around geographical location
struct Geofence {
  double centerLat;  // Center latitude
  double centerLon;  // Center longitude
  float radius;      // Radius in meters
  bool active;       // Is geofence active
};

// ==================== COMMAND QUEUE ====================
struct UBXCommand {
  const uint8_t *data;
  uint16_t length;
  uint8_t retries;
  unsigned long timestamp;
  bool awaitingACK;
};

// ==================== UTM COORDINATE ====================
struct UTMCoordinate {
  double easting;
  double northing;
  uint8_t zone;
  char band;
};

// ==================== TIME CACHE ====================
struct TimeCache {
  int year, month, day, dayIndex, hour, minute, second;
  bool valid;
  unsigned long lastUpdate;
};