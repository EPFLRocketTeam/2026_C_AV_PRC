#!/usr/bin/env python3
"""
fdc1004_logger.py - Read FDC1004 4-channel capacitance readings streamed
over serial (USB CDC) from the STM32 firmware and log them to a CSV file.

Firmware side prints one line per sample, tagged so it can be picked out
from anything else sharing the same serial output:

    FDC1004,<tick_ms>,<ch0_pF>,<ch1_pF>,<ch2_pF>,<ch3_pF>

Usage:
    pip install pyserial
    python fdc1004_logger.py --port /dev/tty.usbmodemXXXX --output capture.csv
    python fdc1004_logger.py            # lists available ports if --port is omitted/invalid

Press Ctrl+C to stop; the CSV file is flushed after every row, so no data
is lost if the script is interrupted.
"""

import argparse
import csv
import datetime
import sys

import serial
import serial.tools.list_ports

TAG = "FDC1004"
EXPECTED_FIELDS = 6  # TAG, tick_ms, ch0, ch1, ch2, ch3


def list_ports():
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return
    print("Available serial ports:")
    for p in ports:
        print(f"  {p.device}  ({p.description})")


def parse_line(line: str):
    """Return (tick_ms, ch0, ch1, ch2, ch3) or None if the line isn't a data row."""
    fields = line.strip().split(",")
    if len(fields) != EXPECTED_FIELDS or fields[0] != TAG:
        return None
    try:
        tick_ms = int(fields[1])
        channels = [float(x) for x in fields[2:6]]
    except ValueError:
        return None
    return (tick_ms, *channels)


def main():
    parser = argparse.ArgumentParser(description="Log FDC1004 capacitance stream to CSV.")
    parser.add_argument("--port", help="Serial port, e.g. /dev/tty.usbmodemXXXX or COM5")
    parser.add_argument("--baud", type=int, default=115200,
                         help="Baud rate (ignored by USB CDC, kept for compatibility)")
    parser.add_argument("--output", default=None,
                         help="Output CSV path (default: fdc1004_<timestamp>.csv)")
    args = parser.parse_args()

    if not args.port:
        list_ports()
        print("\nRe-run with --port <device> to start logging.")
        sys.exit(1)

    output_path = args.output or datetime.datetime.now().strftime("fdc1004_%Y%m%d_%H%M%S.csv")

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"Could not open {args.port}: {e}")
        list_ports()
        sys.exit(1)

    print(f"Logging {args.port} -> {output_path}  (Ctrl+C to stop)")

    row_count = 0
    with open(output_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["host_time_iso", "device_tick_ms", "ch0_pF", "ch1_pF", "ch2_pF", "ch3_pF"])
        f.flush()

        try:
            while True:
                raw = ser.readline()
                if not raw:
                    continue  # read timeout, no data yet
                try:
                    line = raw.decode("utf-8", errors="replace")
                except UnicodeDecodeError:
                    continue

                parsed = parse_line(line)
                if parsed is None:
                    continue  # not an FDC1004 data line (other firmware logs, noise, etc.)

                tick_ms, ch0, ch1, ch2, ch3 = parsed
                host_time = datetime.datetime.now().isoformat(timespec="milliseconds")
                writer.writerow([host_time, tick_ms, ch0, ch1, ch2, ch3])
                f.flush()

                row_count += 1
                print(f"[{host_time}] tick={tick_ms:>8}  "
                      f"ch0={ch0:8.4f} pF  ch1={ch1:8.4f} pF  "
                      f"ch2={ch2:8.4f} pF  ch3={ch3:8.4f} pF")

        except KeyboardInterrupt:
            print(f"\nStopped. {row_count} samples written to {output_path}")
        finally:
            ser.close()


if __name__ == "__main__":
    main()
