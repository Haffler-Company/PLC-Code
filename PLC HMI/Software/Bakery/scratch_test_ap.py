import sys
sys.stdout.reconfigure(encoding='utf-8')

bell_bytes = b"\xef\x83\xb3"
bell_char = bell_bytes.decode('utf-8')
print("Bell codepoint:", hex(ord(bell_char)))

ok_bytes = b"\xef\x80\x8c"
ok_char = ok_bytes.decode('utf-8')
print("OK codepoint:", hex(ord(ok_char)))
