/**
 * AirGradient Cellular Payload Decoder
 * Decodes binary payload format for cellular transmission
 */

const { SensorFlag, SensorFieldNames, SensorInfo } = require("./payload_types");

/**
 * Decode metadata byte (byte 0)
 * @param {number} metadata - Metadata byte
 * @returns {Object} { version, sharedPresenceMask }
 */
function decodeMetadata(metadata) {
  const version = metadata & 0x1f; // Bits 0-4
  const sharedPresenceMask = (metadata & 0x20) !== 0; // Bit 5
  return { version, sharedPresenceMask };
}

/**
 * Check if a bit is set in a 64-bit presence mask.
 * Mask is represented as two 32-bit words ({ lo, hi }).
 * @param {{lo:number, hi:number}} mask
 * @param {number} bit
 * @returns {boolean}
 */
function isBitSet64(mask, bit) {
  if (bit < 32) {
    return ((mask.lo >>> bit) & 1) !== 0;
  }
  return ((mask.hi >>> (bit - 32)) & 1) !== 0;
}

/**
 * Read uint16 from buffer (little-endian)
 * @param {Buffer} buffer - Buffer to read from
 * @param {number} offset - Offset to read at
 * @returns {number}
 */
function readUint16LE(buffer, offset) {
  return buffer.readUInt16LE(offset);
}

/**
 * Read int16 from buffer (little-endian)
 * @param {Buffer} buffer - Buffer to read from
 * @param {number} offset - Offset to read at
 * @returns {number}
 */
function readInt16LE(buffer, offset) {
  return buffer.readInt16LE(offset);
}

/**
 * Read uint32 from buffer (little-endian)
 * @param {Buffer} buffer - Buffer to read from
 * @param {number} offset - Offset to read at
 * @returns {number}
 */
function readUint32LE(buffer, offset) {
  return buffer.readUInt32LE(offset);
}

/**
 * Read int8 from buffer
 * @param {Buffer} buffer - Buffer to read from
 * @param {number} offset - Offset to read at
 * @returns {number}
 */
function readInt8(buffer, offset) {
  return buffer.readInt8(offset);
}

/**
 * Read presence mask from buffer (64-bit little-endian)
 * @param {Buffer} buffer - Buffer to read from
 * @param {number} offset - Offset to read at
 * @returns {{lo:number, hi:number}}
 */
function readPresenceMask(buffer, offset) {
  return {
    lo: readUint32LE(buffer, offset),
    hi: readUint32LE(buffer, offset + 4),
  };
}

/**
 * Decode sensor data based on presence mask
 * @param {Buffer} buffer - Buffer containing sensor data
 * @param {number} offset - Starting offset
 * @param {{lo:number, hi:number}} presenceMask - 64-bit presence mask
 * @param {boolean} applyScaling - Apply scaling factors to values
 * @returns {Object} { data, bytesRead }
 */
function decodeSensorData(buffer, offset, presenceMask, applyScaling = true) {
  let currentOffset = offset;
  const data = {};

  // Iterate through flags in ascending order
  for (let flag = 0; flag <= SensorFlag.FLAG_SIGNAL; flag++) {
    if (!isBitSet64(presenceMask, flag)) {
      continue; // Skip if flag not set
    }

    const fieldName = SensorFieldNames[flag];
    const info = SensorInfo[flag];

    if (!fieldName || !info) {
      throw new Error(`Unknown sensor flag ${flag}`);
    }

    // Read value based on type
    if (info.type === "int8") {
      // Signed 8-bit (signal strength)
      const rawValue = readInt8(buffer, currentOffset);
      data[fieldName] = applyScaling ? rawValue / info.scale : rawValue;
      currentOffset += 1;
    } else if (info.type === "uint32") {
      // 32-bit fields (always scalar)
      const rawValue = readUint32LE(buffer, currentOffset);
      data[fieldName] = applyScaling ? rawValue / info.scale : rawValue;
      currentOffset += 4;
    } else if (info.type === "int16") {
      // Signed 16-bit (temperature)
      const rawValue = readInt16LE(buffer, currentOffset);
      data[fieldName] = applyScaling ? rawValue / info.scale : rawValue;
      currentOffset += 2;
    } else {
      // Unsigned 16-bit
      const rawValue = readUint16LE(buffer, currentOffset);
      data[fieldName] = applyScaling ? rawValue / info.scale : rawValue;
      currentOffset += 2;
    }
  }

  return {
    data,
    bytesRead: currentOffset - offset,
  };
}

