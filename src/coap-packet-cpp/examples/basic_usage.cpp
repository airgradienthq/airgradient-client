#include "../include/coap-packet/CoapBuilder.h"
#include "../include/coap-packet/CoapParser.h"
#include <iostream>
#include <iomanip>

// Helper function to print buffer in hex
void printHex(const std::vector<uint8_t> &buffer) {
  for (size_t i = 0; i < buffer.size(); i++) {
    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i])
              << " ";
    if ((i + 1) % 16 == 0)
      std::cout << std::endl;
  }
  std::cout << std::dec << std::endl;
}

// Helper function to print packet info
void printPacket(const CoapPacket::CoapPacket &packet) {
  std::cout << "Version: " << static_cast<int>(packet.version) << std::endl;
  std::cout << "Type: " << static_cast<int>(packet.type) << std::endl;
  std::cout << "Code: " << static_cast<int>(packet.code) << std::endl;
  std::cout << "Message ID: " << packet.message_id << std::endl;
  std::cout << "Token Length: " << static_cast<int>(packet.token_length) << std::endl;

  if (packet.token_length > 0) {
    std::cout << "Token: ";
    for (int i = 0; i < packet.token_length; i++) {
      std::cout << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(packet.token[i]) << " ";
    }
    std::cout << std::dec << std::endl;
  }

  std::cout << "Options (" << packet.options.size() << "):" << std::endl;
  for (const auto &opt : packet.options) {
    std::cout << "  Option " << opt.number << " (length " << opt.value.size() << "): ";
    // Try to print as string if printable
    bool isPrintable = true;
    for (auto b : opt.value) {
      if (b < 32 || b > 126) {
        isPrintable = false;
        break;
      }
    }
    if (isPrintable && !opt.value.empty()) {
      std::cout << "\"" << std::string(opt.value.begin(), opt.value.end()) << "\"";
    } else {
      for (auto b : opt.value) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b) << " ";
      }
      std::cout << std::dec;
    }
    std::cout << std::endl;
  }

  if (!packet.payload.empty()) {
    std::cout << "Payload (" << packet.payload.size() << " bytes): ";
    // Try to print as string if printable
    bool isPrintable = true;
    for (auto b : packet.payload) {
      if (b < 32 || b > 126) {
        isPrintable = false;
        break;
      }
    }
    if (isPrintable) {
      std::cout << "\"" << std::string(packet.payload.begin(), packet.payload.end()) << "\"";
    } else {
      printHex(packet.payload);
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

int main() {
  std::cout << "=== CoAP Packet Library - Basic Usage Examples ===" << std::endl << std::endl;

  // Example 1: Build a simple GET request
  std::cout << "Example 1: Building a GET request to /sensors/temp" << std::endl;
  std::cout << "---------------------------------------------------" << std::endl;

  CoapPacket::CoapBuilder builder1;
  std::vector<uint8_t> buffer1;

  uint8_t token1[] = {0x12, 0x34};

  CoapPacket::CoapError err1 = builder1.setType(CoapPacket::CoapType::CON)
                                   .setCode(CoapPacket::CoapCode::GET)
                                   .setMessageId(1234)
                                   .setToken(token1, 2)
                                   .setUriPath("/sensors/temp")
                                   .buildBuffer(buffer1);

  if (err1 == CoapPacket::CoapError::OK) {
    std::cout << "✓ Build successful!" << std::endl;
    std::cout << "Buffer size: " << buffer1.size() << " bytes" << std::endl;
    std::cout << "Hex dump:" << std::endl;
    printHex(buffer1);
  } else {
    std::cout << "✗ Build failed: " << CoapPacket::getErrorMessage(err1) << std::endl;
  }
  std::cout << std::endl;

  // Example 2: Parse the buffer we just created
  std::cout << "Example 2: Parsing the GET request" << std::endl;
  std::cout << "-----------------------------------" << std::endl;

  std::vector<uint8_t> test_ = {0x50, 0x01, 0x47, 0xCD, 0xFF, 0x48, 0x65, 0x6C,
                               0x6C, 0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64};

  CoapPacket::CoapPacket packet1;
  CoapPacket::CoapError err2 = CoapPacket::CoapParser::parse(test_, packet1);

  if (err2 == CoapPacket::CoapError::OK) {
    std::cout << "✓ Parse successful!" << std::endl;
    printPacket(packet1);
  } else {
    std::cout << "✗ Parse failed: " << CoapPacket::getErrorMessage(err2) << std::endl;
  }

  // Example 3: Build a POST request with payload
  std::cout << "Example 3: Building a POST request with JSON payload" << std::endl;
  std::cout << "-----------------------------------------------------" << std::endl;

  CoapPacket::CoapBuilder builder3;
  std::vector<uint8_t> buffer3;

  uint8_t token3[] = {0xAB, 0xCD, 0xEF};
  std::string jsonPayload = "{\"temperature\":22.5,\"humidity\":45}";

  CoapPacket::CoapError err3 = builder3.setType(CoapPacket::CoapType::CON)
                                   .setCode(CoapPacket::CoapCode::POST)
                                   .setMessageId(5678)
                                   .setToken(token3, 3)
                                   .setUriPath("/sensors/data")
                                   .setContentFormat(CoapPacket::CoapContentFormat::JSON)
                                   .setPayload(jsonPayload)
                                   .buildBuffer(buffer3);

  if (err3 == CoapPacket::CoapError::OK) {
    std::cout << "✓ Build successful!" << std::endl;
    std::cout << "Buffer size: " << buffer3.size() << " bytes" << std::endl;
    std::cout << "Hex dump:" << std::endl;
    printHex(buffer3);

    // Parse it back
    CoapPacket::CoapPacket packet3;
    if (CoapPacket::CoapParser::parse(buffer3, packet3) == CoapPacket::CoapError::OK) {
      std::cout << "✓ Parse successful!" << std::endl;
      printPacket(packet3);
    }
  } else {
    std::cout << "✗ Build failed: " << CoapPacket::getErrorMessage(err3) << std::endl;
  }

  // Example 4: Build a response (2.05 Content)
  std::cout << "Example 4: Building a 2.05 Content response" << std::endl;
  std::cout << "-------------------------------------------" << std::endl;

  CoapPacket::CoapBuilder builder4;
  std::vector<uint8_t> buffer4;

  uint8_t token4[] = {0x12, 0x34}; // Echo token from request
  std::string responseData = "25.3";

  CoapPacket::CoapError err4 = builder4.setType(CoapPacket::CoapType::ACK)
                                   .setCode(CoapPacket::CoapCode::CONTENT_2_05)
                                   .setMessageId(1234) // Same as request
                                   .setToken(token4, 2)
                                   .setContentFormat(CoapPacket::CoapContentFormat::TEXT_PLAIN)
                                   .setPayload(responseData)
                                   .buildBuffer(buffer4);

  if (err4 == CoapPacket::CoapError::OK) {
    std::cout << "✓ Build successful!" << std::endl;
    std::cout << "Buffer size: " << buffer4.size() << " bytes" << std::endl;
    std::cout << "Hex dump:" << std::endl;
    printHex(buffer4);

    // Parse it back
    CoapPacket::CoapPacket packet4;
    if (CoapPacket::CoapParser::parse(buffer4, packet4) == CoapPacket::CoapError::OK) {
      std::cout << "✓ Parse successful!" << std::endl;
      printPacket(packet4);
    }
  } else {
    std::cout << "✗ Build failed: " << CoapPacket::getErrorMessage(err4) << std::endl;
  }

  // Example 5: Build request with URI query parameters
  std::cout << "Example 5: Building GET with query parameters" << std::endl;
  std::cout << "----------------------------------------------" << std::endl;

  CoapPacket::CoapBuilder builder5;
  std::vector<uint8_t> buffer5;

  uint8_t token5[] = {0xFF};

  CoapPacket::CoapError err5 = builder5.setType(CoapPacket::CoapType::CON)
                                   .setCode(CoapPacket::CoapCode::GET)
                                   .setMessageId(9999)
                                   .setToken(token5, 1)
                                   .setUriPath("/api/data")
                                   .addUriQuery("start", "0")
                                   .addUriQuery("limit", "10")
                                   .buildBuffer(buffer5);

  if (err5 == CoapPacket::CoapError::OK) {
    std::cout << "✓ Build successful!" << std::endl;
    std::cout << "Buffer size: " << buffer5.size() << " bytes" << std::endl;
    std::cout << "Hex dump:" << std::endl;
    printHex(buffer5);

    // Parse it back
    CoapPacket::CoapPacket packet5;
    if (CoapPacket::CoapParser::parse(buffer5, packet5) == CoapPacket::CoapError::OK) {
      std::cout << "✓ Parse successful!" << std::endl;
      printPacket(packet5);
    }
  } else {
    std::cout << "✗ Build failed: " << CoapPacket::getErrorMessage(err5) << std::endl;
  }

  // Example 6: Error handling - invalid packet
  std::cout << "Example 6: Error handling - parsing invalid packet" << std::endl;
  std::cout << "---------------------------------------------------" << std::endl;

  std::vector<uint8_t> invalidBuffer = {0x40, 0x01}; // Too short
  CoapPacket::CoapPacket packetInvalid;
  CoapPacket::CoapError errInvalid = CoapPacket::CoapParser::parse(invalidBuffer, packetInvalid);

  if (errInvalid != CoapPacket::CoapError::OK) {
    std::cout << "✓ Correctly detected error: " << CoapPacket::getErrorMessage(errInvalid)
              << std::endl;
  } else {
    std::cout << "✗ Should have failed but didn't!" << std::endl;
  }
  std::cout << std::endl;

  std::cout << "=== All examples completed ===" << std::endl;

  return 0;
}
