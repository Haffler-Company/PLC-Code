import sys, pathlib
p = pathlib.Path(sys.argv[1])
t = p.read_text(encoding='utf-8', errors='ignore')
t = t.replace('\u200c', ' ').replace('\u200b', '').replace('\ufeff', '').replace('\u200e','').replace('\u200f','')
end = int(sys.argv[2]) if len(sys.argv) > 2 else None
start = int(sys.argv[3]) if len(sys.argv) > 3 else 0
print(t[start:end])
