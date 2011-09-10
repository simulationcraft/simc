#!/usr/bin/python

import re, sys, tempfile

LENGTH=77

delimiter_re = re.compile(r'^\s*//.*========$')

dbg = sys.stderr

def scanlines(lines, out):
    dirty = False
    lineno = 0
    for line in lines:
        lineno += 1
        line = line.rstrip()
        if delimiter_re.match(line) is not None:
            l = len(line)
            if l < LENGTH:
                out.flush()
                # dbg.write('Line %d too short: (%d < %d)\n' % (lineno, l, length))
                line += '=' * (LENGTH - l)
                dirty = True
            elif l > LENGTH:
                out.flush()
                # dbg.write('Line %d too long: (%d > %d)\n' % (lineno, l, length))
                line = line[:LENGTH]
                dirty = True
        out.write(line)
        out.write('\n')
    return dirty

def rewritefile(name):
    infile = open(name, 'rt')
    outfile = tempfile.SpooledTemporaryFile(mode='w+t')
    dirty = scanlines(iter(infile), outfile)
    infile.close()
    infile = open(name, 'wt')
    outfile.seek(0)
    for l in outfile:
        infile.write(l)
    outfile.close()

for name in sys.argv[1:]: rewritefile(name)
