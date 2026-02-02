/**
 * Example Usage - AirGradient Payload Decoder (RFC v0)
 * Run with: node src/example_usage.js
 */

const { decodePayload, decodePayloadToJSON } = require("./payload_decoder");

function mask64LE(lo, hi = 0) {
  return Buffer.from([
    lo & 0xff,
    (lo >>> 8) & 0xff,
    (lo >>> 16) & 0xff,
    (lo >>> 24) & 0xff,
    hi & 0xff,
    (hi >>> 8) & 0xff,
    (hi >>> 16) & 0xff,
    (hi >>> 24) & 0xff,
  ]);
}

// Metadata:
// - bits 0-4: version (0)
// - bit 5: shared presence mask
const META_SHARED = 0x20;

console.log("=== Example 1: Shared Mask (Temp + CO2) ===");
{
  const payload = Buffer.concat([
    Buffer.from([META_SHARED, 0x05]),
    mask64LE(0x00000005),
    Buffer.from([
      0xc4,
      0x09, // temp = 2500 => 25.00C
      0x90,
      0x01, // co2 = 400
    ]),
  ]);

  console.log(decodePayloadToJSON(payload, true));
}

console.log("\n=== Example 2: Shared Mask Batch (3 CO2 readings) ===");
{
  const payload = Buffer.concat([
    Buffer.from([META_SHARED, 0x05]),
    mask64LE(0x00000004),
    Buffer.from([0x90, 0x01, 0x9a, 0x01, 0xa4, 0x01]),
  ]);

  const decoded = decodePayload(payload);
  console.log(`Reading count: ${decoded.readingCount}`);
  console.log(
    "CO2 values:",
    decoded.readings.map((r) => r.co2),
  );
}

console.log("\n=== Example 3: Two-Channel PM2.5 (CH1 + CH2) ===");
{
  // bits 8 and 9
  const payload = Buffer.concat([
    Buffer.from([META_SHARED, 0x05]),
    mask64LE(0x00000300),
    Buffer.from([
      0x7d,
      0x00, // pm25_ch1 = 125 => 12.5
      0x87,
      0x00, // pm25_ch2 = 135 => 13.5
    ]),
  ]);

  const decoded = decodePayload(payload);
  console.log(decoded.readings[0]);
}

console.log("\n=== Example 4: Per reading masks==");
{
  const payload = Buffer.concat([
    Buffer.from([0x00, 0x05]),
    mask64LE(0x0000000000000001), // Temp
    Buffer.from([0xc4, 0x09]),
    mask64LE(0x0000000000000004), // CO2
    Buffer.from([0x90, 0x01]),
  ]);

  const decoded = decodePayload(payload);
  console.log(decoded);
}
