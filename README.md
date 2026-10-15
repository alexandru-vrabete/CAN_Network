# ESP32 CAN (TWAI) Network

A small two-node CAN bus built on ESP32 and SN65HVD230 transceivers, using the
native ESP-IDF TWAI driver (`driver/twai.h`). One node transmits a simulated
sensor value every 500 ms, the other receives it and drives an LED when the
value crosses a threshold. A third, optional listen-only node dumps every frame
over serial so a Python script can log the bus traffic to CSV.

## Nodes

| Node | Folder            | Role                                                    |
|------|-------------------|---------------------------------------------------------|
| A    | `node_a_tx/`      | Periodic transmitter (ID `0x123`, DLC 2, every 500 ms)  |
| B    | `node_b_rx/`      | Receiver + actuator (hardware ID filter, LED on GPIO 2) |
| C    | `node_c_sniffer/` | Listen-only sniffer, prints frames over UART            |

- **Baud rate:** 500 kbps (`TWAI_TIMING_CONFIG_500KBITS()`)
- **Frame:** standard 11-bit ID `0x123`, 2 data bytes
  - `data[0]` = simulated sensor value, counts `0..100` and wraps
  - `data[1]` = `0xA5` fixed marker byte
- **Actuator rule (Node B):** `sensor > 50` → LED on, otherwise off

## Hardware

- 2x ESP32 dev board (3x if you also build the sniffer node)
- 2x SN65HVD230 CAN transceiver (3.3 V)
- 2x 120 Ω resistor (bus termination)
- Twisted pair for CAN_H / CAN_L

### CAN pins (same on every node)

| Signal      | ESP32 GPIO |
|-------------|------------|
| CTX (CAN TX)| GPIO 5     |
| CRX (CAN RX)| GPIO 4     |
| LED (Node B)| GPIO 2     |

### ESP32 ↔ SN65HVD230 wiring

```
   ESP32                         SN65HVD230
 ---------                      ------------
 GPIO 5 (CTX) ----------------> D   (TXD)
 GPIO 4 (CRX) <---------------- R   (RXD)
 3V3          ----------------> VCC (3.3V)
 GND          ----------------> GND
                                RS  --> tie to GND for high-speed mode
                                CANH --> bus line CAN_H
                                CANL --> bus line CAN_L
```

### Bus and termination

A CAN bus is a single twisted pair shared by all nodes, with a 120 Ω resistor
across CAN_H and CAN_L at **each physical end** of the pair.

```
        120 Ω                                        120 Ω
   CAN_H --+-------------------------------------------+-- CAN_H
           |                   |                       |
         Node A              Node B              Node C (sniffer)
           |                   |                       |
   CAN_L --+-------------------------------------------+-- CAN_L
        120 Ω                                        120 Ω
```

Notes:
- The two 120 Ω resistors go on the end nodes only. Nodes tapped in the middle
  (such as the sniffer) do not get their own terminator.
- Keep all grounds common: tie the GND of every ESP32 together.
- The SN65HVD230 is a 3.3 V part — power it from the ESP32 `3V3` pin, not 5 V.

## Build and flash

Set up ESP-IDF first (v4.4 or newer). Each node is a standalone project, so
build and flash them one at a time from their own folder.

```bash
# Node A - transmitter
cd node_a_tx
idf.py set-target esp32
idf.py build flash monitor

# Node B - receiver + actuator
cd ../node_b_rx
idf.py set-target esp32
idf.py build flash monitor

# Node C - listen-only sniffer (optional)
cd ../node_c_sniffer
idf.py set-target esp32
idf.py build flash monitor
```

Use `-p <port>` with `flash`/`monitor` if the port is not auto-detected
(for example `idf.py -p COM5 flash monitor` or `-p /dev/ttyUSB0`).

## Testing

1. Wire up Node A and Node B as shown above, with a 120 Ω terminator at each
   end of the bus.
2. Flash both nodes and open a monitor on each.
   - Node A prints a line on every successful transmission:
     `Sent ID=0x123 DLC=2 sensor=37`
   - Node B prints each accepted frame and the resulting actuator state:
     `Received ID=0x123 DLC=2 sensor=37` / `actuator OFF`
3. Watch Node B's LED (GPIO 2): it turns on while the incoming value is above
   50 and off below, so it blinks in step with Node A's counter sweep.

## Sniffing the bus

Flash Node C (`node_c_sniffer`) and tap it onto the bus. It prints one line per
frame over UART:

```
ID: 0x123, DLC: 2, DATA: [0x12, 0xA5]
```

Run the host script to decode those lines and log them to `can_traffic.csv`:

```bash
cd tools
pip install pyserial
python can_sniffer.py --port COM5          # Windows
python can_sniffer.py --port /dev/ttyUSB0  # Linux / macOS
```

The CSV columns are `timestamp, id, dlc, data`, for example:

```
timestamp,id,dlc,data
2026-01-20T14:05:11.482913,0x123,2,25 A5
2026-01-20T14:05:11.982547,0x123,2,26 A5
```
