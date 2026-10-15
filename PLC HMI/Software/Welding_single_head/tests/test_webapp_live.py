import http.server
import socketserver
import threading
import asyncio
import websockets
import json
import time
import os
import sys

DATA_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "data"))
HTTP_PORT = 8080
WS_PORT = 81

PLC_DATA = {
    "DID": 555,
    "RQ": 1,
    "FW_VER": "2.0.0",
    "MACHINE_NAME": "Welding Single Head",
    "DEV_NAME": "Haffler PLC 10K",
    "TEMP": 245.5,
    "SET_TEMP": 245,
    "HEATER_ACTIVE": 1,
    "WELD_STAT": "READY",
    "CNT_TOTAL": 1420,
    "CNT_SESSION": 45,
    "Q_PRS": 0,
    "Q_TAB": 0,
    "Q_ELM": 0,
    "Q_SHB": 0,
    "EMG": 0,
    "PED": 0,
    "IN_BITS": 0,
    "OUT_BITS": 0,
    "TEMP_MELT": 15,
    "TEMP_WELD": 25,
    "PROFILES": [
        {"name": "WinTech 60", "temp": 245, "melt": 15, "weld": 25},
        {"name": "Vistabest", "temp": 250, "melt": 16, "weld": 26},
        {"name": "Hoffer 60", "temp": 240, "melt": 14, "weld": 24}
    ]
}

class CustomHTTPHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DATA_DIR, **kwargs)

    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        super().end_headers()

def run_http_server():
    with socketserver.TCPServer(("", HTTP_PORT), CustomHTTPHandler) as httpd:
        print(f"[HTTP] Server running on http://localhost:{HTTP_PORT} (serving {DATA_DIR})")
        httpd.serve_forever()

async def ws_handler(websocket):
    print(f"[WS] Client connected from {websocket.remote_address}")
    await websocket.send(json.dumps(PLC_DATA))
    try:
        async for message in websocket:
            print(f"[WS] Received from client: {message[:100]}")
            try:
                req = json.loads(message)
                resp = {"status": "ok", "ack": req.get("RQ", 0)}
                await websocket.send(json.dumps(resp))
            except Exception:
                pass
    except Exception:
        print("[WS] Client disconnected")

async def run_ws_server():
    async with websockets.serve(ws_handler, "0.0.0.0", WS_PORT, subprotocols=["arduino"]):
        print(f"[WS] WebSocket server running on ws://localhost:{WS_PORT}")
        await asyncio.Future()

if __name__ == "__main__":
    t_http = threading.Thread(target=run_http_server, daemon=True)
    t_http.start()
    asyncio.run(run_ws_server())
