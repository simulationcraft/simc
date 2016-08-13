#!/usr/bin/env python3

import sys, os, pathlib, hashlib, shutil

if len(sys.argv) < 3:
    print('copy-hotfixes.py input_dir output_dir')
    sys.exit(1)

if not os.access(sys.argv[1], os.R_OK):
    print('copy-hotfixes.py: Unable to access %s' % sys.argv[1])
    sys.exit(1)

if not os.access(sys.argv[2], os.R_OK):
    print('copy-hotfixes.py: Unable to access %s' % sys.argv[2])
    sys.exit(1)

WCH56_TIMESTAMP_OFFSET = 32
WCH7_TIMESTAMP_OFFSET = 36

_input_base = pathlib.Path(sys.argv[1])
_output_base = pathlib.Path(sys.argv[2])
_input = {}
_output = {}

print('Reading output files from %s' % sys.argv[2])
for fn in _output_base.glob('*.adb'):
    data = bytearray(fn.read_bytes())
    if data[:4] == b'WCH5' or data[:4] == b'WCH6':
        data[WCH56_TIMESTAMP_OFFSET:WCH56_TIMESTAMP_OFFSET + 4] = b'\x00\x00\x00\x00'
    else:
        data[WCH7_TIMESTAMP_OFFSET:WCH7_TIMESTAMP_OFFSET + 4] = b'\x00\x00\x00\x00'

    _output[fn.stem] = { 'name': fn, 'md5': hashlib.md5(data).digest() }

print('Reading input files from %s' % sys.argv[1])
for fn in _input_base.glob('*.adb'):
    if fn.stem not in _output.keys():
        continue

    data = bytearray(fn.read_bytes())
    if data[:4] == b'WCH5' or data[:4] == b'WCH6':
        data[WCH56_TIMESTAMP_OFFSET:WCH56_TIMESTAMP_OFFSET + 4] = b'\x00\x00\x00\x00'
    else:
        data[WCH7_TIMESTAMP_OFFSET:WCH7_TIMESTAMP_OFFSET + 4] = b'\x00\x00\x00\x00'

    _input[fn.stem] = { 'name': fn, 'md5': hashlib.md5(data).digest() }

# Copy over any modified files
for _fn, _data in _output.items():
    if _fn not in _input:
      continue
    
    if _data['md5'] != _input[_fn]['md5']:
        print('Copying %s -> %s ...' % (_input[_fn]['name'], _data['name']))
        try:
            shutil.copyfile(str(_input[_fn]['name']), str(_data['name']))
        except Exception as e:
            print('copy-hotfixes.py: Copy error on %s: %s' % (_fn, e))
            sys.exit(1)
