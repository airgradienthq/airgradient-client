/**
 * Tests for AirGradient Payload Decoder
 * Run with: node src/test_decoder.js
 */

const assert = require("assert");
const { decodePayload } = require("./payload_decoder");

function mask64LE(lo, hi = 0) {
  return [
    lo & 0xff,
    (lo >>> 8) & 0xff,
    (lo >>> 16) & 0xff,
    (lo >>> 24) & 0xff,
    hi & 0xff,
    (hi >>> 8) & 0xff,
    (hi >>> 16) & 0xff,
    (hi >>> 24) & 0xff,
  ];
}

function approxEqual(a, b, eps = 1e-9) {
  return Math.abs(a - b) <= eps;
}

// Metadata byte layout:
// - bits 0-4: version (0)
// - bit 5: shared presence mask
const META_SHARED = 0x20;
const META_PER_READING = 0x00;

// Test 1: Shared mask, single reading (Temp + CO2)
{
  const buffer = Buffer.from([
    META_SHARED,
    0x05,
    ...mask64LE(0x00000005),
    0xc4,
    0x09,
    0x90,
    0x01,
  ]);

  const decoded = decodePayload(buffer);

  assert.strictEqual(decoded.header.version, 0);
  assert.strictEqual(decoded.header.sharedPresenceMask, true);
  assert.strictEqual(decoded.header.intervalMinutes, 5);
  assert.strictEqual(decoded.readingCount, 1);
  assert.strictEqual(decoded.readings[0].temperature, 25);
  assert.strictEqual(decoded.readings[0].co2, 400);
}

// Test 2: Shared mask, 3 readings (CO2 only)
{
  const buffer = Buffer.from([
    META_SHARED,
    0x05,
    ...mask64LE(0x00000004),
    0x90,
    0x01,
    0x9a,
    0x01,
    0xa4,
    0x01,
  ]);

  const decoded = decodePayload(buffer);

  assert.strictEqual(decoded.header.sharedPresenceMask, true);
  assert.strictEqual(decoded.readingCount, 3);
  assert.deepStrictEqual(
    decoded.readings.map((r) => r.co2),
    [400, 410, 420],
  );
}

// Test 3: Per-reading masks, 2 readings with different masks
{
  const buffer = Buffer.from([
    META_PER_READING,
    0x05,
    // Reading 1: temp
    ...mask64LE(0x00000001),
    0xc4,
    0x09,
    // Reading 2: co2
    ...mask64LE(0x00000004),
    0x90,
    0x01,
  ]);

  const decoded = decodePayload(buffer);

  assert.strictEqual(decoded.header.sharedPresenceMask, false);
  assert.strictEqual(decoded.readingCount, 2);
  assert.strictEqual(decoded.readings[0].temperature, 25);
  assert.strictEqual(decoded.readings[1].co2, 400);
}

// Test 4: Shared mask, PM2.5 two-channel flags
{
  const buffer = Buffer.from([
    META_SHARED,
    0x05,
    ...mask64LE(0x00000300),
    0x7d,
    0x00,
    0x87,
    0x00,
  ]);

  const decoded = decodePayload(buffer);
  assert.strictEqual(decoded.readingCount, 1);
  assert.strictEqual(decoded.readings[0].pm25_ch1, 12.5);
  assert.strictEqual(decoded.readings[0].pm25_ch2, 13.5);
}

// Test 5: Ordering (Temp, Hum, CO2) must follow ascending bit index
// Mask bits 0,1,2 set => values must come as temp, hum, co2.
{
  const buffer = Buffer.from([
    META_SHARED,
    0x05,
    ...mask64LE(0x00000007),
    // temp = 0x09C4 (2500 -> 25.00)
    0xc4,
    0x09,
    // hum = 0x1770 (6000 -> 60.00)
    0x70,
    0x17,
    // co2 = 0x0190 (400)
    0x90,
    0x01,
  ]);

  const decoded = decodePayload(buffer);
  assert.strictEqual(decoded.readings[0].temperature, 25);
  assert.strictEqual(decoded.readings[0].humidity, 60);
  assert.strictEqual(decoded.readings[0].co2, 400);
}

// Test 6: Shared mask, 32-bit field (O3_WE)
{
  const buffer = Buffer.from([
    META_SHARED,
    0x05,
    ...mask64LE(0x01000000),
    0x78,
    0x56,
    0x34,
    0x12,
  ]);

  const decoded = decodePayload(buffer);
  assert.strictEqual(decoded.readingCount, 1);
  assert.ok(approxEqual(decoded.readings[0].o3_we, 0x12345678 / 1000));
}

// Test 7: Shared mask, signal (int8)
{
  // bit 29 => 0x20000000
  const buffer = Buffer.from([
    META_SHARED,
    0x05,
    ...mask64LE(0x20000000),
    0xb5,
  ]);

  const decoded = decodePayload(buffer);
  assert.strictEqual(decoded.readings[0].signal, -75);
}

console.log("All tests passed");
