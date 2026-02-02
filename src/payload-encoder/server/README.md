# AirGradient Cellular Payload Decoder (JavaScript)

JavaScript/Node.js decoder for the AirGradient binary cellular payload format (RFC v0).

## Installation

```bash
npm install
```

## Quick Start

```js
const { decodePayload } = require('./src/payload_decoder');

// Metadata:
// - bits 0-4: version (0)
// - bit 5: shared presence mask
const META_SHARED = 0x20;

// Shared mask payload: Temp + CO2
const payloadBuffer = Buffer.from([
  META_SHARED, 0x05,              // header: metadata, interval
  0x05, 0x00, 0x00, 0x00,         // mask lo (0x00000005)
  0x00, 0x00, 0x00, 0x00,         // mask hi
  0xC4, 0x09,                     // temp = 2500 => 25.00C
  0x90, 0x01                      // co2 = 400
]);

const decoded = decodePayload(payloadBuffer);
console.log(decoded);
```

## Format Notes

- Header is always 2 bytes: `[metadata][interval_minutes]`.
- Presence mask is 64-bit (8 bytes, little-endian) on the wire.
- `metadata`:
  - bits 0-4: `version`
  - bit 5: `sharedPresenceMask`
  - bits 6-7: reserved

### Shared Presence Mask

If `sharedPresenceMask` is set, the payload contains one 8-byte presence mask after the 2-byte header, and then sensor data for N readings back-to-back.

The decoder validates:

- shared mask has at least one field
- remaining payload length is a multiple of the per-reading sensor data size

## Testing

```bash
npm test
```

Run examples:

```bash
npm run example
```

## Files

- `src/payload_types.js` - Flag table, field names, scaling
- `src/payload_decoder.js` - Decoder
- `src/test_decoder.js` - Tests
- `src/example_usage.js` - Examples

## License

MIT
