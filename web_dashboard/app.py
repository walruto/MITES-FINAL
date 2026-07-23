import re
import threading
import time

import serial
from flask import Flask, jsonify, render_template

# CHANGE THIS to match your board's port. Find it with `ls /dev/cu.*`
# while the board is plugged in (or check Tools > Port in the Arduino IDE).
SERIAL_PORT = "/dev/cu.usbmodem1101"
BAUD_RATE = 9600

app = Flask(__name__)

state_lock = threading.Lock()
state = {
    "distance_cm": None,
    "status": "unknown",     # "DETECTED" or "clear"
    "link": "unknown",       # "CONNECTED" or "NOT CONNECTED"
    "laps": [],              # [{"lap": n, "seconds": t}, ...]
    "connected": False,      # whether the serial port itself is open
}

DISTANCE_RE = re.compile(r"Distance:\s*(-?\d+)\s*cm")
STATUS_RE = re.compile(r"\|\s*(DETECTED|clear)")
LINK_RE = re.compile(r"Link:\s*(CONNECTED|NOT CONNECTED)")
LAP_RE = re.compile(r"Lap (\d+) completed in ([\d.]+) sec")


def read_serial():
    while True:
        try:
            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
                with state_lock:
                    state["connected"] = True
                while True:
                    line = ser.readline().decode(errors="ignore").strip()
                    if not line:
                        continue

                    with state_lock:
                        dist_match = DISTANCE_RE.search(line)
                        if dist_match:
                            state["distance_cm"] = int(dist_match.group(1))

                        status_match = STATUS_RE.search(line)
                        if status_match:
                            state["status"] = status_match.group(1)

                        link_match = LINK_RE.search(line)
                        if link_match:
                            state["link"] = link_match.group(1)

                        lap_match = LAP_RE.search(line)
                        if lap_match:
                            state["laps"].append({
                                "lap": int(lap_match.group(1)),
                                "seconds": float(lap_match.group(2)),
                            })
        except serial.SerialException:
            with state_lock:
                state["connected"] = False
            time.sleep(2)  # port not available yet (unplugged, wrong name) - keep retrying


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/data")
def data():
    with state_lock:
        return jsonify(state)


if __name__ == "__main__":
    threading.Thread(target=read_serial, daemon=True).start()
    app.run(host="0.0.0.0", port=5000)
