/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef COAP_TYPES_H
#define COAP_TYPES_H

#include <cstdint>

namespace CoapPacket {

// CoAP Protocol Version
constexpr uint8_t COAP_VERSION = 1;

// Maximum payload size per RFC 7252
constexpr uint16_t MAX_PAYLOAD_SIZE = 1024;

// Payload marker byte (0xFF)
constexpr uint8_t PAYLOAD_MARKER = 0xFF;

// Maximum option value size
constexpr uint16_t MAX_OPTION_VALUE_SIZE = 1034;

/**
 * CoAP Message Types (2 bits)
 */
enum class CoapType : uint8_t {
    CON = 0,  // Confirmable
    NON = 1,  // Non-confirmable
    ACK = 2,  // Acknowledgment
    RST = 3   // Reset
};

/**
 * CoAP Message Codes (8 bits: 3-bit class + 5-bit detail)
 * Format: class.detail (e.g., 2.05 = Success Content)
 */
enum class CoapCode : uint8_t {
    // Empty message (0.00)
    EMPTY = 0,

    // Request codes (0.01 - 0.07)
    GET = 1,
    POST = 2,
    PUT = 3,
    DELETE = 4,
    FETCH = 5,
    PATCH = 6,
    IPATCH = 7,

    // Success response codes (2.xx)
    CREATED_2_01 = 65,           // (2 << 5) | 1
    DELETED_2_02 = 66,           // (2 << 5) | 2
    VALID_2_03 = 67,             // (2 << 5) | 3
    CHANGED_2_04 = 68,           // (2 << 5) | 4
    CONTENT_2_05 = 69,           // (2 << 5) | 5
    CONTINUE_2_31 = 95,          // (2 << 5) | 31

    // Client error codes (4.xx)
    BAD_REQUEST_4_00 = 128,      // (4 << 5) | 0
    UNAUTHORIZED_4_01 = 129,     // (4 << 5) | 1
    BAD_OPTION_4_02 = 130,       // (4 << 5) | 2
    FORBIDDEN_4_03 = 131,        // (4 << 5) | 3
    NOT_FOUND_4_04 = 132,        // (4 << 5) | 4
    METHOD_NOT_ALLOWED_4_05 = 133,     // (4 << 5) | 5
    NOT_ACCEPTABLE_4_06 = 134,         // (4 << 5) | 6
    REQUEST_ENTITY_INCOMPLETE_4_08 = 136,  // (4 << 5) | 8
    PRECONDITION_FAILED_4_12 = 140,    // (4 << 5) | 12
    REQUEST_ENTITY_TOO_LARGE_4_13 = 141,   // (4 << 5) | 13
    UNSUPPORTED_CONTENT_FORMAT_4_15 = 143, // (4 << 5) | 15

    // Server error codes (5.xx)
    INTERNAL_SERVER_ERROR_5_00 = 160,  // (5 << 5) | 0
    NOT_IMPLEMENTED_5_01 = 161,        // (5 << 5) | 1
    BAD_GATEWAY_5_02 = 162,            // (5 << 5) | 2
    SERVICE_UNAVAILABLE_5_03 = 163,    // (5 << 5) | 3
    GATEWAY_TIMEOUT_5_04 = 164,        // (5 << 5) | 4
    PROXYING_NOT_SUPPORTED_5_05 = 165  // (5 << 5) | 5
};

/**
 * CoAP Option Numbers
 */
enum class CoapOptionNumber : uint16_t {
    IF_MATCH = 1,
    URI_HOST = 3,
    ETAG = 4,
    IF_NONE_MATCH = 5,
    OBSERVE = 6,
    URI_PORT = 7,
    LOCATION_PATH = 8,
    URI_PATH = 11,
    CONTENT_FORMAT = 12,
    MAX_AGE = 14,
    URI_QUERY = 15,
    ACCEPT = 17,
    LOCATION_QUERY = 20,
    BLOCK2 = 23,
    BLOCK1 = 27,
    SIZE2 = 28,
    PROXY_URI = 35,
    PROXY_SCHEME = 39,
    SIZE1 = 60
};

/**
 * CoAP Content Format Codes
 */
enum class CoapContentFormat : uint16_t {
    TEXT_PLAIN = 0,
    LINK_FORMAT = 40,
    XML = 41,
    OCTET_STREAM = 42,
    EXI = 47,
    JSON = 50,
    CBOR = 60
};

/**
 * Helper function to get code class (3 most significant bits)
 */
inline uint8_t getCodeClass(CoapCode code) {
    return static_cast<uint8_t>(code) >> 5;
}

/**
 * Helper function to get code detail (5 least significant bits)
 */
inline uint8_t getCodeDetail(CoapCode code) {
    return static_cast<uint8_t>(code) & 0x1F;
}

/**
 * Helper function to create a CoAP code from class and detail
 */
inline CoapCode makeCode(uint8_t codeClass, uint8_t detail) {
    return static_cast<CoapCode>((codeClass << 5) | detail);
}

/**
 * Check if code class is valid (1, 6, 7 are reserved)
 */
inline bool isValidCodeClass(uint8_t codeClass) {
    return codeClass != 1 && codeClass != 6 && codeClass != 7;
}

} // namespace CoapPacket

#endif // COAP_TYPES_H
