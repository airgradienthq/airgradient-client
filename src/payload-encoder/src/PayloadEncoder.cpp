/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#include "PayloadEncoder.h"
#include <string.h>

PayloadEncoder::PayloadEncoder() { reset(); }

void PayloadEncoder::init(const PayloadHeader &header) {
  reset();
  ctx.header = header;
}

bool PayloadEncoder::addReading(const SensorReading &reading) {
  if (ctx.reading_count >= MAX_BATCH_SIZE) {
    return false;
  }

  ctx.readings[ctx.reading_count++] = reading;
  return true;
}

void PayloadEncoder::reset() { memset(&ctx, 0, sizeof(EncoderContext)); }

uint8_t PayloadEncoder::getReadingCount() const { return ctx.reading_count; }

uint8_t PayloadEncoder::encodeMetadata() const {
  uint8_t metadata = 0;

  // Bits 0-2: VERSION
  metadata |= (ctx.header.version & 0x07);

  // Bit 3: DUAL_MODE
  if (ctx.header.dual_mode) {
    metadata |= (1 << 3);
  }

  // Bit 4: DEDICATED_TEMPHUM_SENSOR
  if (ctx.header.dedicated_temphum_sensor) {
    metadata |= (1 << 4);
  }

  // Bits 5-7: RESERVED (0)

  return metadata;
}

bool PayloadEncoder::isExpandable(SensorFlag flag) const {
  // Based on RFC: fields marked with * are expandable
  switch (flag) {
  case FLAG_TEMP:
  case FLAG_HUM:
    // Temp/Hum are NOT expandable if dedicated sensor is used
    if (ctx.header.dedicated_temphum_sensor) {
      return false;
    }
    return true;

  case FLAG_PM_01:
  case FLAG_PM_25:
  case FLAG_PM_10:
  case FLAG_PM_01_SP:
  case FLAG_PM_25_SP:
  case FLAG_PM_10_SP:
  case FLAG_PM_03_PC:
  case FLAG_PM_05_PC:
  case FLAG_PM_01_PC:
  case FLAG_PM_25_PC:
  case FLAG_PM_5_PC:
  case FLAG_PM_10_PC:
    return true;

  default:
    return false;
  }
}

void PayloadEncoder::encodePresenceMask(uint8_t *buffer, uint32_t mask) const {
  // Write as little-endian 32-bit integer
  buffer[0] = (mask >> 0) & 0xFF;
  buffer[1] = (mask >> 8) & 0xFF;
  buffer[2] = (mask >> 16) & 0xFF;
  buffer[3] = (mask >> 24) & 0xFF;
}

void PayloadEncoder::writeUint16(uint8_t *buffer, uint16_t value) const {
  // Little-endian encoding
  buffer[0] = (value >> 0) & 0xFF;
  buffer[1] = (value >> 8) & 0xFF;
}

void PayloadEncoder::writeInt16(uint8_t *buffer, int16_t value) const {
  // Little-endian encoding
  uint16_t unsigned_value = *((uint16_t *)&value);
  writeUint16(buffer, unsigned_value);
}

void PayloadEncoder::writeUint32(uint8_t *buffer, uint32_t value) const {
  // Little-endian encoding
  buffer[0] = (value >> 0) & 0xFF;
  buffer[1] = (value >> 8) & 0xFF;
  buffer[2] = (value >> 16) & 0xFF;
  buffer[3] = (value >> 24) & 0xFF;
}

