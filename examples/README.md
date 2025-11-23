# Examples

This directory contains complete GNU Radio Companion flowgraphs with sample binary acquisition files for testing both 10BASE-T and 100BASE-TX decoding.

## Files

### 100BASE-TX (Fast Ethernet)

**Flowgraph:** `decode_100BASETX.grc`

**Sample files:**
- `output.bin` - Sample rate: 625 MS/s, complex float32
- `output2.bin` - Sample rate: 625 MS/s, complex float32
- `RefCurve_2025-04-10_1_132807.Wfm.bin` - Sample rate: 500 MS/s, complex float32

### 10BASE-T (Ethernet)

**Flowgraph:** `decode_10BASET.grc`

**Sample file:**
- `oscillo_10Mbps.bin` - Sample rate: 1.25 GS/s, complex float32

## Usage

1. Open the desired flowgraph in GNU Radio Companion:
```bash
   gnuradio-companion examples/decode_100BASETX.grc
```

2. The file source is already configured to use the provided sample files

3. Adjust the Throttle block if needed to control playback speed:
   - Higher values = faster processing
   - Lower values (500k-1M) = slower, easier to observe frames

4. Run the flowgraph

5. Observe decoded frames in:
   - Console output (frame details printed automatically)
   - Web inspector (if running `apps/ethernet_inspector.py`)

## Signal Acquisition

The provided binary files contain data (float32) captured from oscilloscopes monitoring Ethernet twisted pairs. The signals were captured using differential probes on twisted pair cables


All captures follow the methodology described in Hackable Magazine #61 (jui. aout 2025).

## File Format

All `.bin` files are raw binary files containing:
- Type: float32
- Byte order: Little-endian
- No header, just raw IQ data

`

## Notes

- The Multiply Const block in each flowgraph adjusts signal amplitude before slicing
- Typical values: 5-15 depending on your acquisition setup
- You may need to adjust this value for your own captures

- The Throttle block prevents GNU Radio from processing files too quickly
- Without throttle, entire files process in milliseconds
- Adjust to comfortable viewing speed (recommended: 500k-10M samples/s)
