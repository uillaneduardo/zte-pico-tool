import argparse
import json
import hashlib
import serial
import time
from pathlib import Path

TRIGGER = b"Press 1 means entering boot mode"


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(65536), b""):
            h.update(block)
    return h.hexdigest()


def append_log(path: Path, data: bytes) -> None:
    with path.open("ab") as f:
        f.write(data)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Passive ZTE H3601P UART capture; never arms TX and never sends the bootloader '1'."
    )
    parser.add_argument("port", help="USB serial port, e.g. COM3")
    parser.add_argument("--output", required=True, help="Output session directory")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=0.1)
    args = parser.parse_args()

    out = Path(args.output)
    out.mkdir(parents=True, exist_ok=True)
    rx_path = out / "rx.raw"
    tx_path = out / "tx.raw"
    terminal_path = out / "terminal.log"
    metadata_path = out / "session.json"

    started = time.time()
    trigger_seen = False
    rx_total = 0
    rolling = bytearray()

    metadata = {
        "schema_version": 1,
        "device": "ZTE H3601P",
        "firmware": "zte-pico-tool uart_console v0.1.0",
        "transport": "usb-cdc",
        "capture_interface": "uart",
        "baud": args.baud,
        "data_bits": 8,
        "parity": "N",
        "stop_bits": 1,
        "mode": "passive-read-only",
        "rx_mapping": "ZTE pad 2 -> Pico GP2",
        "tx_mapping": "Pico GP3 -> ZTE pad 3",
        "tx_policy": "disabled",
        "bootloader_trigger": "Press 1 means entering boot mode",
        "automatic_response": None,
        "started_at_unix": started,
        "finished_at_unix": None,
        "trigger_seen": False,
        "trigger_detected_at_unix": None,
        "response_sent": False,
        "rx_bytes": 0,
        "tx_bytes": 0,
        "stopped_by_keyboard_interrupt": False,
        "serial_disconnect_detected": False,
    }

    with serial.Serial(args.port, args.baud, timeout=args.timeout) as ser:
        # Deliberately do NOT send the firmware 'a' command. TX remains
        # disarmed, so this session cannot transmit the bootloader trigger
        # response or any other payload.
        print("Passive UART capture started.")
        print("TX remains disarmed; no bootloader input will be sent.")
        print("Power-cycle or factory-reset the ZTE now. Ctrl-C stops the session.")

        try:
            while True:
                data = ser.read(256)
                if not data:
                    continue

                rx_total += len(data)
                append_log(rx_path, data)
                append_log(terminal_path, data)

                search_buffer = rolling + data
                if not trigger_seen and TRIGGER in search_buffer:
                    trigger_seen = True
                    metadata["trigger_seen"] = True
                    metadata["trigger_detected_at_unix"] = time.time()
                    print("\n[BOOTLOADER] interaction prompt detected; no response will be sent.")

                keep = max(0, len(TRIGGER) - 1)
                rolling = bytearray(search_buffer[-keep:]) if keep else bytearray()

                print(data.decode("utf-8", errors="replace"), end="", flush=True)

        except KeyboardInterrupt:
            metadata["stopped_by_keyboard_interrupt"] = True
            print("\nStopping passive capture...")
        except serial.SerialException:
            metadata["serial_disconnect_detected"] = True
            print("\nUSB serial connection lost.")

    metadata["finished_at_unix"] = time.time()
    metadata["rx_bytes"] = rx_total
    metadata["tx_bytes"] = 0

    hashes = {}
    for path in (rx_path, terminal_path):
        if path.exists():
            hashes[path.name] = sha256_file(path)
    metadata["sha256"] = hashes

    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    with (out / "SHA256SUMS").open("w", encoding="utf-8") as f:
        for name, digest in hashes.items():
            f.write(f"{digest}  {name}\n")

    print("\nSession finalized:")
    print(f"  RX bytes: {rx_total}")
    print("  TX bytes: 0")
    print(f"  Prompt detected: {trigger_seen}")
    print("  '1' sent: False")
    print(f"  Output: {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
