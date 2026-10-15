import subprocess
import time
import urllib.request
import json
import asyncio
import websockets
import sys
import os

CHROME_PATH = r"C:\Program Files\Google\Chrome\Application\chrome.exe"
if not os.path.exists(CHROME_PATH):
    CHROME_PATH = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"

proc = subprocess.Popen([
    CHROME_PATH,
    "--headless=new",
    "--remote-debugging-port=9222",
    "--disable-gpu",
    "--no-first-run",
    "--no-default-browser-check",
    "http://localhost:8080/index.html"
], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

time.sleep(2)

try:
    req = urllib.request.urlopen("http://localhost:9222/json")
    targets = json.loads(req.read().decode('utf-8'))
    page_target = next((t for t in targets if t.get("type") == "page" and "localhost:8080" in t.get("url", "")), None)
    if not page_target:
        page_target = next((t for t in targets if t.get("type") == "page"), targets[0])
        
    ws_url = page_target["webSocketDebuggerUrl"]
    print(f"[TEST] Target URL: {page_target.get('url')} | WS: {ws_url}")
    
    async def run_cdp():
        async with websockets.connect(ws_url) as ws:
            await ws.send(json.dumps({"id": 1, "method": "Console.enable"}))
            await ws.send(json.dumps({"id": 2, "method": "Runtime.enable"}))
            await ws.send(json.dumps({"id": 3, "method": "Network.enable"}))
            
            await asyncio.sleep(3)
            
            eval_req = {
                "id": 10,
                "method": "Runtime.evaluate",
                "params": {
                    "expression": """({
                        title: document.title,
                        loadingDisplay: document.getElementById('loading') ? window.getComputedStyle(document.getElementById('loading')).display : 'no-loading',
                        onlineFlag: typeof ONLINE_Flag !== 'undefined' ? ONLINE_Flag : 'undefined',
                        hasConnection: typeof connection !== 'undefined' && connection !== null,
                        bodyLength: document.body.innerHTML.length,
                        hasTabs: document.querySelectorAll('.tab-btn, .tab').length,
                        tempValue: document.getElementById('txt_temp') ? document.getElementById('txt_temp').innerText : 'no-txt-temp'
                    })""",
                    "returnByValue": True
                }
            }
            await ws.send(json.dumps(eval_req))
            
            while True:
                msg = await asyncio.wait_for(ws.recv(), timeout=5)
                data = json.loads(msg)
                if data.get("id") == 10:
                    result = data.get("result", {}).get("result", {}).get("value", {})
                    print("\n" + "="*50)
                    print("[VERIFICATION RESULT]")
                    print(json.dumps(result, indent=2, ensure_ascii=False))
                    print("="*50 + "\n")
                    break
                if data.get("method") == "Console.messageAdded":
                    print(f"[CONSOLE] {data['params']['message']['level']}: {data['params']['message']['text']}")
                elif data.get("method") == "Runtime.consoleAPICalled":
                    args = " ".join([str(a.get("value", "")) for a in data["params"].get("args", [])])
                    print(f"[CONSOLE.{data['params']['type']}] {args}")

    asyncio.run(run_cdp())

except Exception as e:
    print(f"[ERROR] Verification failed: {e}")
finally:
    proc.terminate()
