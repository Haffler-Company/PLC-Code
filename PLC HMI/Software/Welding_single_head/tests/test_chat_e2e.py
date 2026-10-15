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
    "http://localhost:8080/chat.html"
], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

time.sleep(2)

try:
    req = urllib.request.urlopen("http://localhost:9222/json")
    targets = json.loads(req.read().decode('utf-8'))
    page_target = next((t for t in targets if t.get("type") == "page" and "chat.html" in t.get("url", "")), None)
    if not page_target:
        page_target = next((t for t in targets if t.get("type") == "page"), targets[0])
        
    ws_url = page_target["webSocketDebuggerUrl"]
    
    async def run_cdp():
        async with websockets.connect(ws_url) as ws:
            await ws.send(json.dumps({"id": 1, "method": "Runtime.enable"}))
            await asyncio.sleep(1.5)
            
            # Send message through input and click send
            eval_req = {
                "id": 10,
                "method": "Runtime.evaluate",
                "params": {
                    "expression": """(async function() {
                        var inp = document.getElementById('chatInput');
                        inp.value = 'وین تک';
                        sendMessage();
                        await new Promise(r => setTimeout(r, 400));
                        var msgs = Array.from(document.querySelectorAll('.msg')).map(m => ({
                            isBot: m.classList.contains('bot'),
                            textSnippet: m.innerText.substring(0, 100)
                        }));
                        var chips = Array.from(document.querySelectorAll('.chip')).map(c => c.innerText);
                        return {
                            msgCount: msgs.length,
                            messages: msgs,
                            chips: chips
                        };
                    })()""",
                    "awaitPromise": True,
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
                    print("[END-TO-END CHAT INTERACTION RESULT]")
                    print(json.dumps(val, indent=2, ensure_ascii=False))
                    print("="*50 + "\n")
                    break

    asyncio.run(run_cdp())

except Exception as e:
    print(f"[ERROR] Test failed: {e}")
finally:
    proc.terminate()
