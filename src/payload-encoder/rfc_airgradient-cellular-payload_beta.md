---
tags:
- cellular
- payload
summary: New design of airgradient cellular payload format
---

# Summary

A new AirGradient payload for all monitor that use cellular as transmission network. 

Problem on existing format as follow:

1. Cannot have a dynamic measures value. it needs to follow an order, hence inconsistent across model and harder to maintain
2. Since need to follow order, if only some data that needs to be send, it needs to also send all the delimiters
3. Starting to have a bloated payload size since O3/NO2 (AE & WE)

The new design is a binary payload format that reduce payload size, consistent and easily maintained.

# Specification

## Payload Structure

```
  [ 1 Byte ] [ 1 Byte ] [ 4 Bytes ] [ Variable ] [ 4 Bytes ] [ Variable ] ...
  +----------+-----------+---------------+-------------+---------------+-------------+
  | Metadata | Interval  | Presence Mask | Sensor Data | Presence Mask | Sensor Data | ...
  +----------+-----------+---------------+-------------+---------------+-------------+
       ^          ^              ^              ^              ^              ^
       |          |              |              |              |              |
       |          |              +-- Reading 1--+              +-- Reading 2--+  ...
       |          |
       +-- Shared header for all readings in this batch
```

Total Header Size: **6 Bytes** + Data

> Presence mask and sensor data can be multiple times based on total cache

### Byte 0: Metadata

| **Bit Index** | **Name**                   | **Value** | **Description**                                                                                         |
| ------------- | -------------------------- | --------- | ------------------------------------------------------------------------------------------------------- |
| **0-2**       | `VERSION`                  | `0` - `7` | Payload Schema Version (e.g., set to 1).                                                                |
| **3**         | `DUAL_MODE`                | `0` / `1` | **0:** Single Channel (Arrays send index 0 only).<br><br>**1:** Dual Channel (Arrays send index 0 & 1). |
| **4**         | `DEDICATED_TEMPHUM_SENSOR` | `0` / `1` | 0: Temp/hum from PM sensor<br><br>1: Dedicated temp/hum sensor                                          |
| **5-7**       | `RESERVED`                 | `0`       | Reserved for future use.                                                                                |

### Bytes 1: Interval

Measurement Interval in minutes

### Bytes 2-5 or N-N: Presence Mask (32-bit Integer)

This mask determines which data fields follow the header.

- **If `DUAL_MODE` is 1:** Array types (marked with `*`) send **2 values**.
- **If `DUAL_MODE` is 0:** Array types send **1 value**.
- **If `DEDICATED_TEMPHUM_SENSOR` is 1**: Temperature and Humidity value is from dedicated sensor, hence will ONLY 1 values, even though `DUAL_MODE` is enabled.
- **If `DEDICATED_TEMPHUM_SENSOR` is 0**: Temperature and Humidity value is from PMS sensor, hence can be 2 values depends on `DUAL_MODE` flag.

| **Bit**   | **Flag Macro**  | **Data Type** | **Scale** | **Unit / Note**                |
| --------- | --------------- | ------------- | --------- | ------------------------------ |
| **0**     | `FLAG_TEMP`     | `int16_t` *   | 100       | Celcius                        |
| **1**     | `FLAG_HUM`      | `uint16_t` *  | 100       | %                              |
| **2**     | `FLAG_CO2`      | `uint16_t`    | 1         | ppm                            |
| **3**     | `FLAG_TVOC`     | `uint16_t`    | 1         | Index Value                    |
| **4**     | `FLAG_TVOC_RAW` | `uint16_t`    | 1         | Raw Value                      |
| **5**     | `FLAG_NOX`      | `uint16_t`    | 1         | Index Value                    |
| **6**     | `FLAG_NOX_RAW`  | `uint16_t`    | 1         | Raw Value                      |
| **7**     | `FLAG_PM_01`    | `uint16_t` *  | 10        | PM 1.0 (Atmospheric)           |
| **8**     | `FLAG_PM_25`    | `uint16_t` *  | 10        | PM 2.5 (Atmospheric)           |
| **9**     | `FLAG_PM_10`    | `uint16_t` *  | 10        | PM 10 (Atmospheric)            |
| **10**    | `FLAG_PM_01_SP` | `uint16_t` *  | 10        | PM 1.0 (Standard Particle)     |
| **11**    | `FLAG_PM_25_SP` | `uint16_t` *  | 10        | PM 2.5 (Standard Particle)     |
| **12**    | `FLAG_PM_10_SP` | `uint16_t` *  | 10        | PM 10 (Standard Particle)      |
| **13**    | `FLAG_PM_03_PC` | `uint16_t` *  | 1         | Count 0.3µm                    |
| **14**    | `FLAG_PM_05_PC` | `uint16_t` *  | 1         | Count 0.5µm                    |
| **15**    | `FLAG_PM_01_PC` | `uint16_t` *  | 1         | Count 1.0µm                    |
| **16**    | `FLAG_PM_25_PC` | `uint16_t` *  | 1         | Count 2.5µm                    |
| **17**    | `FLAG_PM_5_PC`  | `uint16_t` *  | 1         | Count 5.0µm                    |
| **18**    | `FLAG_PM_10_PC` | `uint16_t` *  | 1         | Count 10µm                     |
| **19**    | `FLAG_VBAT`     | `uint16_t`    | 100       | Battery Voltage (mV)           |
| **20**    | `FLAG_VPANEL`   | `uint16_t`    | 100       | Panel/Charger Voltage (mV)     |
| **21**    | `FLAG_O3_WE`    | `uint32_t`    | 1000      | O3 Working Electrode (mV/Raw)  |
| **22**    | `FLAG_O3_AE`    | `uint32_t`    | 1000      | O3 Aux Electrode (mV/Raw)      |
| **23**    | `FLAG_NO2_WE`   | `uint32_t`    | 1000      | NO2 Working Electrode (mV/Raw) |
| **24**    | `FLAG_NO2_AE`   | `uint32_t`    | 1000      | NO2 Aux Electrode (mV/Raw)     |
| **25**    | `FLAG_AFE_TEMP` | `uint16_t`    | 10        | AFE Chip Temperature           |
| **26**    | `FLAG_SIGNAL`   | `int8_t`      | 1         | Signal in DBM                  |
| **27-31** | `RESERVED`      | -             |           | Reserved for future expansion  |

