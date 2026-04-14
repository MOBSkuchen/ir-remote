import sys
import time
import random

def get_random_spinner():
    spinners = [
    "dots", "dots2", "dots3", "dots4", "dots5", "dots6", "dots7", "dots8", 
    "dots9", "dots10", "dots11", "dots12", "dots8Bit", "line", "line2", 
    "pipe", "star", "star2", "flip", "hamburger", "growVertical", 
    "growHorizontal", "grenade", "point", "layer", "balloon", "balloon2", 
    "noise", "bounce", "boxBounce", "boxBounce2", "triangle", "arc", 
    "circle", "squareCorners", "circleQuarters", "circleHalves", "squish", 
    "toggle", "toggle2", "toggle3", "toggle4", "toggle5", "toggle6", 
    "toggle7", "toggle8", "toggle9", "toggle10", "toggle11", "toggle12", 
    "toggle13", "arrow"
    ]    
    return random.choice(spinners)

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("Missing pyserial. Install with: pip install pyserial")
    sys.exit(1)

try:
    from rich.console import Console
    from rich.panel import Panel
    from rich.prompt import Prompt, Confirm, IntPrompt
    from rich.table import Table
    from rich.text import Text
    from rich import box
    import click
except ImportError:
    print("Missing dependencies. Install with: pip install rich click")
    sys.exit(1)

console = Console()

BANNER = Text.from_markup(
    "[bold cyan]IR-Remote Flasher[/bold cyan]\n"
    "[dim]Send WiFi credentials to your IR Remote over serial[/dim]"
)


def find_esp32():
    candidates = []
    for p in serial.tools.list_ports.comports():
        desc = (p.description or "").lower()
        vid = p.vid or 0
        if "cp210" in desc or "ch340" in desc or "ftdi" in desc or vid in (0x10C4, 0x1A86, 0x0403):
            candidates.append(p)
    if not candidates:
        for p in serial.tools.list_ports.comports():
            if "usb" in (p.device or "").lower() or "ttyUSB" in (p.device or "") or "ttyACM" in (p.device or ""):
                candidates.append(p)
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
    with console.status(f"[bold yellow]Opening {port}…", spinner=get_random_spinner()) as status:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(0.5)
        ser.setDTR(False)
        time.sleep(0.1)
        ser.setDTR(True)
        time.sleep(2)

        status.update("[bold yellow]Waiting for IR-Remote to respond…", spinner=get_random_spinner())
        while True:
            line = read_line(ser, timeout=10)
            if line is None:
                status.update("[bold yellow]No response[/] — sending PING…", spinner=get_random_spinner())
                ser.write(b"PING\n")
                line = read_line(ser, timeout=5)
            if line is None:
                console.print("[red]Device not responding. Check your connection.")
                ser.close()
                return None
            if line.startswith("OK:IP:"):
                ip = line[6:]
                status.stop()
                console.print(f"[green]Already connected at [cyan]{ip}[/cyan]")
                if not Confirm.ask("  Reconfigure anyway?", default=False):
                    ser.close()
                    return ip
                break
            if line in ("READY", "NEED_WIFI"):
                break

        status.update(f"[bold yellow]Sending credentials (SSID: {ssid})…", spinner=get_random_spinner())
        cmd = f"WIFI:{ssid}|{password}\n"
        ser.write(cmd.encode("utf-8"))

        while True:
            line = read_line(ser, timeout=20)
            if line is None:
                console.print("[bold red]Timeout[/bold] waiting for response.")
                ser.close()
                return None
            if line.startswith("OK:IP:"):
                ip = line[6:]
                status.stop()
                console.print(Panel(
                    f"[bold green]Flashed![/bold green]\n\n"
                    f"  IP Address : [cyan]{ip}[/cyan]\n"
                    f"  Web UI     : [link=http://{ip}/]http://{ip}/[/link]",
                    title="[bold]Success[/bold]",
                    border_style="green",
                    box=box.ROUNDED,
                    width=60,
                    padding=(1, 2),
                ))
                ser.close()
                return ip
            if line.startswith("ERR:"):
                console.print(f"[bold red]{line[4:]}")
                ser.close()
                return None


def pick_port(ports):
    table = Table(
        title="[bold]Detected Serial Ports[/bold]",
        box=box.SIMPLE_HEAVY,
        title_style="cyan",
        header_style="bold",
        row_styles=["", "dim"],
    )
    table.add_column("#", justify="right", style="bold cyan", width=3)
    table.add_column("Port", style="green")
    table.add_column("Description", style="white")
    for i, p in enumerate(ports):
        table.add_row(str(i), p.device, p.description or "—")
    console.print(table)
    idx = IntPrompt.ask("Select port", choices=[str(i) for i in range(len(ports))])
    return ports[idx].device


def resolve_port(port):
    if port:
        return port

    with console.status("[bold yellow]Scanning for devices…", spinner=get_random_spinner()):
        ports = find_esp32()
        time.sleep(0.4)

    if not ports:
        console.print("[bold][red]Not Found[/][red]. Specify a port with [cyan]--port[/cyan]")
        sys.exit(1)
    elif len(ports) > 1:
        return pick_port(ports)
    else:
        console.print(f"[bold][green]Found[/][green] IR-Remote on [cyan]{ports[0].device}[/cyan]")
        return ports[0].device


@click.command()
@click.option("--ssid", "-s", default=None, help="WiFi network name (prompted if omitted)")
@click.option("--password", "-p", default=None, help="WiFi password (prompted if omitted)")
@click.option("--port", default=None, help="Serial port (auto-detected if omitted)")
def main(ssid, password, port):
    console.print()
    console.print(Panel(BANNER, title="[bold cyan]Welcome", box=box.ROUNDED, border_style="bright_cyan", padding=(1, 3)), width=60)

    if not ssid:
        ssid = Prompt.ask("[bold]WiFi SSID[/bold]")
    if not password:
        password = Prompt.ask("[bold]WiFi Password[/bold]", password=True)

    console.print()
    port = resolve_port(port)
    console.print()

    ip = flash_wifi(port, ssid, password)
    if not ip:
        sys.exit(1)


if __name__ == "__main__":
    main()