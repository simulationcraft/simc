#!/usr/bin/python

import re
import string

# Read current styles.css file and write slimmed-down version for inclusion
# in report\sc_report.cpp

OUTFILE = open('cpp-styles.txt', 'w')

OUTFILE.write('  // Styles\n')
OUTFILE.write('  // If file is being hosted on simulationcraft.org, link to the local\n')
OUTFILE.write('  // stylesheet; otherwise, embed the styles.\n')
OUTFILE.write('  if ( sim -> hosted_html )\n')
OUTFILE.write('  {\n')
OUTFILE.write('    os << "\\t\\t<style type=\\"text/css\\" media=\\"screen\\">\\n"\n')
OUTFILE.write('       << "\\t\\t\\t@import url(\'http://www.simulationcraft.org/css/styles.css\');\\n"\n')
OUTFILE.write('       << "\\t\\t</style>\\n"\n')
OUTFILE.write('       << "\\t\\t<style type=\\"text/css\\" media=\\"print\\">\\n"\n')
OUTFILE.write('       << "\\t\\t\t@import url(\'http://www.simulationcraft.org/css/styles-print.css\');\\n"\n')
OUTFILE.write('       << "\\t\\t</style>\\n";\n')
OUTFILE.write('  }\n')
OUTFILE.write('  else if ( sim -> print_styles )\n')
OUTFILE.write('  {\n')
OUTFILE.write('    os << "\\t\\t<style type=\\"text/css\\" media=\\"all\\">\\n"\n')

INFILE = open('../../simulationcraft_html/css/styles-print.css', 'r')
lines = []
comment = False
for line in INFILE.readlines():
    line = string.strip(line)
    if not line:
        continue
    if line[0:2] == '/*':
        comment = True
        continue
    elif line[0:2] == '*/':
        comment = False
        continue
    if comment:
        continue
    line = re.sub('"', '\\"', line)
    lines.append(line)
line_count = len(lines)

new_line = '       << "\\t\\t\\t'

for line in lines:
    if line == '}':
        new_line = '{0} }}\\n"'.format(new_line)
        new_line = '{0}\n'.format(new_line)
        OUTFILE.write(new_line)
        new_line = '       << "\\t\\t\\t'
        continue
    
    new_line = '{0}{1}'.format(new_line, line)

OUTFILE.write('       << "\\t\\t</style>\\n";\n')
INFILE.close()

OUTFILE.write('  }\n')
OUTFILE.write('  else\n')
OUTFILE.write('  {\n')
OUTFILE.write('    os << "\\t\\t<style type=\\"text/css\\" media=\\"all\\">\\n"\n')

INFILE = open('../../simulationcraft_html/css/styles.css', 'r')
lines = []
comment = False
for line in INFILE.readlines():
    line = string.strip(line)
    if not line:
        continue
    if line[0:2] == '/*':
        comment = True
        continue
    elif line[0:2] == '*/':
        comment = False
        continue
    elif re.search('images\/hosted_html\/bg\.jpg', line):
        line = re.sub(' url\(\.\.\/images\/hosted_html\/bg\.jpg\) \-100px 5px', '', line)
    if comment:
        continue
    line = re.sub('"', '\\"', line)
    lines.append(line)
line_count = len(lines)

new_line = '       << "\\t\\t\\t'

for line in lines:
    if line == '}':
        new_line = '{0} }}\\n"'.format(new_line)
        new_line = '{0}\n'.format(new_line)
        OUTFILE.write(new_line)
        new_line = '       << "\\t\\t\\t'
        continue
    
    new_line = '{0}{1}'.format(new_line, line)

OUTFILE.write('       << "\\t\\t</style>\\n";\n')
OUTFILE.write('  }\n')
INFILE.close()
