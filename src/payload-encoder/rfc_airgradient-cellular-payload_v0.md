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
  Mode A (default): Presence mask per reading

  [ 1 Byte ] [ 1 Byte ] [ 8 Bytes ] [ Variable ] [ 8 Bytes ] [ Variable ] ...
  +----------+-----------+---------------+-------------+---------------+-------------+
  | Metadata | Interval  | Presence Mask | Sensor Data | Presence Mask | Sensor Data | ...
  +----------+-----------+---------------+-------------+---------------+-------------+
       ^          ^              ^              ^              ^              ^
       |          |              |              |              |              |
       |          |              +-- Reading 1--+              +-- Reading 2--+  ...
       |          |
       +-- Shared header for all readings in this batch

  Mode B: Shared presence mask (Metadata bit 5 = 1)

  [ 1 Byte ] [ 1 Byte ] [ 8 Bytes ] [ Variable ] [ Variable ] [ Variable ] ...
  +----------+-----------+-------------------+-------------+-------------+-------------+
  | Metadata | Interval  | Shared Mask       | Sensor Data | Sensor Data | Sensor Data | ...
  +----------+-----------+-------------------+-------------+-------------+-------------+
       ^          ^              ^                   ^             ^
       |          |              |                   |             |
       |          |              +-- Applies to all readings -------+
       |          |
       +-- Shared header for all readings in this batch
