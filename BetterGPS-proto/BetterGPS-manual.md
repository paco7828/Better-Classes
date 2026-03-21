# BetterGPS

A full-featured Arduino library for u-blox GPS modules using the UBX binary protocol. Supports auto baud detection, position, velocity, time, satellite info, raw measurements, geofencing, UTM conversion, timezone handling with automatic DST, and more.

---

## Table of Contents

- [Supported Modules](#supported-modules)
- [File Structure](#file-structure)
- [Wiring](#wiring)
- [Quick Start](#quick-start)
- [Configuration](#configuration)
- [Feature Reference](#feature-reference)
  - [Initialization](#initialization)
  - [Update Loop](#update-loop)
  - [Fix and Status](#fix-and-status)
  - [Position](#position)
  - [Velocity](#velocity)
  - [Time and Timezone](#time-and-timezone)
  - [PVT — Position Velocity Time](#pvt--position-velocity-time)
  - [Dilution of Precision](#dilution-of-precision)
  - [Satellite Information](#satellite-information)
  - [Odometer](#odometer)
  - [Raw Measurements](#raw-measurements)
  - [Subframe Data](#subframe-data)
  - [Hardware Status](#hardware-status)
  - [Geofence](#geofence)
  - [UTM Coordinates](#utm-coordinates)
  - [Distance Calculation](#distance-calculation)
  - [GNSS Constellation Configuration](#gnss-constellation-configuration)
  - [Dynamic Platform Model](#dynamic-platform-model)
  - [Update Rate](#update-rate)
  - [Power Mode](#power-mode)
  - [Position Smoothing](#position-smoothing)
  - [Configuration Save](#configuration-save)
  - [Reset](#reset)
  - [Custom UBX Commands](#custom-ubx-commands)
  - [Validation](#validation)
- [Response Structures](#response-structures)
- [Enums](#enums)
- [Feature Availability by Module](#feature-availability-by-module)
- [Demo Sketch](#demo-sketch)

---

## Supported Modules

| Module | GPS | PVT | NAV-SAT | ODO | RAWX | Multi-GNSS |
|--------|-----|-----|---------|-----|------|------------|
| NEO-6M v2 | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ |
| NEO-7M v2 | ✓ | ✓ | ✗ | ✗ | ✗ | ✗ |
| NEO-8M v2 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |

> The library works on all three. Features unavailable on your module will return no response — the rest of the library is unaffected.

---

## File Structure

```
Better-GPS.h            — Main library class (all methods)
Better-GPS-commands.h   — Pre-built UBX binary command packets
Better-GPS-config.h     — User configuration (update rate, GNSS, timezone, etc.)
Better-GPS-structs.h    — Response data structures and enums
BetterGPS-demo.ino      — Full feature demo sketch
```

All five files must be in the same directory as your `.ino` sketch.

---

## Wiring

Connect your u-blox module to any two free UART pins on your ESP32 (or other HardwareSerial-capable board):

```
GPS TX  →  ESP32 RX pin  (defined as GPS_RX_PIN)
GPS RX  →  ESP32 TX pin  (defined as GPS_TX_PIN)
GPS VCC →  3.3 V
GPS GND →  GND
```

> The library uses `HardwareSerial(1)` internally. Do not use pins already assigned to Serial0.

---

## Quick Start

```cpp
#include "Better-GPS.h"

BetterGPS gps;

void setup() {
  Serial.begin(115200);
  gps.begin(16, 17);  // RX pin, TX pin
}

void loop() {
  gps.update();

  if (gps.hasFix()) {
    Serial.print("Lat: "); Serial.println(gps.getLatitude(), 7);
    Serial.print("Lon: "); Serial.println(gps.getLongitude(), 7);
    Serial.print("Alt: "); Serial.print(gps.getAltitude(), 2);
    Serial.println(" m");
  }

  delay(1000);
}
```

---

## Configuration

Edit `Better-GPS-config.h` to configure the library behaviour at compile time.

```cpp
// Navigation update rate sent to the GPS module
#define UPDATE_RATE UpdateRate::HZ_1   // HZ_1 / HZ_5 / HZ_10

// Request and print these on startup
#define STARTUP_POSITION        false
#define STARTUP_STATUS          false
#define STARTUP_SATELLITE_INFO  false
#define STARTUP_VERSION         false

// Enable GPS power saving mode on startup
#define POWER_SAVING_ON         false

// Where to persist configuration after begin()
#define DEFAULT_SAVE_LOCATION   SaveLocation::Both  // Flash / BBR / Both

// Disable individual NMEA sentence types (set true to suppress)
#define DISABLE_GGA  false
#define DISABLE_GLL  false
#define DISABLE_GSA  false
#define DISABLE_GSV  false
#define DISABLE_RMC  false
#define DISABLE_VTG  false

// GNSS constellations (NEO-8M only)
#define GPS_ENABLED      true
#define GALILEO_ENABLED  false
#define GLONASS_ENABLED  false
#define BEIDOU_ENABLED   false

// Apply position smoothing on startup (CFG-NAVX5)
#define STARTUP_SMOOTHING  false

// How long to wait for a UBX ACK / response in milliseconds
#define UBX_RESPONSE_TIMEOUT  300

// Local timezone offset from UTC in hours
// Examples: 1.0 = UTC+1,  -5.0 = UTC-5,  5.5 = UTC+5:30
#define TIMEZONE_OFFSET  1.0
#define AUTO_DST         true   // Apply EU daylight saving time rules automatically
```

---

## Feature Reference

### Initialization

```cpp
gps.begin(int rxPin, int txPin);
```

Performs automatic baud rate detection across all standard rates (4800 – 460800), then applies the startup configuration defined in `Better-GPS-config.h`. Always call this in `setup()` before any other method.

---

### Update Loop

```cpp
gps.update();
```

Must be called regularly in `loop()`. Reads all available bytes from the GPS UART, parses complete UBX frames, dispatches them to the correct response structures, processes the command queue, and triggers the auto-position-request when a fix is active.

---

### Fix and Status

```cpp
bool hasFix();
FixType getFixType();
void requestStatus();
bool isStatusReady();
const NAV_STATUS_Response& getStatus() const;
```

`hasFix()` returns `true` if NAV-STATUS reports a 2D or 3D fix, or if NAV-PVT confirms a fix, or if the position cache is valid and was updated within the last 5 seconds.

`getFixType()` returns a `FixType` enum value:

```cpp
if (gps.getFixType() == FixType::Fix3D) {
  // full 3D fix
}
```

`getStatus()` returns a const reference to the full NAV-STATUS payload including time-to-first-fix (`ttff`) and milliseconds since startup (`msss`).

---

### Position

```cpp
void requestPosition();
bool isPositionReady();
const NAV_POSLLH_Response& getPOSLLH() const;

double getLatitude();   // degrees
double getLongitude();  // degrees
double getAltitude();   // metres above MSL
```

`requestPosition()` sends a NAV-POSLLH poll. The response is parsed automatically on the next `update()` call. Use `isPositionReady()` to check whether fresh data has arrived.

The quick getters (`getLatitude()` etc.) read from a position cache that is updated whenever NAV-POSLLH or NAV-PVT data arrives.

```cpp
gps.requestPosition();
delay(300);
gps.update();

if (gps.isPositionReady()) {
  const NAV_POSLLH_Response& p = gps.getPOSLLH();
  Serial.println(p.lat / 1e7, 7);    // latitude in degrees
  Serial.println(p.hMSL / 1000.0);   // altitude in metres
}
```

A blocking helper is also available if you want to wait for the first position before continuing:

```cpp
bool waitForPosition(uint32_t timeout_ms);
```

---

### Velocity

```cpp
void requestVelocity();
bool isVelocityReady();
const NAV_VELNED_Response& getVelocity() const;

double getSpeedKmph();
double getSpeedMps();
double getCourse();     // heading of motion in degrees
```

Returns velocity in the North / East / Down frame. All raw values are in cm/s; the quick getters convert automatically.

---

### Time and Timezone

```cpp
void requestTimeUTC();
bool isTimeUTCReady();
const NAV_TIMEUTC_Response& getTimeUTC() const;

void setTimezone(float offsetHours, bool enableAutoDST = false);
void getLocalTime(int &year, int &month, int &day, int &dayIndex,
                  int &hour, int &minute, int &second);

int     getYear();
uint8_t getMonth();
uint8_t getDay();
uint8_t getHour();
uint8_t getMinute();
uint8_t getSecond();
uint8_t getDayIndex();  // 0 = Sunday … 6 = Saturday
```

The timezone offset and DST flag are initialised from `Better-GPS-config.h` but can be changed at runtime with `setTimezone()`. When `AUTO_DST` is `true`, EU daylight saving rules are applied automatically (last Sunday in March / October, 01:00 UTC transition).

```cpp
gps.setTimezone(1.0, true);  // UTC+1, EU DST

int yr, mo, dy, di, hr, mn, sc;
gps.getLocalTime(yr, mo, dy, di, hr, mn, sc);
// di: 0 = Sunday, 1 = Monday … 6 = Saturday
```

---

### PVT — Position Velocity Time

*Available on NEO-7M v2 and NEO-8M v2 only.*

```cpp
void requestPVT();
bool isPVTReady();
const NAV_PVT_Response& getPVT() const;
```

NAV-PVT combines position, velocity, and time into a single message. It is the most comprehensive single poll available and also feeds the position cache and time cache automatically when it arrives.

```cpp
gps.requestPVT();
delay(300);
gps.update();

if (gps.isPVTReady()) {
  const NAV_PVT_Response& p = gps.getPVT();
  Serial.println(p.gSpeed / 1000.0);  // ground speed in m/s
  Serial.println(p.numSV);            // number of satellites used
}
```

---

### Dilution of Precision

```cpp
void requestDOP();
bool isDOPReady();
const NAV_DOP_Response& getDOP() const;
double getHDOP();
```

All raw DOP values are integers scaled by 0.01 (e.g. a raw value of `142` means a DOP of `1.42`). `getHDOP()` returns the horizontal DOP as a double, or `99.99` if no DOP data is available.

---

### Satellite Information

*NAV-SAT is available on NEO-8M v2 only. The NEO-6M uses the older NAV-SVINFO (0x30), which is not implemented in this library.*

```cpp
void requestSatellites();
bool isSatellitesReady();
bool waitForSatellites(uint32_t timeout_ms);
const NAV_SAT_Response& getSatellites() const;

uint8_t getSatelliteCount();
uint8_t getUsedSatelliteCount();
uint8_t getSatelliteCountByGNSS(uint8_t gnssId);
uint8_t getUsedSatelliteCountByGNSS(uint8_t gnssId);
float   getAverageCNO();
uint8_t getSignalQuality();   // composite score 0–100
```

GNSS ID values: `0` = GPS, `1` = SBAS, `2` = Galileo, `3` = BeiDou, `5` = QZSS, `6` = GLONASS.

```cpp
gps.requestSatellites();
delay(500);
gps.update();

const NAV_SAT_Response& s = gps.getSatellites();
for (int i = 0; i < s.numSvs; i++) {
  const SatelliteInfo& sv = s.satellites[i];
  // sv.gnssId, sv.svId, sv.cno (dBHz),
  // sv.elev (deg), sv.azim (deg), sv.used
}
```

---

### Odometer

*NEO-8M v2 only.*

```cpp
void requestOdometer();
bool isOdometerReady();
const NAV_ODO_Response& getOdometer() const;
```

Returns distance travelled since the last reset and total cumulative distance, both in metres.

---

### Raw Measurements

*NEO-8M v2 only.*

```cpp
void requestRawMeasurements();
bool isRawMeasurementsReady();
const RXM_RAWX_Response& getRawMeasurements() const;
```

Provides pseudorange, carrier phase, and Doppler measurements for each tracked signal. Useful for post-processing or precise positioning algorithms.

```cpp
const RXM_RAWX_Response& r = gps.getRawMeasurements();
for (int i = 0; i < r.numMeas; i++) {
  const RawMeasurement& m = r.measurements[i];
  // m.prMes  — pseudorange in metres
  // m.cpMes  — carrier phase in cycles
  // m.doMes  — Doppler in Hz
  // m.cno    — carrier-to-noise density in dBHz
  // m.trkStat — tracking status flags
}
```

---

### Subframe Data

*NEO-8M v2 only.*

```cpp
void requestSubframeData();
bool isSubframeDataReady();
const RXM_SFRBX_Response& getSubframeData() const;
```

Returns raw navigation message subframes (ephemeris, almanac, clock corrections). Each `dwrd[]` entry is a 32-bit navigation data word.

---

### Hardware Status

```cpp
void requestHardwareStatus();
bool isHardwareStatusReady();
const MON_HW_Response& getHardwareStatus() const;

bool    isAntennaOK();       // true when aStatus == 2 (OK)
uint8_t getJammingLevel();   // 0 = no jamming, 255 = severe jamming
```

`aStatus` values: `0` = INIT, `1` = UNKNOWN, `2` = OK, `3` = SHORT, `4` = OPEN.

The jamming indicator (`jamInd`) runs from 0 to 255. Values above approximately 150 indicate interference worth investigating.

---

### Geofence

```cpp
void setGeofence(double centerLat, double centerLon, float radiusMeters);
void disableGeofence();
bool isInGeofence();
bool geofenceStateChanged();   // true once per boundary crossing
```

A software geofence computed using the Haversine formula. `geofenceStateChanged()` latches and resets on each call, so it returns `true` only on the call immediately after a boundary crossing event.

```cpp
gps.setGeofence(51.5074, -0.1278, 1000.0);  // 1 km radius around London

if (gps.geofenceStateChanged()) {
  if (gps.isInGeofence()) Serial.println("Entered zone");
  else                    Serial.println("Left zone");
}
```

---

### UTM Coordinates

```cpp
UTMCoordinate convertToUTM(double lat, double lon);
UTMCoordinate getCurrentUTM();
```

Converts WGS-84 latitude/longitude to UTM easting/northing using the Transverse Mercator projection (WGS-84 ellipsoid, k₀ = 0.9996).

```cpp
UTMCoordinate utm = gps.getCurrentUTM();
Serial.print(utm.zone);     // e.g. 34
Serial.println(utm.band);   // e.g. 'T'
Serial.println(utm.easting, 1);
Serial.println(utm.northing, 1);
```

---

### Distance Calculation

```cpp
double calculateDistance(double lat1, double lon1,
                         double lat2, double lon2);
```

Returns the great-circle distance between two WGS-84 coordinates in metres using the Haversine formula. Accurate to within ~0.5 % for distances under 1 000 km.

```cpp
double dist = gps.calculateDistance(
  gps.getLatitude(), gps.getLongitude(),
  51.5074, -0.1278   // destination: London
);
Serial.print(dist / 1000.0, 1);
Serial.println(" km");
```

---

### GNSS Constellation Configuration

*Multi-GNSS requires NEO-8M v2. On NEO-6M/7M the hardware is GPS-only regardless of this setting.*

```cpp
bool setGNSS(bool gps, bool galileo, bool glonass, bool beidou);
```

Common combinations use pre-built UBX payloads from `Better-GPS-commands.h` for reliability. Any other combination (e.g. GPS + GLONASS only) is built dynamically and checksummed at runtime.

```cpp
gps.setGNSS(true, true, false, false);   // GPS + Galileo
gps.setGNSS(true, false, true, false);   // GPS + GLONASS
gps.setGNSS(true, true, true, true);     // All constellations
```

> Changing constellations takes full effect after a warm or cold reset. The library does not automatically restart the receiver after this call.

---

### Dynamic Platform Model

```cpp
bool setDynamicModel(DynamicModel model);
```

Optimises the navigation filter for the expected motion profile. Returns `false` for models not yet implemented (`Sea`, `Airborne1g`, `Airborne2g`, `Airborne4g`).

| Value | Use case |
|-------|----------|
| `DynamicModel::Portable` | Default — no movement restrictions |
| `DynamicModel::Stationary` | Fixed installations |
| `DynamicModel::Pedestrian` | Walking speeds |
| `DynamicModel::Automotive` | Vehicle tracking |

---

### Update Rate

```cpp
void setUpdateRate(UpdateRate rate);
```

Sets both the GPS measurement rate and the internal interval used by `update()` for the auto-position-request. Available rates: `UpdateRate::HZ_1`, `HZ_5`, `HZ_10`.

> At 10 Hz a stable serial connection and sufficient processing time in `loop()` are required. Disabling unused NMEA sentences helps if data is being lost at high rates.

---

### Power Mode

```cpp
void setPowerMode(PowerMode mode);
```

`PowerMode::Continuous` — full performance, higher current draw.
`PowerMode::PowerSave` — reduced current draw, slower re-acquisition. Suitable for battery applications with infrequent position needs.

---

### Position Smoothing

```cpp
void setSmoothing(bool enable);
```

Enables or disables carrier smoothing of pseudoranges (CFG-NAVX5). Smoothing improves position stability for stationary applications but introduces latency for dynamic tracking.

---

### Configuration Save

```cpp
void saveConfig(SaveLocation target = SaveLocation::Both);
```

Persists the current receiver configuration to non-volatile memory. Called automatically at the end of `begin()` using the `DEFAULT_SAVE_LOCATION` setting.

| Value | Description |
|-------|-------------|
| `SaveLocation::Flash` | Save to internal flash |
| `SaveLocation::BBR` | Save to battery-backed RAM (lost if backup battery is removed) |
| `SaveLocation::Both` | Save to both (recommended) |

---

### Reset

```cpp
void resetHot();    // Preserves ephemeris, almanac, position — fastest re-acquisition
void resetWarm();   // Clears ephemeris, keeps almanac and position
void resetCold();   // Clears all stored data — full cold start
```

> No ACK is returned for reset commands. The receiver restarts immediately. The library inserts a blocking delay (1 / 2 / 3 seconds respectively) before returning to allow the receiver to reboot.

---

### Custom UBX Commands

```cpp
void sendCustomUBX(const uint8_t *cmd, uint16_t len);
```

Sends any raw UBX frame directly to the receiver. Useful for commands not covered by the library. You are responsible for correct checksums. See `Better-GPS-commands.h` for examples of how frames are structured.

---

### Validation

```cpp
bool testCommunication();
bool validateConfiguration();
```

`testCommunication()` sends a NAV-STATUS poll and returns `true` if a valid response is received — confirming that serial communication is working.

`validateConfiguration()` returns `true` only if all three checks pass: serial communication is working, antenna status is `OK` (`aStatus == 2`), and at least one satellite is visible. Use this as a startup health check before entering normal operation.

---

## Response Structures

All structures are defined in `Better-GPS-structs.h`. Getters return `const` references — no data is copied.

| Getter | Structure | Key fields |
|--------|-----------|------------|
| `getPOSLLH()` | `NAV_POSLLH_Response` | `lat`, `lon`, `hMSL`, `height`, `hAcc`, `vAcc` |
| `getStatus()` | `NAV_STATUS_Response` | `gpsFix`, `flags`, `fixStat`, `ttff`, `msss` |
| `getDOP()` | `NAV_DOP_Response` | `gDOP`, `pDOP`, `tDOP`, `vDOP`, `hDOP`, `nDOP`, `eDOP` |
| `getPVT()` | `NAV_PVT_Response` | `lat`, `lon`, `hMSL`, `gSpeed`, `headMot`, `year`…`second`, `numSV`, `pDOP` |
| `getOdometer()` | `NAV_ODO_Response` | `distance`, `totalDistance`, `distanceStd` |
| `getVelocity()` | `NAV_VELNED_Response` | `velN`, `velE`, `velD`, `speed`, `gSpeed`, `heading`, `sAcc`, `cAcc` |
| `getTimeUTC()` | `NAV_TIMEUTC_Response` | `year`, `month`, `day`, `hour`, `min`, `sec`, `tAcc`, `nano`, `valid` |
| `getSatellites()` | `NAV_SAT_Response` | `numSvs`, `satellites[]` |
| `getRawMeasurements()` | `RXM_RAWX_Response` | `rcvTow`, `week`, `leapS`, `numMeas`, `measurements[]` |
| `getSubframeData()` | `RXM_SFRBX_Response` | `gnssId`, `svId`, `numWords`, `chn`, `version`, `dwrd[]` |
| `getHardwareStatus()` | `MON_HW_Response` | `aStatus`, `aPower`, `jamInd`, `noisePerMS`, `agcCnt`, `flags` |

### SatelliteInfo fields

| Field | Type | Description |
|-------|------|-------------|
| `gnssId` | `uint8_t` | 0 = GPS, 1 = SBAS, 2 = Galileo, 3 = BeiDou, 5 = QZSS, 6 = GLONASS |
| `svId` | `uint8_t` | Satellite PRN / slot number |
| `cno` | `uint8_t` | Carrier-to-noise density in dBHz (signal strength) |
| `elev` | `int8_t` | Elevation angle −90° to +90° |
| `azim` | `int16_t` | Azimuth 0° to 360° |
| `prRes` | `int16_t` | Pseudorange residual in 0.1 m |
| `flags` | `uint32_t` | Status flags (bit 3 = used in fix) |
| `used` | `bool` | `true` if used in the current navigation solution |

### RawMeasurement fields

| Field | Type | Description |
|-------|------|-------------|
| `prMes` | `double` | Pseudorange measurement in metres |
| `cpMes` | `double` | Carrier phase measurement in cycles |
| `doMes` | `float` | Doppler measurement in Hz |
| `gnssId` | `uint8_t` | GNSS identifier |
| `svId` | `uint8_t` | Satellite identifier |
| `freqId` | `uint8_t` | GLONASS frequency slot (255 if not GLONASS) |
| `locktime` | `uint16_t` | Carrier phase lock time counter in ms |
| `cno` | `uint8_t` | Carrier-to-noise density in dBHz |
| `prStdev` | `uint8_t` | Pseudorange standard deviation |
| `cpStdev` | `uint8_t` | Carrier phase standard deviation |
| `doStdev` | `uint8_t` | Doppler standard deviation |
| `trkStat` | `uint8_t` | Tracking status flags |

---

## Enums

All enums are defined in `Better-GPS-config.h` as scoped `enum class`.

```cpp
enum class UpdateRate   : uint8_t { HZ_1, HZ_5, HZ_10 }

enum class SaveLocation : uint8_t { Flash, BBR, Both }

enum class PowerMode    : uint8_t { Continuous, PowerSave }

enum class FixType      : uint8_t {
  NoFix, DeadReckoning, Fix2D, Fix3D, GPSDeadReckoning, TimeOnly
}

enum class DynamicModel : uint8_t {
  Portable, Stationary, Pedestrian, Automotive,
  Sea, Airborne1g, Airborne2g, Airborne4g
}
```

> `Sea`, `Airborne1g`, `Airborne2g`, and `Airborne4g` are defined in the enum but `setDynamicModel()` returns `false` for these values as pre-built UBX payloads are not yet included.

---

## Feature Availability by Module

| Feature | NEO-6M v2 | NEO-7M v2 | NEO-8M v2 |
|---------|:---------:|:---------:|:---------:|
| NAV-POSLLH — position | ✓ | ✓ | ✓ |
| NAV-STATUS — fix status | ✓ | ✓ | ✓ |
| NAV-VELNED — velocity NED | ✓ | ✓ | ✓ |
| NAV-TIMEUTC — UTC time | ✓ | ✓ | ✓ |
| NAV-DOP — dilution of precision | ✓ | ✓ | ✓ |
| MON-VER — firmware version | ✓ | ✓ | ✓ |
| MON-HW — hardware / antenna / jamming | ✓ | ✓ | ✓ |
| CFG-RATE — update rate | ✓ | ✓ | ✓ |
| CFG-NAV5 — dynamic platform model | ✓ | ✓ | ✓ |
| CFG-NAVX5 — position smoothing | ✓ | ✓ | ✓ |
| CFG-RXM — power mode | ✓ | ✓ | ✓ |
| CFG-CFG — save configuration | ✓ | ✓ | ✓ |
| CFG-RST — reset | ✓ | ✓ | ✓ |
| NAV-PVT — position + velocity + time | ✗ | ✓ | ✓ |
| NAV-SAT — detailed satellite info | ✗ | ✗ | ✓ |
| NAV-ODO — odometer | ✗ | ✗ | ✓ |
| RXM-RAWX — raw measurements | ✗ | ✗ | ✓ |
| RXM-SFRBX — subframe data | ✗ | ✗ | ✓ |
| CFG-GNSS — multi-constellation | ✗ | ✗ | ✓ |

> The NEO-6M supports NAV-SVINFO (0x30) for basic satellite counts, but this older message is not implemented in the current version of the library. NAV-SAT (0x35) is NEO-8M only.

---

## Demo Sketch

The file `BetterGPS-demo.ino` demonstrates every feature of the library in a single sketch. To use it:

**1. Select your module** — uncomment exactly one of the three lines at the top:

```cpp
// #define NEO6MV2
// #define NEO7MV2
#define NEO8MV2
```

Features unavailable on the selected module are automatically excluded at compile time via `#ifdef HAS_xxx` guards. No manual changes are needed elsewhere.

**2. Enable or disable demo sections** — comment out any `#define DEMO_xxx` line you do not want to run:

```cpp
#define DEMO_POSITION
#define DEMO_STATUS
// #define DEMO_ODOMETER   // comment out if not needed
```

**3. Select GNSS constellations** (NEO-8M only — ignored on other modules):

```cpp
#define DEMO_GPS_ONLY
// #define DEMO_GPS_GALILEO
// #define DEMO_GPS_GLONASS
// #define DEMO_GPS_BEIDOU
// #define DEMO_ALL_GNSS
```

**4. Set your hardware pins and timing:**

```cpp
#define GPS_RX_PIN             16
#define GPS_TX_PIN             17
#define DEMO_LOOP_INTERVAL_MS  3000   // how often the demo cycle repeats
#define DEMO_WAIT_FOR_FIX_MS   15000  // how long setup() waits for a fix
```

**5.** Upload and open the Serial Monitor at **115200 baud**. Each demo cycle prints a clearly labelled section for every enabled feature. If no fix is available, sections that require one print a notice rather than crashing.

> Save, reset, and restart commands (`DEMO_SAVE_CONFIG`, `DEMO_RESET_HOT`, etc.) are commented out by default. Uncomment them only when explicitly needed, as they modify persistent receiver state or restart the module.
