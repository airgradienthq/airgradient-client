# CoapPacket Library

A lightweight, platform-agnostic C++ library for building and parsing CoAP (Constrained Application Protocol) UDP packets.

## Overview

CoapPacket is a minimal CoAP implementation focused solely on packet construction and parsing. Unlike full CoAP stacks, this library:

- **Only handles packet building and parsing** - no networking, no state management
- **Platform agnostic** - pure C++ with no hardware dependencies
- **Builder pattern API** - fluent, easy-to-use interface
- **Zero external dependencies** - only uses C++ standard library (STL)
- **Lightweight** - extracted from lobaro-coap, keeping only the essentials

Perfect for embedded systems or applications that need CoAP packet handling without the full protocol stack overhead.

## Features

- ✅ Build CoAP packets using fluent builder pattern
- ✅ Parse CoAP packets from UDP datagrams
- ✅ Support for all CoAP message types (CON, NON, ACK, RST)
- ✅ Support for all standard CoAP codes (GET, POST, PUT, DELETE, response codes)
- ✅ CoAP option handling with automatic delta encoding/decoding
- ✅ Automatic option sorting
- ✅ Convenience methods for common operations (Uri-Path, Uri-Query, Content-Format)
- ✅ Comprehensive error handling without exceptions or templates
- ✅ RFC 7252 compliant

## Requirements

- **C++11 or later**
- **No external dependencies** (uses only STL)

## Installation

Simply copy the `lib/coap-packet` directory into your project and include the headers.

### Directory Structure

```
lib/coap-packet/
├── include/
│   └── coap-packet/
│       ├── CoapBuilder.h      # Builder pattern API
│       ├── CoapParser.h       # Parser API
│       ├── CoapPacket.h       # Packet structure
│       ├── CoapTypes.h        # Enums and constants
│       └── CoapError.h        # Error codes
├── src/
│   ├── CoapBuilder.cpp
│   └── CoapParser.cpp
└── examples/
    └── basic_usage.cpp
```

## Quick Start

### Building a CoAP GET Request

```cpp
#include "coap-packet/CoapBuilder.h"

using namespace CoapPacket;

CoapBuilder builder;
std::vector<uint8_t> udpBuffer;

uint8_t token[] = {0x12, 0x34};

CoapError err = builder
    .setType(CoapType::CON)
    .setCode(CoapCode::GET)
    .setMessageId(1234)
    .setToken(token, 2)
    .setUriPath("/sensors/temp")
    .buildBuffer(udpBuffer);

if (err == CoapError::OK) {
    // Send udpBuffer via UDP socket
}
```

### Building a POST Request with Payload

```cpp
CoapBuilder builder;
std::vector<uint8_t> udpBuffer;

uint8_t token[] = {0xAB, 0xCD};
std::string jsonData = "{\"temperature\":22.5}";

CoapError err = builder
    .setType(CoapType::CON)
    .setCode(CoapCode::POST)
    .setMessageId(5678)
    .setToken(token, 2)
    .setUriPath("/sensors/data")
    .setContentFormat(CoapContentFormat::JSON)
    .setPayload(jsonData)
    .buildBuffer(udpBuffer);
```

### Parsing a CoAP Response

```cpp
#include "coap-packet/CoapParser.h"

using namespace CoapPacket;

// Received UDP datagram
uint8_t* udpData = ...; // from socket
size_t udpLength = ...;

CoapPacket packet;
CoapError err = CoapParser::parse(udpData, udpLength, packet);

if (err == CoapError::OK) {
    // Access packet data
    uint16_t msgId = packet.message_id;
    CoapCode code = packet.code;

    // Check if it's a successful response
    if (code == CoapCode::CONTENT_2_05) {
        std::string payload(packet.payload.begin(), packet.payload.end());
        std::cout << "Received: " << payload << std::endl;
    }
}
```

## License

MIT License

## References

- [RFC 7252: The Constrained Application Protocol (CoAP)](https://tools.ietf.org/html/rfc7252)
- [lobaro-coap GitHub Repository](https://github.com/Lobaro/lobaro-coap)

## Contributing

This library is designed to be minimal and focused. If you need additional features, consider using libcoap or lobaro-coap
