# -*- coding: utf-8 -*-
"""Clean restore and fix voicememo.cc from scratch."""
import re, os

repo = r'D:\ai\DEEPSSEK\passport-os'
path = os.path.join(repo, 'main/apps/voicememo/app_voicememo.cc')

# Get clean version from 789eee3 (original valid UTF-8)
import subprocess
result = subprocess.run(
    ['git', 'show', '789eee3:main/apps/voicememo/app_voicememo.cc'],
    capture_output=True, cwd=repo
)
data = result.stdout
if data[:3] == b'\xef\xbb\xbf':
    data = data[3:]  # strip BOM
if not data.endswith(b'\n'):
    data += b'\n'

# Fix 1: Add bsp_audio_init_playback() in init()
old_init = b'    mkdir(s_dir, 0755);\n    scan_files(); s_cur = 0;\n}'
new_init = b'    mkdir(s_dir, 0755);\n    bsp_audio_init_playback();\n    scan_files(); s_cur = 0;\n}'
assert old_init in data, f'ERROR: init block not found'
data = data.replace(old_init, new_init, 1)
print('Fix 1: Added bsp_audio_init_playback() call')

# Fix 2: lv_label_set_text -> lv_label_set_text_fmt for format string with %d and s_total
pattern = rb'(lv_label_set_text)\((s_status,\s*"[^"]*%d[^"]*"\s*,\s*s_total\))'
matches = list(re.finditer(pattern, data))
print(f'Found {len(matches)} format-string call(s) to fix')
for m in matches:
    before = data[max(0,m.start()-20):m.start()]
    after = data[m.end():m.end()+20]
    print(f'  Match at {m.start()}: ...{before[-15:]}... {m.group(0)[:60]} ...{after[:15]}...')
    # Replace just the function name part
    data = data[:m.start()] + b'lv_label_set_text_fmt' + data[m.start()+len(b'lv_label_set_text'):]
    print(f'  Replaced')

# Verify
try:
    text = data.decode('utf-8')
    print(f'Valid UTF-8: {len(text)} chars')
except UnicodeDecodeError as e:
    print(f'ERROR at pos {e.start}: byte=0x{data[e.start]:02x}')
    start = max(0, e.start - 20)
    end = min(len(data), e.start + 20)
    print(f'  Context: {data[start:end].hex()}')

with open(path, 'wb') as f:
    f.write(data)
print('Written to disk')