```

Batch header size: **2 bytes** (Metadata + Interval)

If `SHARED_PRESENCE_MASK` is `0`, each reading starts with an **8-byte presence mask**, followed by sensor fields.

If `SHARED_PRESENCE_MASK` is `1`, a single **8-byte shared presence mask** is sent once after the header, then all readings contain only sensor fields.

> Presence mask and sensor data can be repeated based on total cache. If `SHARED_PRESENCE_MASK` is enabled, the presence mask is sent once for the whole batch.

### Byte 0: Metadata

| **Bit Index** | **Name**                   | **Value** | **Description**                                                                                         |
| ------------- | -------------------------- | --------- | ------------------------------------------------------------------------------------------------------- |
| **0-4**       | `VERSION`                  | `0` - `31` | Payload Schema Version (e.g., set to 0).                                                               |
| **5**         | `SHARED_PRESENCE_MASK`     | `0` / `1` | **0:** Each reading includes a presence mask.<br><br>**1:** A single shared mask applies to all readings. |
| **6-7**       | `RESERVED`                 | `0`       | Reserved for future use.                                                                                |

### Bytes 1: Interval

Measurement Interval in minutes

### Presence Mask (64-bit Integer)

Presence mask is a 64-bit integer (8 bytes, little-endian).

Implementation note: on 32-bit MCUs, this can be handled as two 32-bit words (lower 32 bits, upper 32 bits).

- If `SHARED_PRESENCE_MASK` is `0`: each reading begins with its own presence mask.
- If `SHARED_PRESENCE_MASK` is `1`: the payload contains a single shared presence mask immediately after the 2-byte header.

This mask determines which data fields follow the header.

When a bit is set to `1`, the corresponding field is present in the payload and will be serialized in the Sensor Data section.

| **Bit**   | **Flag Macro**          | **Data Type** | **Scale** | **Unit / Note**                |
| --------- | --------------- | ------------- | --------- | ------------------------------ |
| **0**     | `FLAG_TEMP`             | `int16_t`     | 100       | Celsius                        |
| **1**     | `FLAG_HUM`              | `uint16_t`    | 100       | %                              |
| **2**     | `FLAG_CO2`              | `uint16_t`    | 1         | ppm                            |
| **3**     | `FLAG_TVOC`             | `uint16_t`    | 1         | Index Value                    |
| **4**     | `FLAG_TVOC_RAW`         | `uint16_t`    | 1         | Raw Value                      |
| **5**     | `FLAG_NOX`              | `uint16_t`    | 1         | Index Value                    |
| **6**     | `FLAG_NOX_RAW`          | `uint16_t`    | 1         | Raw Value                      |
| **7**     | `FLAG_PM_01`            | `uint16_t`    | 10        | PM 1.0 (Atmospheric)           |
| **8**     | `FLAG_PM_25_CH1`        | `uint16_t`    | 10        | PM 2.5 (Atmospheric) Channel 1 |
| **9**     | `FLAG_PM_25_CH2`        | `uint16_t`    | 10        | PM 2.5 (Atmospheric) Channel 2 |
| **10**    | `FLAG_PM_10`            | `uint16_t`    | 10        | PM 10 (Atmospheric)            |
| **11**    | `FLAG_PM_01_SP`         | `uint16_t`    | 10        | PM 1.0 (Standard Particle)     |
| **12**    | `FLAG_PM_25_SP_CH1`     | `uint16_t`    | 10        | PM 2.5 (Standard Particle) CH1 |
| **13**    | `FLAG_PM_25_SP_CH2`     | `uint16_t`    | 10        | PM 2.5 (Standard Particle) CH2 |
| **14**    | `FLAG_PM_10_SP`         | `uint16_t`    | 10        | PM 10 (Standard Particle)      |
| **15**    | `FLAG_PM_03_PC_CH1`     | `uint16_t`    | 1         | Count 0.3µm Channel 1          |
| **16**    | `FLAG_PM_03_PC_CH2`     | `uint16_t`    | 1         | Count 0.3µm Channel 2          |
| **17**    | `FLAG_PM_05_PC`         | `uint16_t`    | 1         | Count 0.5µm                    |
| **18**    | `FLAG_PM_01_PC`         | `uint16_t`    | 1         | Count 1.0µm                    |
| **19**    | `FLAG_PM_25_PC`         | `uint16_t`    | 1         | Count 2.5µm                    |
| **20**    | `FLAG_PM_5_PC`          | `uint16_t`    | 1         | Count 5.0µm                    |
| **21**    | `FLAG_PM_10_PC`         | `uint16_t`    | 1         | Count 10µm                     |
| **22**    | `FLAG_VBAT`             | `uint16_t`    | 100       | Battery Voltage (mV)           |
| **23**    | `FLAG_VPANEL`           | `uint16_t`    | 100       | Panel/Charger Voltage (mV)     |
| **24**    | `FLAG_O3_WE`            | `uint32_t`    | 1000      | O3 Working Electrode (mV/Raw)  |
| **25**    | `FLAG_O3_AE`            | `uint32_t`    | 1000      | O3 Aux Electrode (mV/Raw)      |
| **26**    | `FLAG_NO2_WE`           | `uint32_t`    | 1000      | NO2 Working Electrode (mV/Raw) |
| **27**    | `FLAG_NO2_AE`           | `uint32_t`    | 1000      | NO2 Aux Electrode (mV/Raw)     |
| **28**    | `FLAG_AFE_TEMP`         | `uint16_t`    | 10        | AFE Chip Temperature           |
| **29**    | `FLAG_SIGNAL`           | `int8_t`      | 1         | Signal in DBM                  |
| **30-63** | `RESERVED`              | -             |           | Reserved for future expansion  |

### Sensor Data

#### Rule of Order

Data fields are serialized in **ascending order of their Presence Bit Index** in the Presence Mask.

- If `SHARED_PRESENCE_MASK` is `0`, use the reading's own presence mask.
- If `SHARED_PRESENCE_MASK` is `1`, use the shared presence mask for every reading.

1. The parser checks **Bit 0**. If set (`1`), the data for `_temperature` is read first.
2. The parser checks **Bit 1**. If set (`1`), the data for `_humidity` is read next.
3. The parser continues this check sequentially up to **Bit 29**.
4. If a **Bit** is set to `0` that field is skipped entirely (0 bytes on wire).

**Crucial:** The position of a field in the payload depends entirely on which _previous_ bits were set.

#### Reading Count (When Shared Presence Mask Is Enabled)

When `SHARED_PRESENCE_MASK` is `1`, the number of readings is inferred from payload length:

- `reading_data_size` = sum of byte sizes for all fields whose bits are set in the shared mask.
- `reading_count` = `(payload_length - (METADATA_SIZE + INTERVAL_SIZE) - (PRESENCE_MASK_SIZE)) / reading_data_size`

The payload is invalid if the division is not an integer.

The payload is also invalid if `reading_data_size` is `0` (shared mask has no fields).

#### Two-Channel Fields

Most fields are single values.

For the following sensors, the payload uses separate presence-mask bits for Channel 1 and Channel 2:

- `FLAG_PM_25_CH1` / `FLAG_PM_25_CH2`
- `FLAG_PM_25_SP_CH1` / `FLAG_PM_25_SP_CH2`
- `FLAG_PM_03_PC_CH1` / `FLAG_PM_03_PC_CH2`

If both channel bits are set, both values are serialized (in ascending bit order). If only one is set, only that channel value is present.

#### Example

##### Single Values (Temp + CO2)

- **Metadata:** `0x00` (Ver=0)
- **Mask:** `0x0000000000000005` (Bits 0 & 2 set: Temp + CO2)

**Decoding Stream:**

1. **Read Header:** Version is **0**.
2. **Check Bit 0 (Temp):** Set.
    - Read 2 Bytes (`int16_t`). -> `Temp`
3. **Check Bit 1 (Hum):** Not set. Skip.
4. **Check Bit 2 (CO2):** Set.
    - Read 2 Bytes (`uint16_t`). -> `CO2`
5. **End of Payload.**

**Total Sensor Data Size:** 4 Bytes.

##### Shared Presence Mask With Multiple Readings (Temp + CO2)

- **Metadata:** `0x20` (Ver=0, Shared Mask=1)
- **Shared Mask:** `0x0000000000000005` (Bits 0 & 2 set: Temp + CO2)

**Decoding Stream:**

1. **Read Header:** Version is **0**, Shared Mask is **enabled**.
2. **Read Shared Mask:** `0x0000000000000005`.
3. **Reading 1:**
    - Temp: Read 2 Bytes (`int16_t`).
    - CO2: Read 2 Bytes (`uint16_t`).
4. **Reading 2:**
    - Temp: Read 2 Bytes (`int16_t`).
    - CO2: Read 2 Bytes (`uint16_t`).
5. Continue until end of payload.

**Per-Reading Sensor Data Size:** 4 Bytes.

##### Two-Channel PM2.5 Atmospheric (CH1 + CH2)

- **Metadata:** `0x00` (Ver=0)
- **Mask:** `0x0000000000000300` (Bits 8 & 9 set: PM2.5 CH1 + PM2.5 CH2)

**Decoding Stream:**

1. **Read Header:** Version is **0**.
2. ... skip Bits 0-7
3. **Check Bit 8 (PM2.5 CH1):** Set.
    - Read 2 Bytes (`uint16_t`). -> `PM25_CH1`
4. **Check Bit 9 (PM2.5 CH2):** Set.
    - Read 2 Bytes (`uint16_t`). -> `PM25_CH2`
5. **End of Payload.**

**Total Sensor Data Size:** 4 Bytes.
