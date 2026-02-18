#!/usr/bin/env python3
import argparse
import string
import sys
import time
from dataclasses import dataclass

try:
    import serial
except ImportError:
    print("pyserial is required. Install with: pip install pyserial", file=sys.stderr)
    sys.exit(2)


@dataclass
class ProbeResult:
    baud: int
    bytes_read: int
    printable_ratio: float
    keyword_hits: int
    score: float
    preview: str


def to_preview(data: bytes, max_len: int = 120) -> str:
    text = data.decode("utf-8", errors="replace")
    text = " ".join(text.split())
    return text[:max_len]


def printable_ratio(data: bytes) -> float:
    if not data:
        return 0.0
    printable = set(bytes(string.printable, "ascii"))
    good = sum(1 for b in data if b in printable)
    return good / len(data)


def keyword_hits(text: str) -> int:
    keys = [
        "boot", "bios", "sbl", "uefi", "error", "stage1", "stage2",
        "intel", "shell", "press", "starting", "linux", "congatec",
    ]
    t = text.lower()
    return sum(1 for k in keys if k in t)


def probe_once(port: str, baud: int, seconds: float, reset_lines: bool) -> ProbeResult:
    data = b""
    with serial.Serial(port=port, baudrate=baud, timeout=0.1) as ser:
        if reset_lines:
            try:
                ser.dtr = False
                ser.rts = False
                time.sleep(0.05)
                ser.reset_input_buffer()
            except Exception:
                pass

        end = time.time() + seconds
        while time.time() < end:
            chunk = ser.read(512)
            if chunk:
                data += chunk

    text = data.decode("utf-8", errors="replace")
    pr = printable_ratio(data)
    hits = keyword_hits(text)

    # Weighted heuristic:
    # - prioritize readable output
    # - bonus for meaningful boot-like keywords
    # - tiny bonus for having data at all
    score = (pr * 100.0) + (hits * 8.0) + (1.5 if len(data) > 0 else 0.0)

    return ProbeResult(
        baud=baud,
        bytes_read=len(data),
        printable_ratio=pr,
        keyword_hits=hits,
        score=score,
        preview=to_preview(data),
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Probe likely UART baud rate by readability scoring")
    parser.add_argument("--port", default="/dev/ttyUSB0", help="Serial port (default: /dev/ttyUSB0)")
    parser.add_argument(
        "--bauds",
        default="115200,57600,38400,19200,9600,230400,460800,921600",
        help="Comma-separated baud list",
    )
    parser.add_argument("--seconds", type=float, default=2.0, help="Capture time per baud")
    parser.add_argument("--top", type=int, default=3, help="Show top N candidates")
    parser.add_argument(
        "--no-reset-lines",
        action="store_true",
        help="Do not toggle DTR/RTS before capture",
    )
    args = parser.parse_args()

    try:
        baud_list = [int(x.strip()) for x in args.bauds.split(",") if x.strip()]
    except ValueError:
        print("Invalid --bauds format. Example: --bauds 115200,57600,38400", file=sys.stderr)
        return 2

    results = []
    print(f"Probing {args.port} across {len(baud_list)} baud rates...")
    for baud in baud_list:
        try:
            res = probe_once(args.port, baud, args.seconds, not args.no_reset_lines)
            results.append(res)
            print(
                f"  {baud:>7}: bytes={res.bytes_read:>5} printable={res.printable_ratio:>5.1%} "
                f"hits={res.keyword_hits:>2} score={res.score:>6.1f}"
            )
        except serial.SerialException as exc:
            print(f"  {baud:>7}: error: {exc}")

    if not results:
        print("No successful reads. Check port permissions/cable/power.", file=sys.stderr)
        return 1

    ranked = sorted(results, key=lambda r: r.score, reverse=True)
    best = ranked[0]

    print("\nTop candidates:")
    for row in ranked[: max(1, args.top)]:
        print(
            f"- {row.baud}: score={row.score:.1f}, bytes={row.bytes_read}, "
            f"printable={row.printable_ratio:.1%}, keyword_hits={row.keyword_hits}, preview='{row.preview}'"
        )

    print(f"\nLikely baud: {best.baud}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
