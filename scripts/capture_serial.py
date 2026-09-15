#!/usr/bin/env python3
"""Capture the USB serial output of the zte-pico-tool Pico firmware.

The script intentionally stores the serial stream without interpreting it.
This gives the project a reproducible record that can be analyzed later.

Typical use:
    python scripts/capture_serial.py COM3 --command m
    python scripts/capture_serial.py COM3 --command c --duration 30

Requirements:
    pip install pyserial
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import sys
import time
from pathlib import Path

try:
    import serial
except ImportError:
    print("pyserial is not installed. Run: python -m pip install pyserial", file=sys.stderr)
    raise SystemExit(2)


BAUD = 115200


def session_name() -> str:
    return dt.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")


def main() -> int:
    parser = argparse.ArgumentParser(description="Save Pico USB serial captures for zte-pico-tool.")
    parser.add_argument("port", help="Serial port, for example COM3")
    parser.add_argument("--command", choices=("m", "c"), default="m",
                        help="Pico capture command: m=5 seconds, c=continuous")
    parser.add_argument("--duration", type=float, default=None,
                        help="Seconds to record. Recommended with --command c.")
    parser.add_argument("--output", default="captures",
                        help="Root directory for saved captures (default: captures)")
    parser.add_argument("--settle", type=float, default=1.0,
                        help="Seconds to wait after opening the port before sending command")
    args = parser.parse_args()

    root = Path(args.output)
    session = root / session_name()
    session.mkdir(parents=True, exist_ok=False)

    raw_path = session / "serial_raw.bin"
    text_path = session / "serial.txt"
    metadata_path = session / "metadata.json"

    metadata = {
        "started_at": dt.datetime.now(dt.timezone.utc).isoformat(),
        "serial_port": args.port,
        "usb_serial_baud_setting": BAUD,
        "command": args.command,
        "duration_seconds": args.duration,
        "tool": "zte-pico-tool/scripts/capture_serial.py",
        "notes": "Raw USB CDC stream from Pico. No protocol decoding performed.",
    }

    print(f"Opening {args.port} at {BAUD}...")
    try:
        with serial.Serial(args.port, BAUD, timeout=0.1) as ser:
            time.sleep(args.settle)
            ser.reset_input_buffer()

            print(f"Sending command '{args.command}' to Pico.")
            ser.write(args.command.encode("ascii"))
            ser.flush()

            start = time.monotonic()
            data = bytearray()

            if args.command == "m" and args.duration is None:
                # v0.2.0 performs a fixed 5-second capture for 'm'.
                deadline = start + 7.0
            elif args.duration is not None:
                deadline = start + args.duration
            else:
                deadline = None

            print("Capturing. Press Ctrl+C to abort safely.")
            while deadline is None or time.monotonic() < deadline:
                chunk = ser.read(4096)
                if chunk:
                    data.extend(chunk)
                    sys.stdout.buffer.write(chunk)
                    sys.stdout.buffer.flush()

            if args.command == "c" and args.duration is not None:
                # Stop continuous mode using a command byte after the requested window.
                ser.write(b"x")
                ser.flush()
                end_deadline = time.monotonic() + 2.0
                while time.monotonic() < end_deadline:
                    chunk = ser.read(4096)
                    if chunk:
                        data.extend(chunk)
                        sys.stdout.buffer.write(chunk)
                        sys.stdout.buffer.flush()

    except KeyboardInterrupt:
        print("\nCapture interrupted by user.")
        metadata["interrupted"] = True
    except serial.SerialException as exc:
        print(f"Serial error: {exc}", file=sys.stderr)
        return 1

    raw_path.write_bytes(data)
    text_path.write_bytes(data)
    metadata["finished_at"] = dt.datetime.now(dt.timezone.utc).isoformat()
    metadata["bytes_captured"] = len(data)
    metadata_path.write_text(json.dumps(metadata, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    print(f"\nSaved capture: {session}")
    print(f"  Raw:      {raw_path}")
    print(f"  Text:     {text_path}")
    print(f"  Metadata: {metadata_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