/**
 * Decode a single reading (presence mask + sensor data)
 * @param {Buffer} buffer - Buffer to decode
 * @param {number} offset - Starting offset
 * @param {boolean} applyScaling - Apply scaling factors
 * @returns {Object} { reading, bytesRead }
 */
function decodeReading(buffer, offset, applyScaling = true) {
  let currentOffset = offset;

  // Read presence mask (8 bytes)
  const presenceMask = readPresenceMask(buffer, currentOffset);
  currentOffset += 8;

  // Decode sensor data
  const { data, bytesRead } = decodeSensorData(
    buffer,
    currentOffset,
    presenceMask,
    applyScaling,
  );
  currentOffset += bytesRead;

  return {
    reading: {
      presenceMask,
      ...data,
    },
    bytesRead: currentOffset - offset,
  };
}

function calculateSensorDataSizeForMask(presenceMask) {
  let size = 0;
  for (let flag = 0; flag <= SensorFlag.FLAG_SIGNAL; flag++) {
    if (!isBitSet64(presenceMask, flag)) {
      continue;
    }
    const info = SensorInfo[flag];
    if (!info) {
      throw new Error(`Unknown sensor flag ${flag}`);
    }
    if (info.type === "int8") {
      size += 1;
    } else if (info.type === "uint32") {
      size += 4;
    } else {
      size += 2;
    }
  }
  return size;
}

/**
 * Decode complete payload with multiple readings
 * @param {Buffer} buffer - Complete payload buffer
 * @param {boolean} applyScaling - Apply scaling factors to sensor values
 * @returns {Object} Decoded payload with header and readings
 */
function decodePayload(buffer, applyScaling = true) {
  if (!Buffer.isBuffer(buffer)) {
    throw new Error("Input must be a Buffer");
  }

  if (buffer.length < 2) {
    throw new Error("Buffer too small (minimum 2 bytes for header)");
  }

  let offset = 0;

  // Decode header (2 bytes)
  const metadata = buffer[offset++];
  const intervalMinutes = buffer[offset++];

  const { version, sharedPresenceMask } = decodeMetadata(metadata);

  if (version !== 0) {
    throw new Error(`Unsupported payload version: ${version}`);
  }

  const header = {
    version,
    sharedPresenceMask,
    intervalMinutes,
  };

  const readings = [];

  if (sharedPresenceMask) {
    if (buffer.length < 2 + 8) {
      throw new Error("Buffer too small for shared presence mask");
    }

    const sharedMask = readPresenceMask(buffer, offset);
    offset += 8;

    const readingDataSize = calculateSensorDataSizeForMask(sharedMask);
    if (readingDataSize === 0) {
      throw new Error("Shared presence mask has no fields");
    }

    const remaining = buffer.length - offset;
    if (remaining % readingDataSize !== 0) {
      throw new Error("Invalid payload length for shared presence mask");
    }

    const readingCount = remaining / readingDataSize;
    for (let i = 0; i < readingCount; i++) {
      const { data, bytesRead } = decodeSensorData(
        buffer,
        offset,
        sharedMask,
        applyScaling,
      );
      if (bytesRead !== readingDataSize) {
        throw new Error("Internal error: decoded size mismatch");
      }
      readings.push({
        presenceMask: sharedMask,
        ...data,
      });
      offset += bytesRead;
    }
  } else {
    while (offset < buffer.length) {
      const { reading, bytesRead } = decodeReading(
        buffer,
        offset,
        applyScaling,
      );
      readings.push(reading);
      offset += bytesRead;
    }
  }

  return {
    header,
    readings,
    readingCount: readings.length,
  };
}

/**
 * Decode payload and return raw values (no scaling applied)
 * @param {Buffer} buffer - Complete payload buffer
 * @returns {Object} Decoded payload with raw sensor values
 */
function decodePayloadRaw(buffer) {
  return decodePayload(buffer, false);
}

/**
 * Convert decoded payload to JSON string
 * @param {Buffer} buffer - Complete payload buffer
 * @param {boolean} pretty - Pretty print JSON
 * @returns {string} JSON string
 */
function decodePayloadToJSON(buffer, pretty = false) {
  const decoded = decodePayload(buffer);
  return pretty ? JSON.stringify(decoded, null, 2) : JSON.stringify(decoded);
}

// Export all functions
module.exports = {
  decodeMetadata,
  isBitSet64,
  readUint16LE,
  readInt16LE,
  readUint32LE,
  readInt8,
  readPresenceMask,
  decodeSensorData,
  decodeReading,
  decodePayload,
  decodePayloadRaw,
  decodePayloadToJSON,
};
