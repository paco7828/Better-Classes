#pragma once

// ==================== ENUMS ====================
enum class UpdateRate : uint8_t {
  HZ_1 = 1,
  HZ_5 = 5,
  HZ_10 = 10
};

enum class SaveLocation : uint8_t {
  Flash = 0,
  BBR = 1,
  Both = 2
};

enum class FixType : uint8_t {
  NoFix = 0,
  DeadReckoning = 1,
  Fix2D = 2,
  Fix3D = 3,
  GPSDeadReckoning = 4,
  TimeOnly = 5
};

enum class PowerMode : uint8_t {
  Continuous = 0,
  PowerSave = 1
};

enum class DynamicModel : uint8_t {
  Portable = 0,
  Stationary = 2,
  Pedestrian = 3,
  Automotive = 4,
  Sea = 5,
  Airborne1g = 6,
  Airborne2g = 7,
  Airborne4g = 8
};

// ==================== CONFIGURATION ====================
#define UPDATE_RATE UpdateRate::HZ_1

// Startup data logging
#define STARTUP_POSITION false
#define STARTUP_STATUS false
#define STARTUP_SATELLITE_INFO false
#define STARTUP_VERSION false

// Power saving
#define POWER_SAVING_ON false

// Flash = 0; BBR = 1; Both = 2;
#define DEFAULT_SAVE_LOCATION SaveLocation::Both

// Response types
#define DISABLE_GGA true
#define DISABLE_GLL true
#define DISABLE_GSA true
#define DISABLE_GSV true
#define DISABLE_RMC true
#define DISABLE_VTG true

// GNSS Configuration
// GPS: Global Positioning System (USA) - Most widely supported, 31 satellites
// Galileo: European GNSS - Better accuracy in urban areas, 30 satellites
// GLONASS: Russian GNSS - Better coverage at high latitudes, 24 satellites
// BeiDou: Chinese GNSS - Good Asia-Pacific coverage, 35+ satellites
//
// Multiple constellations
// - More satellites = better accuracy and faster fix acquisition
// - Better coverage in difficult environments (urban canyons, forests)
// - Redundancy if one constellation has issues
// - Can improve position accuracy from ~5m to ~1-2m
#define GPS_ENABLED true
#define GALILEO_ENABLED false
#define GLONASS_ENABLED false
#define BEIDOU_ENABLED false

// Smoothing on startup
#define STARTUP_SMOOTHING false

// Timeout for UBX responses (milliseconds)
#define UBX_RESPONSE_TIMEOUT 1000

// Timezone configuration (offset from UTC in hours)
// Examples: UTC+1 = 1, UTC-5 = -5, UTC+5:30 = 5.5
#define TIMEZONE_OFFSET 1.0
#define AUTO_DST true  // Automatically handle daylight saving time (EU rules)