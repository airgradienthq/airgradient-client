/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef PAYLOAD_ENCODER_H
#define PAYLOAD_ENCODER_H

#include "PayloadTypes.h"

class PayloadEncoder {
public:
  PayloadEncoder();

  // Initialize encoder with header configuration
  void init(const PayloadHeader &header);

  // Add a sensor reading to the batch
  // Returns: true if added successfully, false if batch full
  bool addReading(const SensorReading &reading);

  // Encode all readings to buffer
  // Returns: number of bytes written, or -1 on error
  int32_t encode(uint8_t *buffer, uint32_t buffer_size);

  // Reset encoder (clear all readings)
  void reset();

  // Get current reading count
  uint8_t getReadingCount() const;

  // Calculate total size needed for current batch
  uint32_t calculateTotalSize() const;

  // Helper functions made public for testing
  uint8_t encodeMetadata() const;
  bool isExpandable(SensorFlag flag) const;
  uint32_t calculateReadingSize(const SensorReading &reading) const;

private:
  EncoderContext ctx;

  // Internal encoding helpers
  void encodePresenceMask(uint8_t *buffer, uint32_t mask) const;
  int32_t encodeSensorData(uint8_t *buffer, uint32_t buffer_size,
                           const SensorReading &reading) const;
  void writeUint16(uint8_t *buffer, uint16_t value) const;
  void writeInt16(uint8_t *buffer, int16_t value) const;
  void writeUint32(uint8_t *buffer, uint32_t value) const;
};

#endif // PAYLOAD_ENCODER_H
