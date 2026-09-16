#!/usr/bin/env python3
"""Store a framed passive UART stream from zte-pico-tool on the host."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

try:
    import serial
except ImportError as exc:
    raise SystemExit("pyserial is required: python -m pip install pyserial") from exc

MAGIC = b"ZTE1"
HEADER_SIZE = 7  # magic(4) + channel(1) + length(2)


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
        description="Capture framed passive UART bytes from the zte-pico-tool Pico."
    )
    parser.add_argument("port", help="Serial port, e.g. COM5 or /dev/ttyACM0")
    parser.add_argument("--output", type=Path, required=True, help="Capture directory")
    parser.add_argument("--baud", type=int, default=115200, help="USB CDC baud setting (default: 115200)")
    parser.add_argument("--device", default="ZTE H3601P", help="Target device label")
    parser.add_argument("--hardware-revision", default="unknown")
    parser.add_argument("--firmware", default="zte-pico-tool uart_capture v0.4.0")
    parser.add_argument("--timeout", type=float, default=0.2)
    return parser


def find_magic(buffer: bytearray) -> bool:
    position = buffer.find(MAGIC)
    if position < 0:
        keep = min(len(buffer), len(MAGIC) - 1)
        if keep:
            del buffer[:-keep]
        else:
            buffer.clear()
        return False
    if position:
        del buffer[:position]
    return True


def main() -> int:
    args = build_parser().parse_args()
    output_dir = args.output
    output_dir.mkdir(parents=True, exist_ok=True)

    gp2_path = output_dir / "gp2.raw"
    gp3_path = output_dir / "gp3.raw"
    metadata_path = output_dir / "metadata.json"
    sums_path = output_dir / "SHA256SUMS"

    started_at = utc_now()
    bytes_by_channel = {2: 0, 3: 0}
    frames = 0
    interrupted = False
    disconnected = False
    protocol_error = False
    buffer = bytearray()

    print(f"Opening {args.port}...")
    print(f"Capture directory: {output_dir}")
    print("Starting Pico continuous stream with command 'c'.")
    print("Press Ctrl-C to stop and finalize the capture.")

    try:
        with serial.Serial(args.port, args.baud, timeout=args.timeout) as port, \
                gp2_path.open("wb") as gp2, gp3_path.open("wb") as gp3:
            port.reset_input_buffer()
            port.write(b"c")
            port.flush()

            started_stream = False

            while True:
                try:
                    chunk = port.read(4096)
                except (serial.SerialException, OSError) as exc:
                    disconnected = True
                    print(f"Serial connection lost: {exc}", file=sys.stderr)
                    break

                if not chunk:
                    continue

                buffer.extend(chunk)

                if not started_stream:
                    marker = b"ZTE-CAPTURE-V1\r\n"
                    marker_pos = buffer.find(marker)
                    if marker_pos < 0:
                        if len(buffer) > 256:
                            del buffer[:-32]
                        continue
                    del buffer[:marker_pos + len(marker)]
                    started_stream = True
                    print("Capture stream started.")

                while len(buffer) >= HEADER_SIZE:
                    if not find_magic(buffer):
                        break
                    if len(buffer) < HEADER_SIZE:
                        break

                    channel = buffer[4]
                    length = buffer[5] | (buffer[6] << 8)
                    if channel not in (2, 3) or length > 256:
                        del buffer[0]
                        protocol_error = True
                        continue
                    if len(buffer) < HEADER_SIZE + length:
                        break

                    payload = bytes(buffer[HEADER_SIZE:HEADER_SIZE + length])
                    del buffer[:HEADER_SIZE + length]

                    if channel == 2:
                        gp2.write(payload)
                        gp2.flush()
                    else:
                        gp3.write(payload)
                        gp3.flush()

                    bytes_by_channel[channel] += length
                    frames += 1
                    total = bytes_by_channel[2] + bytes_by_channel[3]
                    print(
                        f"\r[capture] frames={frames} GP2={bytes_by_channel[2]} "
                        f"GP3={bytes_by_channel[3]} total={total}",
                        end="",
                        flush=True,
                    )

    except KeyboardInterrupt:
        interrupted = True
        print("\nStopping capture...")
    except serial.SerialException as exc:
        print(f"Unable to open/read {args.port}: {exc}", file=sys.stderr)
        return 2

    finished_at = utc_now()
    hashes = {
        "gp2.raw": sha256_file(gp2_path) if gp2_path.exists() else hashlib.sha256(b"").hexdigest(),
        "gp3.raw": sha256_file(gp3_path) if gp3_path.exists() else hashlib.sha256(b"").hexdigest(),
    }

    metadata = {
        "schema_version": 1,
        "device": args.device,
        "hardware_revision": args.hardware_revision,
        "firmware": args.firmware,
        "transport": "usb-cdc",
        "capture_interface": "uart",
        "baud": 115200,
        "data_bits": 8,
        "parity": "N",
        "stop_bits": 1,
        "mode": "passive-continuous-stream",
        "protocol": "ZTE-CAPTURE-V1",
        "started_at_utc": started_at,
        "finished_at_utc": finished_at,
        "frames_received": frames,
        "bytes_received": bytes_by_channel,
        "bytes_dropped_by_host": 0,
        "protocol_error": protocol_error,
        "stopped_by_keyboard_interrupt": interrupted,
        "usb_disconnect_detected": disconnected,
        "files": {
            "gp2": "gp2.raw",
            "gp3": "gp3.raw",
            "sha256": "SHA256SUMS"
        },
        "sha256": hashes,
    }

    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    sums_path.write_text(
        f"{hashes['gp2.raw']}  gp2.raw\n{hashes['gp3.raw']}  gp3.raw\n",
        encoding="utf-8",
    )

    print(f"\nCapture finalized: GP2={bytes_by_channel[2]} bytes, GP3={bytes_by_channel[3]} bytes")
    print(f"Metadata: {metadata_path}")
    print(f"Checksums: {sums_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
