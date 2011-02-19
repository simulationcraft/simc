#!/usr/bin/python

import re
import string

# Read current styles.css file and write slimmed-down version for inclusion
# in sc_report.cpp

INFILE = open('../html/css/styles-new.css', 'r')
OUTFILE = open('cpp-styles.txt', 'w')

lines = []
for line in INFILE.readlines():
    line = string.strip(line)
    if not line:
        continue
    line = re.sub('"', '\\"', line)
    lines.append(line)
line_count = len(lines)

OUTFILE.write('    util_t::fprintf( file,\n')
new_line = '      "      '
line_num = 0

for line in lines:
    line_num += 1
    if line == '}':
        new_line = '{0} }}\\n"'.format(new_line)
        if line_num == line_count:
            new_line = '{0} );'.format(new_line)
        new_line = '{0}\n'.format(new_line)
        OUTFILE.write(new_line)
        new_line = '      "      '
        continue
    
    new_line = '{0} {1}'.format(new_line, line)