int32_t PayloadEncoder::encodeSensorData(uint8_t *buffer, uint32_t buffer_size,
                                         const SensorReading &reading) const {
  uint32_t offset = 0;

  // Iterate through flags in order (0-26)
  for (uint8_t flag = 0; flag <= FLAG_SIGNAL; flag++) {
    if (!IS_FLAG_SET(reading.presence_mask, flag)) {
      continue; // Skip if flag not set
    }

    SensorFlag sensor_flag = (SensorFlag)flag;
    bool expandable = isExpandable(sensor_flag);
    uint8_t value_count = (expandable && ctx.header.dual_mode) ? 2 : 1;

    // Encode based on flag type
    switch (sensor_flag) {
    case FLAG_TEMP:
      for (uint8_t i = 0; i < value_count; i++) {
        if (offset + 2 > buffer_size)
          return -1;
        writeInt16(&buffer[offset], reading.temp[i]);
        offset += 2;
      }
      break;

    case FLAG_HUM:
      for (uint8_t i = 0; i < value_count; i++) {
        if (offset + 2 > buffer_size)
          return -1;
        writeUint16(&buffer[offset], reading.hum[i]);
        offset += 2;
      }
      break;

    case FLAG_CO2:
      if (offset + 2 > buffer_size)
        return -1;
      writeUint16(&buffer[offset], reading.co2);
      offset += 2;
      break;

    case FLAG_TVOC:
      if (offset + 2 > buffer_size)
        return -1;
      writeUint16(&buffer[offset], reading.tvoc);
      offset += 2;
      break;

    case FLAG_TVOC_RAW:
      if (offset + 2 > buffer_size)
        return -1;
      writeUint16(&buffer[offset], reading.tvoc_raw);
      offset += 2;
      break;

    case FLAG_NOX:
      if (offset + 2 > buffer_size)
        return -1;
      writeUint16(&buffer[offset], reading.nox);
      offset += 2;
      break;

    case FLAG_NOX_RAW:
      if (offset + 2 > buffer_size)
        return -1;
      writeUint16(&buffer[offset], reading.nox_raw);
      offset += 2;
      break;

    case FLAG_PM_01:
      for (uint8_t i = 0; i < value_count; i++) {
        if (offset + 2 > buffer_size)
          return -1;
        writeUint16(&buffer[offset], reading.pm_01[i]);
        offset += 2;
      }
      break;

    case FLAG_PM_25:
      for (uint8_t i = 0; i < value_count; i++) {
        if (offset + 2 > buffer_size)
          return -1;
        writeUint16(&buffer[offset], reading.pm_25[i]);
        offset += 2;
      }
      break;

    case FLAG_PM_10:
      for (uint8_t i = 0; i < value_count; i++) {
        if (offset + 2 > buffer_size)
          return -1;
        writeUint16(&buffer[offset], reading.pm_10[i]);
        offset += 2;
      }
      break;

    case FLAG_PM_01_SP:
      for (uint8_t i = 0; i < value_count; i++) {
        if (offset + 2 > buffer_size)
          return -1;
        writeUint16(&buffer[offset], reading.pm_01_sp[i]);
        offset += 2;
      }
      break;

    case FLAG_PM_25_SP:
      for (uint8_t i = 0; i < value_count; i++) {
        if (offset + 2 > buffer_size)
          return -1;
        writeUint16(&buffer[offset], reading.pm_25_sp[i]);
        offset += 2;
      }
      break;

    case FLAG_PM_10_SP:
      for (uint8_t i = 0; i < value_count; i++) {
        if (offset + 2 > buffer_size)
          return -1;
        writeUint16(&buffer[offset], reading.pm_10_sp[i]);
        offset += 2;
      }
      break;

    case FLAG_PM_03_PC:
      for (uint8_t i = 0; i < value_count; i++) {
        if (offset + 2 > buffer_size)
          return -1;
        writeUint16(&buffer[offset], reading.pm_03_pc[i]);
        offset += 2;
      }
      break;

    case FLAG_PM_05_PC:
      for (uint8_t i = 0; i < value_count; i++) {
        if (offset + 2 > buffer_size)
          return -1;
        writeUint16(&buffer[offset], reading.pm_05_pc[i]);
        offset += 2;
      }
      break;

    case FLAG_PM_01_PC:
      for (uint8_t i = 0; i < value_count; i++) {
        if (offset + 2 > buffer_size)
          return -1;
        writeUint16(&buffer[offset], reading.pm_01_pc[i]);
        offset += 2;
      }
      break;

    case FLAG_PM_25_PC:
      for (uint8_t i = 0; i < value_count; i++) {
        if (offset + 2 > buffer_size)
          return -1;
        writeUint16(&buffer[offset], reading.pm_25_pc[i]);
        offset += 2;
      }
      break;

    case FLAG_PM_5_PC:
      for (uint8_t i = 0; i < value_count; i++) {
        if (offset + 2 > buffer_size)
          return -1;
        writeUint16(&buffer[offset], reading.pm_5_pc[i]);
        offset += 2;
      }
      break;

    case FLAG_PM_10_PC:
      for (uint8_t i = 0; i < value_count; i++) {
        if (offset + 2 > buffer_size)
          return -1;
        writeUint16(&buffer[offset], reading.pm_10_pc[i]);
        offset += 2;
      }
      break;

    case FLAG_VBAT:
      if (offset + 2 > buffer_size)
        return -1;
      writeUint16(&buffer[offset], reading.vbat);
      offset += 2;
      break;

    case FLAG_VPANEL:
      if (offset + 2 > buffer_size)
        return -1;
      writeUint16(&buffer[offset], reading.vpanel);
      offset += 2;
      break;

    case FLAG_O3_WE:
      if (offset + 4 > buffer_size)
        return -1;
      writeUint32(&buffer[offset], reading.o3_we);
      offset += 4;
      break;

    case FLAG_O3_AE:
      if (offset + 4 > buffer_size)
        return -1;
      writeUint32(&buffer[offset], reading.o3_ae);
      offset += 4;
      break;

    case FLAG_NO2_WE:
      if (offset + 4 > buffer_size)
        return -1;
      writeUint32(&buffer[offset], reading.no2_we);
      offset += 4;
      break;

    case FLAG_NO2_AE:
      if (offset + 4 > buffer_size)
        return -1;
      writeUint32(&buffer[offset], reading.no2_ae);
      offset += 4;
      break;

    case FLAG_AFE_TEMP:
      if (offset + 2 > buffer_size)
        return -1;
      writeUint16(&buffer[offset], reading.afe_temp);
      offset += 2;
      break;

    case FLAG_SIGNAL:
      if (offset + 1 > buffer_size)
        return -1;
      buffer[offset] = (uint8_t)reading.signal;
      offset += 1;
      break;
    }
  }

  return offset;
}

