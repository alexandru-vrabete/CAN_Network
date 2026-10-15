#!/usr/bin/env python3
"""Serial CAN sniffer.

Reads frames printed by the listen-only ESP32 node (node_c_sniffer) over the
serial port, parses them and appends them to a CSV file.

Expected input line format:

    ID: 0x123, DLC: 2, DATA: [0x12, 0x34]

Usage:

    python can_sniffer.py --port COM5
    python can_sniffer.py --port /dev/ttyUSB0 --output can_traffic.csv
"""

import argparse
import csv
import re
import sys
from datetime import datetime

import serial


LINE_PATTERN = re.compile(
    r"ID:\s*0x([0-9A-Fa-f]+),\s*DLC:\s*(\d+),\s*DATA:\s*\[([^\]]*)\]"
)


def parse_line(line):
    """Return (id, dlc, [bytes]) for a valid frame line, or None."""
    match = LINE_PATTERN.search(line)
    if not match:
        return None

    frame_id = int(match.group(1), 16)
    dlc = int(match.group(2))

    data_part = match.group(3).strip()
    data_bytes = []
    if data_part:
        for token in data_part.split(","):
            data_bytes.append(int(token.strip(), 16))

    return frame_id, dlc, data_bytes


def main():
    parser = argparse.ArgumentParser(
        description="Read CAN frames from the serial port and log them to CSV."
    )
    parser.add_argument("--port", required=True,
                        help="Serial port, e.g. COM5 or /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200,
                        help="Serial baud rate (default 115200)")
    parser.add_argument("--output", default="can_traffic.csv",
                        help="Output CSV file (default can_traffic.csv)")
    args = parser.parse_args()

    print("Opening %s at %d baud..." % (args.port, args.baud))
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as error:
        print("Could not open serial port: %s" % error)
        sys.exit(1)

    with open(args.output, "a", newline="") as csv_file:
        writer = csv.writer(csv_file)
        if csv_file.tell() == 0:
            writer.writerow(["timestamp", "id", "dlc", "data"])

        print("Logging to %s. Press Ctrl+C to stop." % args.output)
        try:
            while True:
                raw = ser.readline().decode("utf-8", errors="ignore").strip()
                if not raw:
                    continue

                parsed = parse_line(raw)
                if parsed is None:
                    continue

                frame_id, dlc, data_bytes = parsed
                timestamp = datetime.now().isoformat()
                data_hex = " ".join("%02X" % b for b in data_bytes)

                writer.writerow([timestamp, "0x%03X" % frame_id, dlc, data_hex])
                csv_file.flush()
                print("%s  ID=0x%03X  DLC=%d  DATA=[%s]"
                      % (timestamp, frame_id, dlc, data_hex))
        except KeyboardInterrupt:
            print("\nStopped.")
        finally:
            ser.close()


if __name__ == "__main__":
    main()