### Sensor Data

#### Rule of Order

Data fields are serialized in **ascending order of their Presence Bit Index** in the Presence Mask.

1. The parser checks **Bit 0**. If set (`1`), the data for `_temperature` is read first.
2. The parser checks **Bit 1**. If set (`1`), the data for `_humidity` is read next.
3. The parser continues this check sequentially up to **Bit 25**.
4. If a **Bit** is set to `0` that field is skipped entirely (0 bytes on wire).

**Crucial:** The position of a field in the payload depends entirely on which _previous_ bits were set.

#### Dual Channel Mode Logic and Dedicated Temperature Humidity Sensor

The size of specific fields depends on the `DUAL_MODE` and `DEDICATED_TEMPHUM_SENSOR` flag located in the Metadata Byte.

- **Mode 0 (Single Channel):** All fields are treated as single values. Array fields send only Index `0`.
- **Mode 1 (Dual Channel):** "Expandable" fields send two values (Index `0` followed by Index `1`).
- **Mode 1 (Dual Channel)** & **Dedicated TempHum is 1**: "Expandable" field send two values except temperature and humidity value

**Note:** Scalar fields (e.g., `co2`, `PM count`) are **always** single values, regardless of the Mode.

#### Example

##### SINGLE CHANNEL

- **Metadata:** `0x01` (Ver=1, Dual=0)
- **Mask:** `0x00000005` (Bits 0 & 2 set: Temp + CO2)

**Decoding Stream:**

1. **Read Header:** Mode is **Single**.
2. **Check Bit 0 (Temp):** Set.
    - Read 2 Bytes (`int16`). -> `Temp[0]`
3. **Check Bit 1 (Hum):** Not set. Skip.
4. **Check Bit 2 (CO2):** Set.
    - Read 2 Bytes (`uint16`). -> `CO2`
5. **End of Payload.**

**Total Data Size:** 4 Bytes.

##### DUAL CHANNEL

- **Metadata:** `0x09` (Ver=1, Dual=1). _(Binary: `0000 1001`)_
- **Mask:** `0x00000005` (Bits 0 & 2 set: Temp + CO2)

**Decoding Stream:**

1. **Read Header:** Mode is **Dual**.
2. **Check Bit 0 (Temp):** Set. Field is _Expandable_.
    - Read 2 Bytes (`int16`). -> `Temp[0]`
    - Read 2 Bytes (`int16`). -> `Temp[1]`  
3. **Check Bit 1 (Hum):** Not set. Skip.
4. **Check Bit 2 (CO2):** Set. Field is _Scalar_ (Fixed).
    - Read 2 Bytes (`uint16`). -> `CO2`
5. **End of Payload.**

**Total Data Size:** 6 Bytes.

##### DUAL CHANNEL + DEDICATED TEMPHUM SENSOR

- **Metadata:** `0x19` (Ver=1, Dual=1, Dedicated=1). _(Binary: `0001 1001`)_
- **Mask:** `0x00000107` (Bits 0 & 1 & 2 & 8 set: Temp + Hum + CO2 + PM 25 AE)

**Decoding Stream:**

1. **Read Header:** Mode is **Dual** and Dedicated Temp/Hum sensor enabled
2. **Check Bit 0 (Temp):** Set.  **Dedicated**
    - Read 2 Bytes (`int16`). -> `Temp[0]`
3. **Check Bit 1 (Hum):** Set. **Dedicated**
	- Read 2 Bytes (`int16`). -> `Temp[0]`
4. **Check Bit 2 (CO2):** Set. Field is _Scalar_ (Fixed).
    - Read 2 Bytes (`uint16`). -> `CO2`
5. .... skip
6. **Check Bit 8 (PM25 AE)**: Set. Field is _Expandable_
	- Read 2 Bytes (`uint16_t`). -> `PM25_AE[0]`
	- Read 2 Bytes (`uint16_t`). -> `PM25_AE[1]`
7. **End of Payload.**

**Total Data Size:** 8 Bytes.

