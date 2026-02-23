/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#include "CoapParser.h"
#include <cstring>

namespace CoapPacket {

CoapError CoapParser::parse(const uint8_t* buffer, size_t length, CoapPacket& packet) {
    // Clear packet first
    packet.clear();

    // 1. Check minimum size (4-byte header)
    if (length < 4) {
        return CoapError::DATAGRAM_TOO_SHORT;
    }

    size_t offset = 0;

    // 2. Parse header (4 bytes)
    uint8_t versionTypeToken = buffer[offset++];

    // Extract version (bits 6-7)
    uint8_t version = (versionTypeToken >> 6) & 0x03;
    if (version != COAP_VERSION) {
        return CoapError::INVALID_VERSION;
    }
    packet.version = version;

    // Extract type (bits 4-5)
    packet.type = static_cast<CoapType>((versionTypeToken >> 4) & 0x03);

    // Extract token length (bits 0-3)
    packet.token_length = versionTypeToken & 0x0F;
    if (packet.token_length > 8) {
        return CoapError::INVALID_TOKEN_LENGTH;
    }

    // Extract code (byte 1)
    packet.code = static_cast<CoapCode>(buffer[offset++]);

    // Validate code class (1, 6, 7 are reserved)
    uint8_t codeClass = getCodeClass(packet.code);
    if (!isValidCodeClass(codeClass)) {
        return CoapError::INVALID_CODE_CLASS;
    }

    // Extract message ID (bytes 2-3, big-endian)
    packet.message_id = (static_cast<uint16_t>(buffer[offset]) << 8) |
                        static_cast<uint16_t>(buffer[offset + 1]);
    offset += 2;

    // 3. Parse token (if any)
    if (packet.token_length > 0) {
        if (offset + packet.token_length > length) {
            return CoapError::DATAGRAM_TOO_SHORT;
        }
        std::memcpy(packet.token, buffer + offset, packet.token_length);
        offset += packet.token_length;
    }

    // 4. Parse options (if any remain)
    bool hasPayload = false;
    if (offset < length) {
        CoapError err = parseOptions(buffer, length, offset, packet.options, hasPayload);
        if (err != CoapError::OK) {
            return err;
        }
    }

    // 5. Parse payload (if marker found)
    if (hasPayload) {
        if (offset >= length) {
            // Payload marker present but no payload data (error)
            return CoapError::INVALID_FORMAT;
        }

        size_t payloadLength = length - offset;
        if (payloadLength > MAX_PAYLOAD_SIZE) {
            return CoapError::PAYLOAD_TOO_LARGE;
        }

        packet.payload.assign(buffer + offset, buffer + length);
    }

    return CoapError::OK;
}

CoapError CoapParser::parse(const std::vector<uint8_t>& buffer, CoapPacket& packet) {
    return parse(buffer.data(), buffer.size(), packet);
}

CoapError CoapParser::decodeOptionDeltaLength(const uint8_t* buffer, size_t bufferLen,
                                               size_t& offset, uint8_t field, uint16_t& result) {
    if (field < 13) {
        // Value is in the 4-bit field itself
        result = field;
        return CoapError::OK;
    } else if (field == 13) {
        // 8-bit extended value
        if (offset >= bufferLen) {
            return CoapError::DATAGRAM_TOO_SHORT;
        }
        result = buffer[offset++] + 13;
        return CoapError::OK;
    } else if (field == 14) {
        // 16-bit extended value (big-endian)
        if (offset + 1 >= bufferLen) {
            return CoapError::DATAGRAM_TOO_SHORT;
        }
        result = ((static_cast<uint16_t>(buffer[offset]) << 8) |
                  static_cast<uint16_t>(buffer[offset + 1])) + 269;
        offset += 2;
        return CoapError::OK;
    } else {
        // Field == 15
        // This is either payload marker (when both delta and length are 15)
        // or an error
        return CoapError::INVALID_FORMAT;
    }
}

CoapError CoapParser::parseOptions(const uint8_t* buffer, size_t bufferLen,
                                    size_t& offset, std::vector<CoapOption>& options,
                                    bool& hasPayload) {
    hasPayload = false;
    uint16_t lastOptionNumber = 0;

    while (offset < bufferLen) {
        uint8_t deltaLengthByte = buffer[offset];

        // Check for payload marker (0xFF)
        if (deltaLengthByte == PAYLOAD_MARKER) {
            hasPayload = true;
            offset++;  // Skip marker
            return CoapError::OK;
        }

        offset++;

        // Extract delta and length fields (4 bits each)
        uint8_t deltaField = (deltaLengthByte >> 4) & 0x0F;
        uint8_t lengthField = deltaLengthByte & 0x0F;

        // Decode delta
        uint16_t delta = 0;
        CoapError err = decodeOptionDeltaLength(buffer, bufferLen, offset, deltaField, delta);
        if (err != CoapError::OK) {
            return err;
        }

        // Decode length
        uint16_t length = 0;
        err = decodeOptionDeltaLength(buffer, bufferLen, offset, lengthField, length);
        if (err != CoapError::OK) {
            return err;
        }

        // Calculate absolute option number
        uint16_t optionNumber = lastOptionNumber + delta;
        lastOptionNumber = optionNumber;

        // Check if option value fits in buffer
        if (offset + length > bufferLen) {
            return CoapError::DATAGRAM_TOO_SHORT;
        }

        // Check option length limit
        if (length > MAX_OPTION_VALUE_SIZE) {
            return CoapError::OPTION_TOO_LONG;
        }

        // Extract option value
        CoapOption option;
        option.number = optionNumber;
        if (length > 0) {
            option.value.assign(buffer + offset, buffer + offset + length);
            offset += length;
        }

        options.push_back(option);
    }

    return CoapError::OK;
}

uint32_t CoapParser::decodeUint(const uint8_t* data, size_t length) {
    if (length == 0) {
        return 0;
    }

    uint32_t value = 0;
    for (size_t i = 0; i < length && i < 4; i++) {
        value = (value << 8) | data[i];
    }
    return value;
}

} // namespace CoapPacket
