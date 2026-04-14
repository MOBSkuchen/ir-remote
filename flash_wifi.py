import argparse
import sys
import time

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("Missing pyserial. Install with: pip install pyserial")
    sys.exit(1)


def find_esp32():
    candidates = []
    for p in serial.tools.list_ports.comports():
        desc = (p.description or "").lower()
        vid = p.vid or 0
        if "cp210" in desc or "ch340" in desc or "ftdi" in desc or vid in (0x10C4, 0x1A86, 0x0403):
            candidates.append(p.device)
    if not candidates:
        for p in serial.tools.list_ports.comports():
            if "usb" in (p.device or "").lower() or "ttyUSB" in (p.device or "") or "ttyACM" in (p.device or ""):
                candidates.append(p.device)
    return candidates


def read_line(ser, timeout=20):
    start = time.time()
    while time.time() - start < timeout:
        if ser.in_waiting:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if line:
                return line
        time.sleep(0.05)
    return None


def flash_wifi(port, ssid, password):
    print(f"Opening {port}...")
    ser = serial.Serial(port, 115200, timeout=1)
    time.sleep(0.5)

    ser.setDTR(False)
    time.sleep(0.1)
    ser.setDTR(True)
    time.sleep(2)

    print("Waiting for ESP32...")
    while True:
        line = read_line(ser, timeout=10)
        if line is None:
            print("No response. Sending PING...")
            ser.write(b"PING\n")
            line = read_line(ser, timeout=5)
        if line is None:
            print("ESP32 not responding. Check connection.")
            ser.close()
            return None
        if line.startswith("OK:IP:"):
            ip = line[6:]
            print(f"Already connected at {ip}")
            resp = input("Reconfigure? (y/N): ").strip().lower()
            if resp != "y":
                ser.close()
                return ip
            break
        if line in ("READY", "NEED_WIFI"):
            break

    cmd = f"WIFI:{ssid}|{password}\n"
    print(f"Sending credentials (SSID: {ssid})...")
    ser.write(cmd.encode("utf-8"))

    while True:
        line = read_line(ser, timeout=20)
        if line is None:
            print("Timeout waiting for response.")
            ser.close()
            return None
        if line.startswith("OK:IP:"):
            ip = line[6:]
            print(f"\nConnected! IP: {ip}")
            print(f"Web UI: http://{ip}/")
            ser.close()
            return ip
        if line.startswith("ERR:"):
            print(f"Failed: {line[4:]}")
            ser.close()
            return None


def main():
    parser = argparse.ArgumentParser(description="Send WiFi credentials to ESP32 IR Remote over serial")
    parser.add_argument("--ssid", "-s", required=True)
    parser.add_argument("--password", "-p", required=True)
    parser.add_argument("--port", help="Serial port (auto-detected if omitted)")
    args = parser.parse_args()

    if args.port:
        port = args.port
    else:
        ports = find_esp32()
        if not ports:
            print("No ESP32 found. Specify --port manually.")
            sys.exit(1)
        if len(ports) > 1:
            print("Multiple ports:")
            for i, p in enumerate(ports):
                print(f"  [{i}] {p}")
            port = ports[int(input("Select: ").strip())]
        else:
            port = ports[0]
            print(f"Found ESP32 on {port}")

    ip = flash_wifi(port, args.ssid, args.password)
    if not ip:
        sys.exit(1)


if __name__ == "__main__":
    main()
