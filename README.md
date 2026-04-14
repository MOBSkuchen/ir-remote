# ESP32 IR Remote

Learn, save, and send IR signals via an ESP32.
Either use the web interface or leverage the API.

## Hardware

I used a base ESP32 together with the following IR modules.
This *should* work on other hardware too tho. 

| Module  | ESP32 Pin |
|---------|-----------|
| KY-022 (receiver) S | GPIO 15 |
| KY-005 (emitter) S  | GPIO 4  |

## Build & Flash (Arduino IDE)

1. Install **ESP32** board package.
2. Install libraries: **IRremoteESP8266**, **ArduinoJson**.
3. Open `ir_remote.ino`, select your ESP32 board, flash.

## Build & Flash (PlatformIO)

```
cd ir_remote
pio run -t upload
```

## WiFi Setup

```
pip install -r requirements.txt
python flash_wifi.py --ssid "YourNetwork" --password "YourPassword"
```

The script sends credentials over serial. The ESP32 connects to your
WiFi and prints back the assigned IP. Credentials are persisted in flash,
so the ESP32 reconnects automatically on reboot.

### Serial Protocol

| Command | Response |
|---------|----------|
| `WIFI:ssid\|password` | `OK:IP:x.x.x.x` or `ERR:...` |
| `PING` | `OK:IP:x.x.x.x` or `OK:NOCONN` |
| `RESET_WIFI` | `OK:CLEARED` |

## HTTP API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/`      | GET    | Web UI |
| `/learn` | GET    | Blocks until IR signal received (15s timeout). Returns `{protocol, bits, raw}` |
| `/send`  | POST   | Sends raw signal. Body: `{raw: "...", hz: 38}` or `{name: "...", hz: 38}` |
| `/save`  | POST   | Saves signal. Body: `{name: "...", raw: "..."}` |
| `/list`  | GET    | Returns saved signals: `[{name, raw}, ...]` |
| `/delete`| POST   | Deletes signal. Body: `{name: "..."}` |

The `raw` format is comma-separated: first value is the count of timing
entries, followed by mark/space durations in microseconds.
