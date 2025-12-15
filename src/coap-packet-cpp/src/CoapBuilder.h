#ifndef COAP_BUILDER_H
#define COAP_BUILDER_H

#include "CoapPacket.h"
#include "CoapError.h"
#include <string>
#include <algorithm>

namespace CoapPacket {

/**
 * Builder class for constructing CoAP packets using the builder pattern
 */
class CoapBuilder {
public:
    CoapBuilder();

    /**
     * Set message type (CON, NON, ACK, RST)
     */
    CoapBuilder& setType(CoapType type);

    /**
     * Set message code (GET, POST, response codes, etc.)
     */
    CoapBuilder& setCode(CoapCode code);

    /**
     * Set message ID
     */
    CoapBuilder& setMessageId(uint16_t id);

    /**
     * Set token from buffer
     */
    CoapBuilder& setToken(const uint8_t* token, uint8_t length);

    /**
     * Add option with raw byte value
     */
    CoapBuilder& addOption(CoapOptionNumber optionNum, const std::vector<uint8_t>& value);

    /**
     * Add option with string value
     */
    CoapBuilder& addOption(CoapOptionNumber optionNum, const std::string& value);

    /**
     * Add option with uint32 value (encoded as variable-length big-endian)
     */
    CoapBuilder& addOption(CoapOptionNumber optionNum, uint32_t value);

    /**
     * Convenience: Set URI path (e.g., "/sensors/temp")
     * Automatically splits on '/' and creates multiple URI_PATH options
     */
    CoapBuilder& setUriPath(const std::string& path);

    /**
     * Convenience: Add a single URI path segment
     */
    CoapBuilder& addUriPathSegment(const std::string& segment);

    /**
     * Convenience: Add URI query parameter (e.g., "key=value")
     */
    CoapBuilder& addUriQuery(const std::string& key, const std::string& value);

    /**
     * Convenience: Set content format
     */
    CoapBuilder& setContentFormat(CoapContentFormat format);

    /**
     * Set payload from vector
     */
    CoapBuilder& setPayload(const std::vector<uint8_t>& data);

    /**
     * Set payload from string
     */
    CoapBuilder& setPayload(const std::string& data);

    /**
     * Set payload from raw buffer
     */
    CoapBuilder& setPayload(const uint8_t* data, size_t length);

    /**
     * Build the packet structure
     * Returns CoapError::OK on success, error code otherwise
     */
    CoapError build(CoapPacket& packet);

    /**
     * Build directly to UDP buffer (ready to send)
     * Returns CoapError::OK on success, error code otherwise
     */
    CoapError buildBuffer(std::vector<uint8_t>& buffer);

    /**
     * Get the last error that occurred
     */
    CoapError getLastError() const;

    /**
     * Reset builder to initial state
     */
    void reset();

private:
    CoapPacket packet_;
    CoapError lastError_;

    /**
     * Sort options by option number (required by CoAP spec)
     */
    void sortOptions();

    /**
     * Encode option delta and length using CoAP delta encoding
     */
    size_t encodeOptionDeltaLength(uint8_t* buffer, uint16_t delta, uint16_t length);

    /**
     * Encode uint32 as variable-length big-endian bytes
     */
    std::vector<uint8_t> encodeUint(uint32_t value);

    /**
     * Pack all options into buffer using delta encoding
     */
    CoapError packOptions(std::vector<uint8_t>& buffer);

    /**
     * Validate packet before building
     */
    CoapError validate();
};

} // namespace CoapPacket

#endif // COAP_BUILDER_H
