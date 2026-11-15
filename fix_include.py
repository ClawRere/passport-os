# -*- coding: utf-8 -*-
path = r'D:\ai\DEEPSSEK\passport-os\main\apps\voicememo\app_voicememo.cc'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

old = '#include "ui/ui_pixel.h"'
new = '#include "ui_pixel.h"'
if old in content:
    content = content.replace(old, new)
    with open(path, 'w', encoding='utf-8', newline='') as f:
        f.write(content)
    print('Fixed ui_pixel.h include in voicememo')
else:
    print('Already correct or not found')