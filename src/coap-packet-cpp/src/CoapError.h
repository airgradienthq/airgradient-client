/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef COAP_ERROR_H
#define COAP_ERROR_H

namespace CoapPacket {

/**
 * CoAP Error Codes
 * Returned by builder and parser methods to indicate success or failure
 */
enum class CoapError {
    OK = 0,

    // Parsing errors
    DATAGRAM_TOO_SHORT,
    INVALID_VERSION,
    INVALID_TOKEN_LENGTH,
    INVALID_CODE_CLASS,
    INVALID_FORMAT,
    TOO_MANY_OPTIONS,
    OPTION_TOO_LONG,
    PAYLOAD_TOO_LARGE,

    // Building errors
    MISSING_REQUIRED_FIELD,
    INVALID_OPTION_NUMBER,
    BUFFER_TOO_SMALL,

    // General errors
    OUT_OF_MEMORY,
    INVALID_ARGUMENT
};

/**
 * Get human-readable error message
 */
inline const char* getErrorMessage(CoapError error) {
    switch (error) {
        case CoapError::OK:
            return "Success";
        case CoapError::DATAGRAM_TOO_SHORT:
            return "Datagram too short (minimum 4 bytes required)";
        case CoapError::INVALID_VERSION:
            return "Invalid CoAP version (expected version 1)";
        case CoapError::INVALID_TOKEN_LENGTH:
            return "Invalid token length (maximum 8 bytes)";
        case CoapError::INVALID_CODE_CLASS:
            return "Invalid code class (1, 6, 7 are reserved)";
        case CoapError::INVALID_FORMAT:
            return "Invalid message format";
        case CoapError::TOO_MANY_OPTIONS:
            return "Too many options";
        case CoapError::OPTION_TOO_LONG:
            return "Option value too long";
        case CoapError::PAYLOAD_TOO_LARGE:
            return "Payload too large (maximum 1024 bytes)";
        case CoapError::MISSING_REQUIRED_FIELD:
            return "Missing required field";
        case CoapError::INVALID_OPTION_NUMBER:
            return "Invalid option number";
        case CoapError::BUFFER_TOO_SMALL:
            return "Buffer too small";
        case CoapError::OUT_OF_MEMORY:
            return "Out of memory";
        case CoapError::INVALID_ARGUMENT:
            return "Invalid argument";
        default:
            return "Unknown error";
    }
}

} // namespace CoapPacket

#endif // COAP_ERROR_H