uint32_t PayloadEncoder::calculateReadingSize(const SensorReading &reading) const {
  uint32_t size = 4; // Presence mask (4 bytes)

  // Iterate through flags and calculate size
  for (uint8_t flag = 0; flag <= FLAG_SIGNAL; flag++) {
    if (!IS_FLAG_SET(reading.presence_mask, flag)) {
      continue;
    }

    SensorFlag sensor_flag = (SensorFlag)flag;
    bool expandable = isExpandable(sensor_flag);
    uint8_t value_count = (expandable && ctx.header.dual_mode) ? 2 : 1;

    // Determine size based on data type
    if (flag == FLAG_SIGNAL) {
      size += 1; // int8_t field
    } else if (flag >= FLAG_O3_WE && flag <= FLAG_NO2_AE) {
      size += 4; // uint32_t fields (O3_WE, O3_AE, NO2_WE, NO2_AE)
    } else if (flag == FLAG_AFE_TEMP) {
      size += 2; // uint16_t field (changed from uint32_t)
    } else {
      size += 2 * value_count; // uint16_t or int16_t fields
    }
  }

  return size;
}

uint32_t PayloadEncoder::calculateTotalSize() const {
  uint32_t size = 2; // Header: Metadata (1) + Interval (1)

  for (uint8_t i = 0; i < ctx.reading_count; i++) {
    size += calculateReadingSize(ctx.readings[i]);
  }

  return size;
}

int32_t PayloadEncoder::encode(uint8_t *buffer, uint32_t buffer_size) {
  if (buffer == nullptr) {
    return -1;
  }

  if (ctx.reading_count == 0) {
    return 0; // No readings to encode
  }

  uint32_t total_size = calculateTotalSize();
  if (total_size > buffer_size) {
    return -1; // Buffer too small
  }

  uint32_t offset = 0;

  // Encode header (Byte 0: Metadata, Byte 1: Interval)
  buffer[offset++] = encodeMetadata();
  buffer[offset++] = ctx.header.interval_minutes;

  // Encode each reading
  for (uint8_t i = 0; i < ctx.reading_count; i++) {
    // Encode presence mask
    encodePresenceMask(&buffer[offset], ctx.readings[i].presence_mask);
    offset += 4;

    // Encode sensor data
    int32_t data_size = encodeSensorData(&buffer[offset], buffer_size - offset, ctx.readings[i]);
    if (data_size < 0) {
      return -1; // Error encoding sensor data
    }
    offset += data_size;
  }

  return offset;
}
