# GPS Knowledge Base

A plain-English reference for GPS terminology, UBX protocol concepts, and everything else you will encounter when working with u-blox GPS modules. No prior knowledge assumed.

---

## Table of Contents

- [How GPS Works — The Basics](#how-gps-works--the-basics)
- [Fix Types](#fix-types)
- [GNSS vs GPS](#gnss-vs-gps)
- [Satellite Constellations](#satellite-constellations)
- [UBX Protocol](#ubx-protocol)
  - [What is UBX?](#what-is-ubx)
  - [NMEA vs UBX](#nmea-vs-ubx)
  - [UBX Frame Structure](#ubx-frame-structure)
  - [Checksum](#checksum)
  - [ACK and NAK](#ack-and-nak)
  - [Poll vs Periodic](#poll-vs-periodic)
  - [Message Classes](#message-classes)
- [NAV Messages](#nav-messages)
- [RXM Messages](#rxm-messages)
- [CFG Messages](#cfg-messages)
- [MON Messages](#mon-messages)
- [Position Concepts](#position-concepts)
  - [Latitude and Longitude](#latitude-and-longitude)
  - [Altitude — hMSL vs height](#altitude--hmsl-vs-height)
  - [Accuracy Estimates — hAcc and vAcc](#accuracy-estimates--hacc-and-vacc)
  - [WGS-84](#wgs-84)
  - [UTM Coordinates](#utm-coordinates)
- [Time Concepts](#time-concepts)
  - [GPS Time vs UTC](#gps-time-vs-utc)
  - [iTOW](#itow)
  - [Leap Seconds](#leap-seconds)
  - [TTFF — Time To First Fix](#ttff--time-to-first-fix)
  - [Timezone and DST](#timezone-and-dst)
- [Velocity Concepts](#velocity-concepts)
  - [NED Frame](#ned-frame)
  - [Ground Speed vs 3D Speed](#ground-speed-vs-3d-speed)
  - [Heading of Motion vs Heading of Vehicle](#heading-of-motion-vs-heading-of-vehicle)
- [Signal Quality](#signal-quality)
  - [CNO — Carrier-to-Noise Density](#cno--carrier-to-noise-density)
  - [DOP — Dilution of Precision](#dop--dilution-of-precision)
  - [Jamming](#jamming)
  - [AGC — Automatic Gain Control](#agc--automatic-gain-control)
- [Satellite Concepts](#satellite-concepts)
  - [PRN — Pseudo-Random Noise Number](#prn--pseudo-random-noise-number)
  - [Elevation and Azimuth](#elevation-and-azimuth)
  - [Pseudorange](#pseudorange)
  - [Carrier Phase](#carrier-phase)
  - [Doppler](#doppler)
  - [Ephemeris and Almanac](#ephemeris-and-almanac)
  - [Subframes](#subframes)
- [Receiver Concepts](#receiver-concepts)
  - [Dynamic Platform Model](#dynamic-platform-model)
  - [Position Smoothing](#position-smoothing)
  - [Dead Reckoning](#dead-reckoning)
  - [Differential GPS — DGPS and SBAS](#differential-gps--dgps-and-sbas)
  - [Raw Measurements — RAWX](#raw-measurements--rawx)
  - [Odometer](#odometer)
- [Antenna](#antenna)
  - [Active vs Passive Antenna](#active-vs-passive-antenna)
  - [Antenna Status Codes](#antenna-status-codes)
- [Baud Rate](#baud-rate)
- [Hot Start, Warm Start, Cold Start](#hot-start-warm-start-cold-start)
- [BBR and Flash Storage](#bbr-and-flash-storage)
- [Power Modes](#power-modes)
- [Quick Reference Glossary](#quick-reference-glossary)

---

## How GPS Works — The Basics

GPS satellites orbit the Earth at about 20 200 km altitude. Each satellite continuously broadcasts a radio signal that contains two things: the satellite's exact position in space at that moment, and a very precise timestamp.

Your GPS receiver picks up signals from multiple satellites simultaneously. Because radio waves travel at the speed of light (approximately 299 792 km/s), the receiver can calculate how far away each satellite is by measuring how long the signal took to arrive. With distances to at least three satellites it can calculate its 2D position (latitude and longitude). With a fourth satellite it can also solve for altitude and correct its own clock error.

This technique is called **trilateration**.

The more satellites the receiver can see and use, the more accurate and reliable the position solution becomes.

---

## Fix Types

A **fix** means the receiver has calculated a valid position. Different fix types indicate how reliable and complete that position is.

| Fix type | Meaning |
|----------|---------|
| **No Fix** | Not enough satellites visible or tracked to compute a position |
| **Dead Reckoning only** | Position estimated from last known position, speed, and heading — no satellite data used at this moment |
| **2D Fix** | Position computed from 3 satellites — latitude and longitude known but altitude is assumed, not measured |
| **3D Fix** | Position computed from 4 or more satellites — latitude, longitude, and altitude all measured |
| **GPS + Dead Reckoning** | Satellite fix combined with inertial sensor data for better continuity (e.g. in tunnels) |
| **Time Only** | The receiver knows the time accurately but cannot compute a position |

A **3D Fix** is what you want for most applications. A 2D fix is acceptable for navigation on flat ground but the assumed altitude may be wrong by tens of metres.

---

## GNSS vs GPS

**GPS** (Global Positioning System) is the American satellite navigation system, operated by the US Air Force. It is also used as a generic word for any satellite positioning system, which causes confusion.

**GNSS** (Global Navigation Satellite System) is the correct umbrella term for all satellite navigation systems combined. When a receiver says it supports GNSS, it means it can use signals from multiple constellations at once, not just American GPS satellites.

---

## Satellite Constellations

| Name | Country | Satellites | Notes |
|------|---------|------------|-------|
| **GPS** | USA | 31 active | The original, most widely supported |
| **GLONASS** | Russia | 24 active | Better coverage at high latitudes |
| **Galileo** | European Union | 30 active | Higher accuracy signals, excellent urban coverage |
| **BeiDou** (BDS) | China | 35+ active | Excellent Asia-Pacific coverage, growing globally |
| **QZSS** | Japan | 4 active | Regional augmentation for Japan, not a standalone system |
| **SBAS** | Various | Multiple | Not navigation satellites — these broadcast correction data |

Using more constellations means more satellites visible at any given moment, which directly improves accuracy, reduces time to fix, and helps in difficult environments like urban canyons and dense forests.

---

## UBX Protocol

### What is UBX?

UBX is u-blox's own binary communication protocol for configuring and reading data from their GPS receivers. It is an alternative to the human-readable NMEA sentences that GPS modules also output.

UBX is more efficient (fewer bytes per message), supports many more features than NMEA, and is less prone to parsing errors because message boundaries and lengths are explicit. For serious applications, UBX is almost always preferable.

### NMEA vs UBX

| | NMEA | UBX |
|-|------|-----|
| Format | ASCII text | Binary bytes |
| Readability | Human-readable | Requires a parser |
| Efficiency | Low (verbose) | High (compact) |
| Features | Basic position and time | Full receiver configuration and diagnostics |
| Error detection | Simple checksum | Fletcher-8 checksum |
| Example | `$GPGGA,123519,...*47` | `0xB5 0x62 0x01 0x02 ...` |

Both protocols can be active at the same time on the same serial port. BetterGPS works in UBX-only mode by default and optionally silences the NMEA sentences to avoid wasted bandwidth.

### UBX Frame Structure

Every UBX message follows this exact structure:

```
┌──────────┬──────────┬───────────┬────────────┬──────────────────┬──────────────────┐
│ Sync 1   │ Sync 2   │ Class     │ ID         │ Length (2 bytes) │ Payload (n bytes)│
│ 0xB5     │ 0x62     │ 1 byte    │ 1 byte     │ LSB first        │                  │
└──────────┴──────────┴───────────┴────────────┴──────────────────┴──────────────────┘
followed by:
┌──────────┬──────────┐
│ CK_A     │ CK_B     │
│ checksum │ checksum │
└──────────┴──────────┘
```

- **Sync chars** `0xB5 0x62` — always these two bytes, mark the start of every UBX frame.
- **Class** — identifies the category of the message (e.g. `0x01` = Navigation, `0x06` = Configuration).
- **ID** — identifies the specific message within the class (e.g. class `0x01` ID `0x02` = NAV-POSLLH).
- **Length** — number of bytes in the payload, stored as a 16-bit integer with the least significant byte first (little-endian).
- **Payload** — the actual data. Structure depends on the message type.
- **CK_A / CK_B** — two checksum bytes for error detection (see below).

### Checksum

UBX uses a **Fletcher-8** checksum. It is computed over the Class, ID, Length, and Payload bytes — not the sync chars, and not the checksum bytes themselves.

The algorithm:

```
CK_A = 0
CK_B = 0
for each byte B in (Class, ID, Length, Payload):
    CK_A = (CK_A + B) & 0xFF
    CK_B = (CK_B + CK_A) & 0xFF
```

If the received CK_A and CK_B do not match the computed values, the message is silently discarded. This catches single-bit errors and most multi-bit errors.

### ACK and NAK

When you send a **configuration command** to a u-blox receiver (anything in class `0x06` CFG), the receiver responds with one of two acknowledgement messages:

| Response | Class | ID | Meaning |
|----------|-------|----|---------|
| **ACK-ACK** | `0x05` | `0x01` | Command accepted and applied |
| **ACK-NAK** | `0x05` | `0x00` | Command rejected — invalid parameters, wrong firmware version, or unsupported feature |

**No ACK at all** means one of: the serial connection is broken, the baud rate is wrong, the message was corrupted in transit, or the command was a reset (reset commands do not send an ACK because the receiver reboots immediately).

When you send a **poll request** (asking for current data) instead of a configuration command, the receiver responds with the requested data message directly — not an ACK.

### Poll vs Periodic

There are two ways to receive data from a u-blox receiver:

**Poll** — you send a request and the receiver responds once with the current data. This is what BetterGPS does by default. You call `requestPosition()`, the receiver sends one NAV-POSLLH response, and the library parses it.

**Periodic** — you configure the receiver (using CFG-MSG) to send a particular message automatically at every measurement cycle. The receiver then streams that message at the configured rate without any further prompting. This is more efficient for high-rate logging but requires more careful buffer management.

BetterGPS uses polling. The `update()` method handles the polling rate automatically.

### Message Classes

| Class byte | Name | Purpose |
|------------|------|---------|
| `0x01` | NAV | Navigation results (position, velocity, time, satellites) |
| `0x02` | RXM | Receiver Manager (raw measurements, subframes) |
| `0x04` | INF | Informational messages (errors, warnings, debug) |
| `0x05` | ACK | Acknowledgements (ACK-ACK and ACK-NAK) |
| `0x06` | CFG | Configuration commands |
| `0x09` | UPD | Firmware update and memory management |
| `0x0A` | MON | Monitoring and hardware status |
| `0x0D` | TIM | Timing and timepulse |
| `0x13` | MGA | Multi-GNSS assistance data |
| `0x21` | LOG | Data logging |
| `0x27` | SEC | Security and unique ID |

---

## NAV Messages

NAV messages carry the results of the navigation solution — what the receiver has computed from the satellite signals.

| Message | ID | Available on | Description |
|---------|----|-------------|-------------|
| **NAV-POSLLH** | `0x02` | All | Position as latitude, longitude, height. The simplest position message. |
| **NAV-STATUS** | `0x03` | All | Fix type, fix status flags, time to first fix, uptime |
| **NAV-DOP** | `0x04` | All | Dilution of precision values (see DOP section) |
| **NAV-PVT** | `0x07` | 7M+ | Position, velocity, and time combined in one message — the most useful single message |
| **NAV-ODO** | `0x09` | 8M+ | Odometer — accumulated distance since last reset |
| **NAV-VELNED** | `0x12` | All | Velocity in the NED (North-East-Down) frame |
| **NAV-TIMEUTC** | `0x21` | All | Current UTC date and time with accuracy estimate |
| **NAV-TIMEGPS** | `0x20` | All | GPS time (week number + time of week) |
| **NAV-SVINFO** | `0x30` | 6M only | Satellite information (older format, replaced by NAV-SAT) |
| **NAV-SAT** | `0x35` | 8M+ | Detailed satellite information including signal strength and usage |
| **NAV-DGPS** | `0x36` | All | Differential GPS correction data used in the solution |

---

## RXM Messages

RXM (Receiver Manager) messages provide low-level access to raw signal measurements. These are used for advanced applications like RTK (Real-Time Kinematics) or precise positioning research.

| Message | ID | Available on | Description |
|---------|----|-------------|-------------|
| **RXM-RAW** | `0x10` | Older only | Raw measurements (older format) |
| **RXM-RAWX** | `0x15` | 8M+ | Extended raw measurements — pseudorange, carrier phase, Doppler for every tracked signal |
| **RXM-SFRBX** | `0x13` | 8M+ | Raw navigation subframe data broadcast by each satellite |

---

## CFG Messages

CFG messages are configuration commands. Sending one changes how the receiver behaves. They require an ACK-ACK response to confirm success.

| Message | ID | Description |
|---------|----|-------------|
| **CFG-PRT** | `0x00` | Configure port settings (UART baud rate, protocol selection) |
| **CFG-MSG** | `0x01` | Enable or disable periodic output of specific messages |
| **CFG-INF** | `0x02` | Configure informational message output |
| **CFG-RATE** | `0x08` | Set measurement and navigation rate (e.g. 1 Hz, 5 Hz, 10 Hz) |
| **CFG-CFG** | `0x09` | Save, load, or clear configuration from non-volatile memory |
| **CFG-ANT** | `0x13` | Antenna settings (pin assignments, short/open detection) |
| **CFG-NAV5** | `0x24` | Navigation engine settings — dynamic platform model, fix mode |
| **CFG-NAVX5** | `0x23` | Advanced navigation engine settings — carrier smoothing etc. |
| **CFG-RXM** | `0x11` | Power mode (continuous or power save) |
| **CFG-RST** | `0x04` | Reset the receiver (hot / warm / cold start) |
| **CFG-GNSS** | `0x3E` | Enable or disable GNSS constellations (NEO-8M only) |

---

## MON Messages

MON (Monitor) messages report on the health and internal state of the receiver hardware.

| Message | ID | Description |
|---------|----|-------------|
| **MON-VER** | `0x04` | Software version, hardware version, firmware version strings |
| **MON-HW** | `0x09` | Hardware status: antenna state, jamming indicator, noise level, AGC |
| **MON-RXBUF** | `0x07` | Receiver buffer usage statistics |
| **MON-STATUS** | `0x08` | Status flags and startup information |

---

## Position Concepts

### Latitude and Longitude

**Latitude** measures how far north or south you are from the equator. The equator is 0°, the North Pole is +90° (or 90°N), and the South Pole is −90° (or 90°S).

**Longitude** measures how far east or west you are from the Prime Meridian (which runs through Greenwich, London). The Prime Meridian is 0°, going east reaches +180°, going west reaches −180°.

Together, a latitude/longitude pair uniquely identifies any point on Earth's surface.

In UBX messages, both values are stored as 32-bit signed integers scaled by 10⁻⁷ (i.e. multiplied by 10 000 000). This avoids floating-point numbers in the protocol while preserving about 1 cm of resolution.

```
Raw value  47 497 900  →  47.4979°  (Budapest, latitude)
Raw value  19 040 200  →  19.0402°  (Budapest, longitude)
```

In code: `double lat = rawLat / 1e7;`

### Altitude — hMSL vs height

GPS receivers report two different altitude values:

**height** (`height` field) — height above the WGS-84 ellipsoid. The ellipsoid is a mathematical model of Earth's shape. It is not the actual surface of the ocean.

**hMSL** (`hMSL` field) — height above Mean Sea Level. This uses a geoid model (a model of where sea level would be across the entire Earth, accounting for gravity variations). This is the altitude you see on maps and in aviation.

For most practical purposes, use **hMSL**. The difference between the two can be anywhere from −100 m to +80 m depending on where on Earth you are.

Both values are stored in millimetres in UBX messages. Convert to metres by dividing by 1000.

### Accuracy Estimates — hAcc and vAcc

**hAcc** (horizontal accuracy estimate) and **vAcc** (vertical accuracy estimate) are the receiver's own estimate of how accurate the current position is, expressed in millimetres. These are statistical estimates (approximately 1-sigma), not guarantees.

A typical value under open sky with a good signal is 2 000–5 000 mm (2–5 m horizontal). Under heavy tree cover or in a city with tall buildings the receiver may report 10 000–50 000 mm or more.

Vertical accuracy is almost always worse than horizontal — typically 1.5–2× worse.

### WGS-84

**WGS-84** (World Geodetic System 1984) is the reference coordinate system used by GPS. It defines the shape of the Earth as a specific ellipsoid (a slightly flattened sphere), and all GPS coordinates are measured relative to this model.

When you use GPS coordinates in a map application, that application also uses WGS-84, which is why the coordinates align correctly.

### UTM Coordinates

**UTM** (Universal Transverse Mercator) is an alternative coordinate system that divides the Earth into 60 numbered zones and expresses positions within each zone as **easting** and **northing** values in metres. This is useful because the values are in metres — you can directly calculate distances by subtracting two easting or northing values, which you cannot do directly with latitude/longitude degrees.

A full UTM coordinate looks like: `34T  E 447600  N 5260800`

Where `34T` is the zone (number 1–60 + letter A–X for the latitude band), easting is the distance in metres east from the zone's central meridian (plus a 500 000 m false origin), and northing is the distance in metres north from the equator.

---

## Time Concepts

### GPS Time vs UTC

**GPS time** is the internal time system used by the GPS constellation. It started on 6 January 1980 and has been running continuously since, without any leap second adjustments. GPS time is expressed as a **week number** (weeks since 6 January 1980) and a **time of week** in milliseconds or seconds.

**UTC** (Coordinated Universal Time) is the civil time standard used worldwide. Unlike GPS time, UTC periodically inserts leap seconds to stay aligned with Earth's rotation.

As of 2024, GPS time is 18 seconds ahead of UTC. The receiver knows the current offset (leap seconds) and can convert between the two.

### iTOW

**iTOW** (integer Time Of Week) appears in almost every NAV message. It is the number of milliseconds elapsed since the start of the current GPS week (Sunday midnight). The value resets to zero every Sunday at midnight GPS time.

iTOW is used to timestamp each measurement. If two messages have the same iTOW, they were produced from the same navigation solution epoch.

Example: `iTOW = 345 600 000` means 96 hours have elapsed since Sunday midnight — it is currently Thursday midnight.

### Leap Seconds

A **leap second** is an occasional one-second adjustment added to UTC to keep it within 0.9 seconds of astronomical time (UT1), which varies slightly due to irregular changes in Earth's rotation speed.

GPS time does not include leap seconds. The **leapS** field in RXM-RAWX tells you the current difference between GPS time and UTC. You must subtract this value from GPS time to get UTC.

As of 2024 the value is 18, meaning `UTC = GPS_time − 18 seconds`.

### TTFF — Time To First Fix

**TTFF** (Time To First Fix) is how long it takes the receiver to compute its first valid position after being powered on. It depends heavily on how much data the receiver already has stored:

| Start type | Typical TTFF | What is pre-loaded |
|------------|-------------|-------------------|
| Hot start | < 1 second | Everything — almanac, ephemeris, last position, time |
| Warm start | 10–45 seconds | Almanac and last position, but ephemeris must be redownloaded |
| Cold start | 30–120 seconds | Nothing — receiver must download everything from satellites |

NAV-STATUS reports the actual TTFF in milliseconds once a fix is obtained.

### Timezone and DST

GPS satellites broadcast UTC time, which has no timezone offset. To display local time, you must add your UTC offset.

**Timezone offset** examples: Central European Time is UTC+1, US Eastern Standard Time is UTC−5, India is UTC+5:30.

**DST** (Daylight Saving Time) is an additional +1 hour adjustment applied during summer months in many countries. In Europe, DST begins on the last Sunday of March at 01:00 UTC and ends on the last Sunday of October at 01:00 UTC.

BetterGPS handles both automatically if you set `TIMEZONE_OFFSET` and `AUTO_DST = true` in the configuration.

---

## Velocity Concepts

### NED Frame

**NED** stands for North, East, Down. It is a local coordinate frame aligned to the Earth's surface at your current location:

- **North** — positive direction points toward geographic north
- **East** — positive direction points toward geographic east
- **Down** — positive direction points toward the centre of the Earth (gravity direction)

This frame is intuitive for navigation because north and east correspond to map directions. Down is positive so that a stationary object on the surface has zero vertical velocity (not falling).

NAV-VELNED reports velocity as three components: `velN` (northward speed in cm/s), `velE` (eastward speed in cm/s), `velD` (downward speed in cm/s).

### Ground Speed vs 3D Speed

**Ground speed** (`gSpeed`) is your speed measured in the horizontal plane only — as if viewed from above on a map. It does not include any vertical motion.

**3D speed** (`speed`) is your total speed including vertical motion. For a car or person, this is nearly identical to ground speed. For an aircraft climbing steeply, 3D speed will be significantly higher.

Both are in cm/s in NAV-VELNED.

### Heading of Motion vs Heading of Vehicle

**Heading of motion** (`headMot`) is the direction you are actually moving, expressed as degrees clockwise from true north (0° = north, 90° = east, 180° = south, 270° = west). This is computed from the velocity vector.

**Heading of vehicle** (`headVeh`) is the direction the vehicle is pointing, which may differ from the direction of motion. For example, a car sliding sideways will have a different heading of motion than heading of vehicle. This field is only populated when a compatible motion sensor is connected.

Both values are stored in UBX as 32-bit integers scaled by 10⁻⁵ degrees. A raw value of `9 000 000` equals `90.00000°`.

---

## Signal Quality

### CNO — Carrier-to-Noise Density

**CNO** (Carrier-to-Noise Density, sometimes written C/N₀) measures the strength of a satellite signal relative to the background noise. It is expressed in **dBHz**.

| CNO value | Signal quality |
|-----------|---------------|
| < 20 dBHz | Very weak — barely tracked, likely unusable |
| 20–30 dBHz | Weak — receiver may track but accuracy is poor |
| 30–40 dBHz | Moderate — usable, typical in light tree cover |
| 40–45 dBHz | Good — typical in open sky |
| > 45 dBHz | Excellent — clear sky, good antenna |

A receiver needs to see several satellites above about 25–30 dBHz to compute a reliable fix. You want an average CNO across used satellites of at least 35 dBHz for good results.

### DOP — Dilution of Precision

**DOP** (Dilution of Precision) is a dimensionless number that describes how the geometry of the visible satellites amplifies positioning errors. A receiver can have a perfect signal but a bad DOP if all its satellites happen to be clustered in one part of the sky.

The smaller the DOP, the better. Think of it as a multiplier on your ranging error: if your pseudorange measurements each have 1 m of error and your HDOP is 2.0, your horizontal position error will be about 2 m.

| DOP value | Rating | Meaning |
|-----------|--------|---------|
| 1 | Ideal | Best possible geometry |
| 1–2 | Excellent | High-precision applications |
| 2–5 | Good | Suitable for most navigation |
| 5–10 | Moderate | Usable but accuracy is degraded |
| 10–20 | Fair | Marginal, large position jumps possible |
| > 20 | Poor | Do not use for navigation |

The different DOP variants:

| Name | Stands for | Measures |
|------|------------|---------|
| **HDOP** | Horizontal DOP | Horizontal position accuracy |
| **VDOP** | Vertical DOP | Vertical position accuracy |
| **PDOP** | Position DOP | Overall 3D position accuracy |
| **TDOP** | Time DOP | Timing accuracy |
| **GDOP** | Geometric DOP | All errors combined |
| **NDOP** | Northing DOP | North component of horizontal accuracy |
| **EDOP** | Easting DOP | East component of horizontal accuracy |

In UBX, all DOP values are stored as integers scaled by 0.01. Raw value `142` = DOP of `1.42`.

### Jamming

**GPS jamming** is deliberate radio frequency interference at the GPS signal frequency (L1 = 1575.42 MHz) that prevents receivers from tracking satellites. Jammers are illegal in most countries but increasingly common near military zones, prisons, and some vehicles.

The `jamInd` field in MON-HW reports a jamming indicator from 0 to 255:

| Range | Interpretation |
|-------|---------------|
| 0–50 | No significant jamming detected |
| 50–150 | Weak interference — monitor the situation |
| 150–200 | Moderate jamming — accuracy may be degraded |
| 200–255 | Strong jamming — fix may be lost |

This is distinct from multipath and natural interference, which the receiver handles normally.

### AGC — Automatic Gain Control

**AGC** (Automatic Gain Control) is a circuit inside the receiver that adjusts the amplification of the incoming signal to keep it at an optimal level for the analogue-to-digital converter.

The `agcCnt` field in MON-HW reports how often the AGC has activated recently. Abnormally high AGC activity can indicate jamming or very strong out-of-band interference, because the circuit is working hard to compensate.

A noise level (`noisePerMS`) significantly above the baseline (typically above 120–130) also suggests interference.

---

## Satellite Concepts

### PRN — Pseudo-Random Noise Number

Each GPS satellite broadcasts a unique **PRN** (Pseudo-Random Noise) code — a specific sequence of bits that looks like random noise but is actually perfectly deterministic and known in advance. The receiver correlates the incoming signal against its own copy of the code to measure the signal arrival time.

PRN numbers also serve as satellite identifiers. GPS satellites are identified by their PRN (1–32). GLONASS uses slot numbers, Galileo and BeiDou use their own numbering schemes. In UBX, the `svId` field carries the satellite identifier — what that means depends on the `gnssId`.

### Elevation and Azimuth

**Elevation** is the angle of the satellite above the horizon:
- 0° = on the horizon
- 90° = directly overhead (zenith)
- Negative values mean the satellite is below the horizon and not visible

**Azimuth** is the horizontal direction to the satellite measured clockwise from true north:
- 0° / 360° = north
- 90° = east
- 180° = south
- 270° = west

Low-elevation satellites (below ~10–15°) have signals that travel through more atmosphere, resulting in more delay errors and generally weaker signals. Most receivers have a configurable **elevation mask** that ignores satellites below a threshold (typically 5–10°).

### Pseudorange

A **pseudorange** is an approximate measurement of the distance between the receiver and a satellite, calculated from the signal travel time multiplied by the speed of light.

It is called a *pseudo*range rather than a true range because it contains errors from several sources:
- Receiver clock error (the most significant)
- Ionospheric delay (the signal slows as it passes through the ionosphere)
- Tropospheric delay (moisture in the lower atmosphere causes small delays)
- Multipath (signals reflecting off buildings before reaching the antenna)

The navigation solution uses measurements from multiple satellites simultaneously to cancel out the receiver clock error and minimise the remaining errors.

### Carrier Phase

The GPS signal is transmitted on a carrier wave at a known frequency (L1 = 1575.42 MHz, L2 = 1227.60 MHz). The receiver can measure not just the arrival time of the code modulated onto this carrier, but also the **phase** of the carrier wave itself.

Carrier phase measurements are far more precise than pseudorange measurements (millimetre-level vs metre-level) but they contain an unknown integer number of complete cycles between the receiver and the satellite (the **integer ambiguity**). Resolving this ambiguity is what makes RTK (Real-Time Kinematics) positioning possible, achieving centimetre-level accuracy.

### Doppler

The **Doppler effect** causes the received frequency to shift when the satellite or receiver is moving. A satellite moving toward you transmits at a slightly higher observed frequency; one moving away transmits at a slightly lower frequency.

The receiver measures this frequency shift to compute **Doppler velocity** — a direct, independent measurement of how fast the range to each satellite is changing. This is used both to assist tracking and to compute velocity.

Doppler measurements in RXM-RAWX are in Hz. The sign convention is: positive Doppler means the satellite is approaching (range decreasing).

### Ephemeris and Almanac

**Ephemeris** data is precise orbital information for a specific satellite — its exact position and velocity in space over the next few hours. The receiver downloads the ephemeris for each satellite it tracks and uses it to compute where the satellite is at any given moment.

Ephemeris data is valid for about 2–4 hours and must be re-downloaded after a cold start or if the receiver has been off for too long. Downloading the ephemeris for a single satellite takes about 30 seconds.

**Almanac** data is less precise orbital information for all satellites in the constellation. The receiver downloads the almanac to know which satellites are expected to be visible from its approximate location and to acquire them quickly. The almanac is valid for weeks or months.

After a warm start (ephemeris cleared but almanac kept), the receiver knows where to look for satellites but must re-download the precise ephemeris data before it can achieve a precise fix.

### Subframes

Each satellite transmits its navigation message divided into pages called **subframes**, each taking 6 seconds to transmit. Five subframes make one complete **frame** (30 seconds). The full set of almanac data for all satellites in the constellation is spread across 25 complete frames — meaning a complete almanac download takes about 12.5 minutes from scratch.

RXM-SFRBX captures these raw subframes before the receiver decodes them, useful for research and advanced processing.

---

## Receiver Concepts

### Dynamic Platform Model

The receiver's navigation filter makes assumptions about how the device is moving. By selecting the correct **dynamic platform model**, you tell the filter what kinds of acceleration and speed are realistic, improving accuracy in your specific use case.

| Model | Max altitude | Max velocity | Use case |
|-------|-------------|-------------|----------|
| Portable | 12 000 m | 310 m/s | General use, default |
| Stationary | 9 000 m | 10 m/s | Fixed installations |
| Pedestrian | 9 000 m | 30 m/s | Walking, hiking |
| Automotive | 9 000 m | 84 m/s | Cars, trucks |
| Sea | 9 000 m | 25 m/s | Boats |
| Airborne < 1g | 50 000 m | 100 m/s | Aircraft with low manoeuvring |
| Airborne < 2g | 50 000 m | 250 m/s | Aircraft with moderate manoeuvring |
| Airborne < 4g | 50 000 m | 500 m/s | High-performance aircraft |

Choosing the wrong model can degrade accuracy. For example, if you use the Stationary model in a moving car, the filter will reject velocity measurements it considers too large and the position will appear to jump rather than track smoothly.

### Position Smoothing

**Position smoothing** (CFG-NAVX5, the `useAOP` and carrier smoothing flags) applies additional filtering to reduce noise in the position output. The receiver averages measurements over time to produce a smoother position trace.

This is beneficial for stationary or slow-moving applications where the goal is to minimise random noise in the reported coordinates.

For high-speed tracking (cars, aircraft), smoothing should be disabled because it introduces latency — the reported position lags behind the true position as the filter catches up.

### Dead Reckoning

**Dead reckoning** is the process of estimating the current position by starting from the last known position and applying known movement — speed, direction, and elapsed time — even without satellite signals.

A car driving into a tunnel loses GPS signal, but if the receiver knows it was travelling at 80 km/h heading north, it can estimate that 30 seconds later it is roughly 667 m further north.

Dead reckoning accuracy degrades over time due to accumulated errors in the speed and heading measurements. It is typically combined with external sensors (wheel speed, gyroscope) in automotive receivers.

### Differential GPS — DGPS and SBAS

**DGPS** (Differential GPS) improves accuracy by using a network of fixed reference stations at precisely known locations. Each station measures GPS errors at its location and broadcasts corrections. Nearby receivers apply these corrections to their own measurements, cancelling out common errors (mainly ionospheric delay) and improving accuracy from ~5 m down to ~1 m.

**SBAS** (Satellite-Based Augmentation System) is a form of DGPS that broadcasts corrections via geostationary satellites rather than ground radio transmitters, providing wider coverage. Regional SBAS systems:

| System | Coverage |
|--------|---------|
| WAAS | North America |
| EGNOS | Europe |
| MSAS | Japan |
| GAGAN | India |
| SDCM | Russia |

In GNSS ID lists, SBAS satellites appear as gnssId = 1. They are not navigation satellites but correction data relay satellites.

### Raw Measurements — RAWX

**Raw measurements** (RXM-RAWX, available on NEO-8M) give you direct access to the underlying signal data before the receiver has processed it into a position:

- **Pseudorange** — raw distance measurement in metres
- **Carrier phase** — fractional and integer carrier cycle count
- **Doppler** — frequency shift measurement in Hz
- Signal quality metrics (CNO, lock time, tracking status)

This is the foundation of **RTK** (Real-Time Kinematics) processing, **PPP** (Precise Point Positioning), and post-processing for survey-grade accuracy.

### Odometer

The receiver's internal **odometer** (NAV-ODO, available on NEO-8M) accumulates the distance travelled over time. It is more reliable than simply integrating GPS speed because the receiver handles signal dropouts, re-acquisitions, and speed smoothing internally.

The odometer reports two values:
- **distance** — metres since the last reset command
- **totalDistance** — total cumulative metres since the receiver was first configured

---

## Antenna

### Active vs Passive Antenna

A **passive antenna** is simply a metallic element cut to the right length to resonate at the GPS frequency. It has no electronics. The signal arrives at the receiver at whatever strength the physics of the path dictates.

An **active antenna** contains a **LNA** (Low-Noise Amplifier) built into or near the antenna element. This amplifies the satellite signal before it travels down the cable to the receiver, overcoming the cable's signal loss. Active antennas require a small supply voltage (typically 3.3 V or 5 V) fed through the coaxial cable from the receiver.

For any cable run longer than about 10 cm, an active antenna is strongly recommended. The u-blox NEO modules can supply and detect the antenna supply voltage, which is how the MON-HW antenna status works.

### Antenna Status Codes

The `aStatus` field in MON-HW reports the result of the receiver's antenna monitoring circuit:

| Code | Status | Meaning |
|------|--------|---------|
| `0` | INIT | The receiver is still initialising the antenna detection circuit |
| `1` | UNKNOWN | Antenna type cannot be determined |
| `2` | OK | Antenna is connected and operating normally |
| `3` | SHORT | Antenna cable is shorted to ground — check for a damaged cable |
| `4` | OPEN | No antenna detected — cable disconnected or antenna missing |

The `aPower` field reports whether the receiver is supplying power to the antenna (`0` = off, `1` = on). A passive antenna will show `aPower = 0` and `aStatus = UNKNOWN`, which is normal.

---

## Baud Rate

**Baud rate** is the speed of the serial UART communication between your microcontroller and the GPS module, measured in bits per second (bps). Common values are 9600, 38400, 115200, and 230400.

Higher baud rates allow more data to be transferred per second, which becomes important at high update rates (10 Hz) or when receiving verbose messages like NAV-SAT. At 9600 baud, a large NAV-SAT message with 30 satellites takes about 40 ms to transmit, which is most of your 100 ms window at 10 Hz.

GPS modules ship from the factory at **9600 baud** by default.

BetterGPS detects the current baud rate automatically by trying each standard rate and checking for a valid UBX response. The tested rates are: 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800.

---

## Hot Start, Warm Start, Cold Start

These terms describe how much data the GPS receiver retains before starting up, which directly affects how quickly it achieves a fix.

**Hot start** — the receiver retained its ephemeris data, almanac, last known position, and time from the previous session (stored in battery-backed RAM). It knows exactly where the satellites are and can acquire them almost instantly. TTFF is typically under 1 second under open sky.

**Warm start** — the ephemeris has expired or been cleared (e.g. after being off for more than a few hours), but the almanac and approximate position are retained. The receiver knows roughly where to look for satellites but must download fresh ephemeris data. TTFF is typically 10–45 seconds.

**Cold start** — all stored data has been cleared. The receiver has no prior knowledge and must discover satellite positions from scratch by searching across all frequencies and downloading a full almanac. TTFF is typically 30–120 seconds under open sky. This is the worst case.

In BetterGPS, the CFG-RST commands map to these start types:
- `resetHot()` — hot start, fastest
- `resetWarm()` — warm start, clears ephemeris
- `resetCold()` — cold start, clears everything

---

## BBR and Flash Storage

A u-blox receiver has three storage locations for its configuration:

**RAM** — the active working configuration. Always starts fresh from the defaults when power is removed, unless a save has been performed.

**BBR** (Battery-Backed RAM) — a small section of RAM kept alive by a backup battery or supercapacitor on the module. Survives power cycling as long as the backup battery has charge. If the backup battery dies, BBR is lost. This is where ephemeris data and the current configuration are normally stored.

**Flash** — internal non-volatile flash memory. Survives power cycling without any battery. Not all module variants have flash memory — the standard NEO-6M and NEO-8M modules do have it, but some smaller variants do not.

When you call `saveConfig()`, the library sends a CFG-CFG command telling the receiver to copy its current RAM configuration to the selected storage. `SaveLocation::Both` writes to both flash and BBR simultaneously, which is the most reliable option.

---

## Power Modes

GPS receivers consume significant current (~25–50 mA typical). For battery-powered applications, reducing GPS power consumption is important.

**Continuous mode** — the receiver tracks all satellites all the time at full power. Best accuracy, fastest re-acquisition, highest current.

**Power save mode** (CFG-RXM, lpMode = 1) — the receiver enters a duty-cycle operation where it periodically turns off its RF front end and tracking loops. The duty cycle depends on the configured update rate. Position is only updated when the receiver wakes up. Current can drop to 5–15 mA average. The trade-off is slower response to movement and occasional position gaps.

For applications that only need a position every 10–30 seconds (asset tracking, weather station), power save mode can extend battery life significantly.

---

## Quick Reference Glossary

| Term | Meaning |
|------|---------|
| **ACK** | Acknowledgement — receiver accepted a configuration command |
| **NAK** | Negative acknowledgement — receiver rejected a command |
| **AGC** | Automatic Gain Control — amplification adjustment circuit |
| **Almanac** | Coarse orbital data for all satellites, valid for weeks |
| **Azimuth** | Horizontal angle to a satellite, clockwise from north |
| **BBR** | Battery-Backed RAM — survives power cycling if battery present |
| **Baud rate** | Serial communication speed in bits per second |
| **BeiDou** | Chinese satellite navigation system |
| **CFG** | Configuration — UBX message class for receiver settings |
| **CNO** | Carrier-to-Noise Density — signal strength in dBHz |
| **Cold start** | Receiver has no stored data, full reacquisition required |
| **Dead reckoning** | Estimating position from last known position + movement |
| **DGPS** | Differential GPS — uses correction stations for ~1 m accuracy |
| **DOP** | Dilution of Precision — geometry-based accuracy multiplier |
| **Doppler** | Frequency shift due to relative motion between satellite and receiver |
| **DST** | Daylight Saving Time — seasonal +1 hour offset |
| **Elevation** | Angle of satellite above the horizon (0°–90°) |
| **Ephemeris** | Precise orbital data for one satellite, valid for ~4 hours |
| **Galileo** | European satellite navigation system |
| **GLONASS** | Russian satellite navigation system |
| **GNSS** | Global Navigation Satellite System — umbrella term for all constellations |
| **GPS** | Global Positioning System — the original US system (also used as a generic term) |
| **hAcc** | Horizontal accuracy estimate in millimetres |
| **HDOP** | Horizontal Dilution of Precision |
| **height** | Height above WGS-84 ellipsoid in millimetres |
| **hMSL** | Height above Mean Sea Level in millimetres |
| **Hot start** | Receiver has full stored data, near-instant re-fix |
| **iTOW** | Integer Time Of Week — milliseconds since Sunday midnight GPS time |
| **Jamming** | Deliberate radio interference blocking GPS reception |
| **Leap second** | Occasional 1-second adjustment to keep UTC aligned with Earth rotation |
| **LNA** | Low-Noise Amplifier — in active antennas |
| **MON** | Monitor — UBX class for hardware status messages |
| **Multipath** | Signal reflections off buildings causing positioning errors |
| **NAV** | Navigation — UBX class for position/velocity/time results |
| **NED** | North-East-Down coordinate frame for velocity |
| **NMEA** | ASCII text protocol for GPS data (alternative to UBX) |
| **Odometer** | Accumulated distance counter (NEO-8M) |
| **PDOP** | Position Dilution of Precision (3D) |
| **Poll** | Request sent to receiver asking for current data |
| **PRN** | Pseudo-Random Noise code — unique identifier per satellite |
| **Pseudorange** | Approximate satellite distance measurement, with clock errors |
| **QZSS** | Japanese regional satellite system |
| **RTK** | Real-Time Kinematics — centimetre-level positioning using carrier phase |
| **RXM** | Receiver Manager — UBX class for raw signal measurements |
| **SBAS** | Satellite-Based Augmentation System — broadcasts differential corrections |
| **Subframe** | 6-second chunk of navigation message broadcast by each satellite |
| **TTFF** | Time To First Fix — how long until receiver computes first valid position |
| **UBX** | u-blox binary protocol for GPS receiver communication |
| **UTC** | Coordinated Universal Time — civil time standard, includes leap seconds |
| **UTM** | Universal Transverse Mercator — coordinate system in metres |
| **vAcc** | Vertical accuracy estimate in millimetres |
| **VDOP** | Vertical Dilution of Precision |
| **Warm start** | Ephemeris cleared but almanac and position retained |
| **WGS-84** | World Geodetic System 1984 — the reference ellipsoid used by GPS |
