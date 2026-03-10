/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef COAP_PACKET_H
#define COAP_PACKET_H

#include "CoapTypes.h"
#include <vector>
#include <cstdint>
#include <cstring>

namespace CoapPacket {

/**
 * Represents a single CoAP option
 */
struct CoapOption {
    uint16_t number;
    std::vector<uint8_t> value;

    CoapOption() : number(0) {}
    CoapOption(uint16_t num, const std::vector<uint8_t>& val)
        : number(num), value(val) {}
    CoapOption(uint16_t num, const uint8_t* data, size_t len)
        : number(num), value(data, data + len) {}
};

/**
 * Represents a complete CoAP packet
 */
struct CoapPacket {
    uint8_t version;
    CoapType type;
    uint8_t token_length;
    uint8_t token[8];
    CoapCode code;
    uint16_t message_id;
    std::vector<CoapOption> options;
    std::vector<uint8_t> payload;

    /**
     * Default constructor - initializes to empty packet
     */
    CoapPacket()
        : version(COAP_VERSION)
        , type(CoapType::CON)
        , token_length(0)
        , code(CoapCode::EMPTY)
        , message_id(0) {
        std::memset(token, 0, sizeof(token));
    }

    /**
     * Get pointer to token data
     */
    const uint8_t* getTokenPtr() const {
        return token;
    }

    /**
     * Get pointer to payload data
     */
    const uint8_t* getPayloadPtr() const {
        return payload.empty() ? nullptr : payload.data();
    }

    /**
     * Get payload size
     */
    size_t getPayloadSize() const {
        return payload.size();
    }

    /**
     * Set token from buffer
     */
    void setToken(const uint8_t* tokenData, uint8_t length) {
        if (length > 8) length = 8;
        token_length = length;
        std::memcpy(token, tokenData, length);
        if (length < 8) {
            std::memset(token + length, 0, 8 - length);
        }
    }

    /**
     * Clear all data
     */
    void clear() {
        version = COAP_VERSION;
        type = CoapType::CON;
        token_length = 0;
        std::memset(token, 0, sizeof(token));
        code = CoapCode::EMPTY;
        message_id = 0;
        options.clear();
        payload.clear();
    }
};

} // namespace CoapPacket

#endif // COAP_PACKET_H
