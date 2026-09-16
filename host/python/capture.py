#!/usr/bin/env python3
"""Stream a passive UART capture from zte-pico-tool to the host filesystem."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

try:
    import serial
except ImportError as exc:
    raise SystemExit("pyserial is required: python -m pip install pyserial") from exc


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Capture passive UART bytes from the zte-pico-tool Pico."
    )
    parser.add_argument("port", help="Serial port, e.g. COM5 or /dev/ttyACM0")
    parser.add_argument("--output", type=Path, required=True, help="Output .raw file")
    parser.add_argument("--baud", type=int, default=115200, help="USB CDC baud setting (default: 115200)")
    parser.add_argument("--source", default="GP2 / ZTE pad 2", help="Capture source label")
    parser.add_argument("--device", default="ZTE H3601P", help="Target device label")
    parser.add_argument("--hardware-revision", default="unknown")
    parser.add_argument("--firmware", default="zte-pico-tool uart_capture v0.3.0")
    parser.add_argument("--timeout", type=float, default=0.2, help="Serial read timeout in seconds")
    return parser


def main() -> int:
    args = build_parser().parse_args()
    output = args.output
    output.parent.mkdir(parents=True, exist_ok=True)

    if output.suffix.lower() != ".raw":
        print("warning: raw capture files normally use the .raw extension", file=sys.stderr)

    started_at = utc_now()
    bytes_received = 0
    interrupted = False
    disconnected = False

    print(f"Opening {args.port}...")
    print(f"Raw output: {output}")
    print("Press Ctrl-C to stop and finalize the capture.")
    print("The raw file contains bytes received from the Pico without text decoding.")

    try:
        with serial.Serial(args.port, args.baud, timeout=args.timeout) as port, output.open("wb") as raw:
            while True:
                try:
                    chunk = port.read(4096)
                except (serial.SerialException, OSError) as exc:
                    disconnected = True
                    print(f"Serial connection lost: {exc}", file=sys.stderr)
                    break

                if chunk:
                    raw.write(chunk)
                    raw.flush()
                    bytes_received += len(chunk)
                    print(f"\r[capture] bytes={bytes_received}", end="", flush=True)
    except KeyboardInterrupt:
        interrupted = True
        print("\nStopping capture...")
    except serial.SerialException as exc:
        print(f"Unable to open/read {args.port}: {exc}", file=sys.stderr)
        return 2

    finished_at = utc_now()
    digest = sha256_file(output) if output.exists() else hashlib.sha256(b"").hexdigest()

    metadata = {
        "schema_version": 1,
        "device": args.device,
        "hardware_revision": args.hardware_revision,
        "firmware": args.firmware,
        "transport": "usb-cdc",
        "capture_interface": "uart",
        "source": args.source,
        "baud": 115200,
        "data_bits": 8,
        "parity": "N",
        "stop_bits": 1,
        "mode": "passive",
        "started_at_utc": started_at,
        "finished_at_utc": finished_at,
        "bytes_received": bytes_received,
        "bytes_dropped_by_host": 0,
        "stopped_by_keyboard_interrupt": interrupted,
        "usb_disconnect_detected": disconnected,
        "raw_file": output.name,
        "sha256": digest,
    }

    metadata_path = output.with_suffix(".json")
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")

    sums_path = output.parent / "SHA256SUMS"
    sums_path.write_text(f"{digest}  {output.name}\n", encoding="utf-8")

    print(f"\nCapture finalized: {bytes_received} bytes")
    print(f"SHA-256: {digest}")
    print(f"Metadata: {metadata_path}")
    print(f"Checksums: {sums_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
