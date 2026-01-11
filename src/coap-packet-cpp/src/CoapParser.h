/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef COAP_PARSER_H
#define COAP_PARSER_H

#include "CoapPacket.h"
#include "CoapError.h"
#include <vector>

namespace CoapPacket {

/**
 * Parser class for parsing CoAP packets from UDP datagrams
 */
class CoapParser {
public:
    /**
     * Parse CoAP packet from raw buffer
     * Returns CoapError::OK on success, error code otherwise
     */
    static CoapError parse(const uint8_t* buffer, size_t length, CoapPacket& packet);

    /**
     * Parse CoAP packet from vector
     * Returns CoapError::OK on success, error code otherwise
     */
    static CoapError parse(const std::vector<uint8_t>& buffer, CoapPacket& packet);

private:
    /**
     * Decode option delta or length value
     * Returns decoded value and updates offset
     */
    static CoapError decodeOptionDeltaLength(const uint8_t* buffer, size_t bufferLen,
                                             size_t& offset, uint8_t field, uint16_t& result);

    /**
     * Parse all options from buffer
     * Returns pointer to payload start (or nullptr if no payload)
     */
    static CoapError parseOptions(const uint8_t* buffer, size_t bufferLen,
                                   size_t& offset, std::vector<CoapOption>& options,
                                   bool& hasPayload);

    /**
     * Decode uint from variable-length big-endian bytes
     */
    static uint32_t decodeUint(const uint8_t* data, size_t length);
};

} // namespace CoapPacket

#endif // COAP_PARSER_H
