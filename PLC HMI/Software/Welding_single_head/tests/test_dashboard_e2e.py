import subprocess
import time
import urllib.request
import json
import asyncio
import websockets
import sys
import os

sys.stdout.reconfigure(encoding='utf-8')

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
    page_target = next((t for t in targets if t.get("type") == "page" and "index.html" in t.get("url", "")), None)
    if not page_target:
        page_target = next((t for t in targets if t.get("type") == "page"), targets[0])
        
    ws_url = page_target["webSocketDebuggerUrl"]
    
    async def run_cdp():
        async with websockets.connect(ws_url) as ws:
            await ws.send(json.dumps({"id": 1, "method": "Runtime.enable"}))
            await asyncio.sleep(2)
            
            eval_req = {
                "id": 10,
                "method": "Runtime.evaluate",
                "params": {
                    "expression": """(function() {
                        var tempEl = document.getElementById('txt_temp') || document.querySelector('.temp-display') || document.getElementById('TM');
                        var badge = document.getElementById('liveConnectionBadge');
                        var splash = document.getElementById('appSplashScreen');
                        var tabs = Array.from(document.querySelectorAll('.tab, button, [onclick*="switchTab"]')).map(t => t.innerText.trim()).filter(Boolean);
                        return {
                            pageTitle: document.title,
                            splashOpacity: splash ? splash.style.opacity : 'no-splash',
                            splashDisplay: splash ? splash.style.display : 'no-splash',
                            isOnline: typeof ONLINE_Flag !== 'undefined' ? ONLINE_Flag : false,
                            badgeColor: badge ? badge.style.background : 'no-badge',
                            domButtonsCount: document.querySelectorAll('button').length,
                            inputElementsCount: document.querySelectorAll('input').length,
                            availableTabs: tabs.slice(0, 10)
                        };
                    })()""",
                    "returnByValue": True
                }
            }
            await ws.send(json.dumps(eval_req))
            
            while True:
                msg = await asyncio.wait_for(ws.recv(), timeout=5)
                data = json.loads(msg)
                if data.get("id") == 10:
                    val = data.get("result", {}).get("result", {}).get("value", data)
                    print("\n" + "="*50)
                    print("[MAIN DASHBOARD VERIFICATION RESULT]")
                    print(json.dumps(val, indent=2, ensure_ascii=False))
                    print("="*50 + "\n")
                    break

    asyncio.run(run_cdp())

except Exception as e:
    print(f"[ERROR] Main dashboard test failed: {e}")
finally:
    proc.terminate()
