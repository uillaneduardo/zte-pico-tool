import argparse
import json
import hashlib
import serial
import time
from pathlib import Path

TRIGGER = b"Press 1 means entering boot mode"
PASSWORD_PROMPT = b"Please input bootmode password"
SUCCESS_MARKERS = (
    b"bootmode>",
    b"Boot mode",
)

# Small, explicitly controlled first-pass candidate set.
# Boot4128s! is documented for another H3601P/ZX279128S variant.
# The remaining candidates preserve the same BootNNNNs! structure.
PASSWORDS = [
    "Boot4128s!",
    "Boot0035s!",
    "Boot0038s!",
    "Boot1630s!",
    "Boot2791s!",
    "Boot2912s!",
    "Boot0128s!",
    "Boot0035S!",
    "Boot0038S!",
    "Boot1630S!",
    "Boot2791S!",
    "Boot2912S!",
]


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
        description="Interactive ZTE H3601P UART console with controlled bootmode password testing."
    )
    parser.add_argument("port", help="USB serial port, e.g. COM3")
    parser.add_argument("--output", required=True, help="Output session directory")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=0.1)
    parser.add_argument("--prompt-timeout", type=float, default=8.0,
                        help="Seconds to wait for the next bootmode password prompt")
    parser.add_argument("--delay", type=float, default=1.5,
                        help="Seconds to wait after each password submission")
    args = parser.parse_args()

    out = Path(args.output)
    out.mkdir(parents=True, exist_ok=True)
    rx_path = out / "rx.raw"
    tx_path = out / "tx.raw"
    terminal_path = out / "terminal.log"
    metadata_path = out / "session.json"

    started = time.time()
    trigger_seen = False
    response_sent = False
    password_prompt_seen = False
    rx_total = 0
    tx_total = 0
    rolling = bytearray()
    password_buffer = bytearray()
    post_password_buffer = bytearray()
    password_index = 0
    attempts = []
    waiting_for_prompt = False
    prompt_deadline = None
    stop_reason = None

    metadata = {
        "schema_version": 3,
        "device": "ZTE H3601P",
        "firmware": "zte-pico-tool uart_console v0.2.0",
        "transport": "usb-cdc",
        "capture_interface": "uart",
        "baud": args.baud,
        "data_bits": 8,
        "parity": "N",
        "stop_bits": 1,
        "mode": "interactive-controlled-password-test",
        "rx_mapping": "ZTE pad 2 -> Pico GP2",
        "tx_mapping": "Pico GP3 -> ZTE pad 3",
        "tx_policy": "armed-by-host",
        "bootloader_trigger": "Press 1 means entering boot mode",
        "password_prompt": "Please input bootmode password",
        "password_candidates": PASSWORDS,
        "prompt_timeout_seconds": args.prompt_timeout,
        "delay_seconds": args.delay,
        "started_at_unix": started,
        "finished_at_unix": None,
        "trigger_seen": False,
        "trigger_detected_at_unix": None,
        "response_sent": False,
        "response_sent_at_unix": None,
        "password_prompt_seen": False,
        "password_attempts": attempts,
        "stop_reason": None,
        "rx_bytes": 0,
        "tx_bytes": 0,
        "stopped_by_keyboard_interrupt": False,
        "serial_disconnect_detected": False,
    }

    with serial.Serial(args.port, args.baud, timeout=args.timeout) as ser:
        ser.write(b"a")
        ser.flush()

        print("Interactive console started.")
        print("TX armed by host.")
        print(f"Password candidates loaded: {len(PASSWORDS)}")
        print("Power-cycle the ZTE now. Ctrl-C stops the session.")

        try:
            while True:
                data = ser.read(256)

                if data:
                    rx_total += len(data)
                    append_log(rx_path, data)
                    append_log(terminal_path, data)
                    password_buffer.extend(data)

                    search_buffer = rolling + data

                    if not trigger_seen and TRIGGER in search_buffer:
                        trigger_seen = True
                        metadata["trigger_seen"] = True
                        metadata["trigger_detected_at_unix"] = time.time()
                        print("\n[BOOTLOADER] interaction prompt detected.")
                        print("[TX] sending ASCII '1'.")
                        ser.write(b"1")
                        ser.flush()
                        append_log(tx_path, b"1")
                        append_log(terminal_path, b"\n[HOST TX] 1\n")
                        tx_total += 1
                        response_sent = True
                        metadata["response_sent"] = True
                        metadata["response_sent_at_unix"] = time.time()
                        post_password_buffer.clear()
                        print("[BOOTLOADER] response sent; waiting for password prompt.")

                    if PASSWORD_PROMPT in password_buffer:
                        password_prompt_seen = True
                        metadata["password_prompt_seen"] = True
                        password_buffer.clear()

                    # Only evaluate password-related success/failure output after
                    # a candidate has actually been submitted. This prevents the
                    # pre-password 'cspboot:' bootloader log from being treated as
                    # a successful bootmode transition.
                    if waiting_for_prompt:
                        post_password_buffer.extend(data)

                        lower = post_password_buffer.lower()
                        if any(marker.lower() in lower for marker in SUCCESS_MARKERS):
                            if attempts:
                                attempts[-1]["response_observed"] = "success-marker"
                            stop_reason = "success-marker-detected"
                            print("\n[BOOTMODE] possible success marker detected; stopping.")
                            break

                        if PASSWORD_PROMPT in post_password_buffer:
                            if attempts:
                                attempts[-1]["response_observed"] = "next-password-prompt"

                            post_password_buffer.clear()
                            password_buffer.clear()
                            waiting_for_prompt = False
                            prompt_deadline = None

                            if password_index >= len(PASSWORDS):
                                stop_reason = "password-list-exhausted"
                                print("\n[BOOTMODE] password list exhausted.")
                                break

                            candidate = PASSWORDS[password_index]
                            password_index += 1
                            attempt_no = password_index

                            print(f"\n[BOOTMODE] password prompt detected; attempt {attempt_no}/{len(PASSWORDS)}")
                            print(f"[TX] sending candidate: {candidate}")

                            payload = candidate.encode("ascii") + b"\r"
                            ser.write(payload)
                            ser.flush()
                            append_log(tx_path, payload)
                            append_log(terminal_path, b"\n[HOST TX] password attempt %d: %s\\r\n" % (attempt_no, candidate.encode("ascii")))
                            tx_total += len(payload)

                            attempt = {
                                "attempt": attempt_no,
                                "candidate": candidate,
                                "sent_at_unix": time.time(),
                                "response_observed": None,
                            }
                            attempts.append(attempt)

                            waiting_for_prompt = True
                            prompt_deadline = time.time() + args.prompt_timeout
                            post_password_buffer.clear()

                    # The first password prompt is handled separately so the
                    # candidate is sent only after the prompt is actually seen.
                    if password_prompt_seen and not attempts and not waiting_for_prompt:
                        if password_index >= len(PASSWORDS):
                            stop_reason = "password-list-exhausted"
                            print("\n[BOOTMODE] password list exhausted.")
                            break

                        candidate = PASSWORDS[password_index]
                        password_index += 1
                        attempt_no = password_index

                        print(f"\n[BOOTMODE] password prompt detected; attempt {attempt_no}/{len(PASSWORDS)}")
                        print(f"[TX] sending candidate: {candidate}")

                        payload = candidate.encode("ascii") + b"\r"
                        ser.write(payload)
                        ser.flush()
                        append_log(tx_path, payload)
                        append_log(terminal_path, b"\n[HOST TX] password attempt %d: %s\\r\n" % (attempt_no, candidate.encode("ascii")))
                        tx_total += len(payload)

                        attempt = {
                            "attempt": attempt_no,
                            "candidate": candidate,
                            "sent_at_unix": time.time(),
                            "response_observed": None,
                        }
                        attempts.append(attempt)

                        waiting_for_prompt = True
                        prompt_deadline = time.time() + args.prompt_timeout
                        post_password_buffer.clear()

                    keep = max(0, len(TRIGGER) - 1)
                    rolling = bytearray(search_buffer[-keep:]) if keep else bytearray()

                    try:
                        print(data.decode("utf-8", errors="replace"), end="", flush=True)
                    except Exception:
                        pass

                if waiting_for_prompt and prompt_deadline is not None and time.time() >= prompt_deadline:
                    if attempts:
                        attempts[-1]["response_observed"] = "no-next-password-prompt"
                    stop_reason = "no-next-password-prompt"
                    print("\n[BOOTMODE] no next password prompt observed within timeout; stopping.")
                    break

        except KeyboardInterrupt:
            metadata["stopped_by_keyboard_interrupt"] = True
            stop_reason = "keyboard-interrupt"
            print("\nStopping console...")
        except serial.SerialException:
            metadata["serial_disconnect_detected"] = True
            stop_reason = "serial-disconnect"
            print("\nUSB serial connection lost.")

    metadata["finished_at_unix"] = time.time()
    metadata["password_prompt_seen"] = password_prompt_seen
    metadata["password_attempts"] = attempts
    metadata["stop_reason"] = stop_reason
    metadata["rx_bytes"] = rx_total
    metadata["tx_bytes"] = tx_total

    hashes = {}
    for path in (rx_path, tx_path, terminal_path):
        if path.exists():
            hashes[path.name] = sha256_file(path)
    metadata["sha256"] = hashes

    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    with (out / "SHA256SUMS").open("w", encoding="utf-8") as f:
        for name, digest in hashes.items():
            f.write(f"{digest}  {name}\n")

    print("\nSession finalized:")
    print(f"  RX bytes: {rx_total}")
    print(f"  TX bytes: {tx_total}")
    print(f"  Password prompts: {password_prompt_seen}")
    print(f"  Attempts sent: {len(attempts)}")
    print(f"  Stop reason: {stop_reason}")
    print(f"  Output: {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
