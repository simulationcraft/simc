# -*- coding: utf-8 -*-
"""
PTCH files are a container format for Blizzard patch files.
They begin with a 72 byte header containing some metadata, immediately
followed by a RLE-packed BSDIFF40.
The original BSDIFF40 format is compressed with bzip2 instead of RLE.
"""

##
#        Copyright (c) Jerome Leclanche for MMO-Champion. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
#     1. Redistributions of source code must retain the above copyright notice,
#        this list of conditions and the following disclaimer.
#
#     2. Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in the
#        documentation and/or other materials provided with the distribution.
#
#     3. Neither the names of MMO#Champion, Major League Gaming nor the name of the
#        author may be used to endorse or promote products derived from this
#        software without specific prior written permission.
#
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

from hashlib import md5
from struct import unpack
from binascii import hexlify
from cStringIO import StringIO

class PatchFile(object):
    def __init__(self, file):
        # Parse the header
        file.seek(0)
        assert file.read(4) == "PTCH"
        unk1 = file.read(4)
        self.sizeBefore, self.sizeAfter = unpack("ii", file.read(8))
        assert file.read(4) == "MD5_"
        assert unpack("i", file.read(4)) == (0x28, )
        self.md5Before, self.md5After = unpack("16s16s", file.read(32))
        self.md5Before, self.md5After = hexlify(self.md5Before), hexlify(self.md5After)
        assert file.read(4) == "XFRM"
        file.read(4)
        assert file.read(4) == "BSD0"
        self.fileSize, = unpack("i", file.read(4))
        
        self.compressedDiff = file.read()
        
        file.close()
    
    def __repr__(self):
        header = ("sizeBefore", "sizeAfter", "md5Before", "md5After", "fileSize")
        return "%s(%s)" % (self.__class__.__name__, ", ".join("%s=%r" % (k, getattr(self, k)) for k in header))
    
    def __bsdiffParseHeader(self, diff):
        """
        The BSDIFF header is as follows:
         - 8 bytes magic "BSDIFF40"
         - 8 bytes control block size
         - 8 bytes diff block size
         - 8 bytes new file size
        We read all this and make sure it's all valid.
        """
        assert diff.read(8) == "BSDIFF40"
        ctrlBlockSize, diffBlockSize, sizeAfter = unpack("QQQ", diff.read(24))
        assert ctrlBlockSize > 0 and diffBlockSize > 0
        assert sizeAfter == self.sizeAfter
        return ctrlBlockSize, diffBlockSize, sizeAfter
    
    
    def bsdiffParse(self):
        diff = StringIO(self.rleUnpack())
        ctrlBlockSize, diffBlockSize, sizeAfter = self.__bsdiffParseHeader(diff)
        
        ##
        # The data part of the file is divded into three parts
        # * The control block, which contains the control part every chunk
        # * The diff block, which contains the diff chunks
        # * And the extra block, which contains the extra chunks
        ctrlBlock = StringIO(diff.read(ctrlBlockSize))
        diffBlock = StringIO(diff.read(diffBlockSize))
        extraBlock = StringIO(diff.read())
        diff.close()
        
        return ctrlBlock, diffBlock, extraBlock
    
    def rleUnpack(self):
        """
        Read the RLE-packed data and
        return the unpacked output.
        """
        data = StringIO(self.compressedDiff)
        ret = []
        
        byte = data.read(1)
        while byte:
            byte = ord(byte)
            # Is it a repeat control?
            if byte & 0x80:
                count = (byte & 0x7F) + 1
                ret.append(data.read(count))
            
            else:
                ret.append("\0" * (byte+1))
            
            byte = data.read(1)
        
        return "".join(ret)
    
    def apply(self, old, validate=True):
        """
        Apply the patch to the old argument. Returns the output.
        If validate == True, will raise ValueError if the md5 of
        input or output is wrong.
        """
        if validate:
            hash = md5(old).hexdigest()
            if hash != self.md5Before:
                raise ValueError("Input MD5 fail. Expected %s, got %s." % (self.md5Before, hash))
        ctrlBlock, diffBlock, extraBlock = self.bsdiffParse()
        
        sizeBefore = len(old)
        new = ["\0" for i in range(self.sizeAfter)]
        
        cursor, oldCursor = 0, 0
        while cursor < self.sizeAfter:
            # Read control chunk
            diffChunkSize, extraChunkSize, extraOffset = unpack("<III", ctrlBlock.read(12))
            # Do negative encoding fixes, as data is in ints
            if diffChunkSize  & 0x80000000: diffChunkSize  = -(diffChunkSize & 0x7FFFFFFF)
            if extraChunkSize & 0x80000000: extraChunkSize = -(extraChunkSize & 0x7FFFFFFF)
            if extraOffset    & 0x80000000: extraOffset    = -(extraOffset & 0x7FFFFFFF)
            assert cursor + diffChunkSize <= self.sizeAfter
            
            # Read diff block
            new[cursor:cursor + diffChunkSize] = diffBlock.read(diffChunkSize)
            
            # Add old data to diff string
            for i in range(diffChunkSize):
                # if (oldCursor + i >= 0) and (oldCursor + i < sizeBefore)
                nb, ob = ord(new[cursor + i]), ord(old[oldCursor + i])
                new[cursor + i] = chr((nb + ob) % 256)
            
            # Update cursors
            cursor += diffChunkSize
            oldCursor += diffChunkSize
            assert cursor + extraChunkSize <= self.sizeAfter
            
            # Read extra chunk
            new[cursor:cursor + extraChunkSize] = extraBlock.read(extraChunkSize)
            
            # Update cursors
            cursor += extraChunkSize
            oldCursor += extraOffset
        
        ret = "".join(new)
        
        if validate:
            hash = md5(ret).hexdigest()
            if hash != self.md5After:
                # This likely means a parsing bug
                raise ValueError("Output MD5 fail. Expected %s, got %s" % (self.md5Before, hash))
        
        return ret

import io, shutil, sys, os

class PatchBuildDBC(object):
    def __init__(self, base, out, *args):
        self._base_dir = base
        self._output_dir = out
        self._patches = args[0]

    def initialize(self):
        if not os.access(self._base_dir, os.R_OK):
            sys.stderr.write('%s: Not readable\n' % os.path.abspath(self._base_dir))
            return False
    
        if not os.access(self._output_dir, os.W_OK):
            sys.stderr.write('%s: Not writable\n' % os.path.abspath(self._output_dir))
            return False
        
        for patch_dir in self._patches:
            if not os.access(patch_dir, os.R_OK):
                sys.stderr.write('%s: Not found\n' % os.path.abspath(patch_dir))
                return False
            elif patch_dir == self._output_dir:
                return False
            elif patch_dir == self._base_dir:
                return False
    
    def PatchFiles(self):
        # Copy the base directory to output
        for orig_file in os.listdir(self._base_dir):
            shutil.copy(os.path.join(self._base_dir, orig_file), self._output_dir)
        
        for patch_dir in self._patches:
            for patch_fname in sorted(os.listdir(patch_dir)):
                patch_file = os.path.join(patch_dir, patch_fname)
                output_file = os.path.join(self._output_dir, patch_fname)
                # If missing, treat the patch files as a completely new entry
                if not os.access(output_file, os.R_OK):
                    sys.stdout.write('[%s] Missing base file %s, presuming a new file\n' % (patch_dir, patch_fname))
                    shutil.copyfile(patch_file, output_file)
                    continue
                
                try:
                    f_patch = io.open(patch_file, r'rb')
                    patch = PatchFile(f_patch)
                # Not a patch, might be a new file, copy over
                except AssertionError:
                    sys.stdout.write('[%s] PTCH header check failure for %s, presuming a new file\n' % (patch_dir, patch_fname))
                    shutil.copyfile(patch_file, output_file)
                    continue
                
                # And now, patch the old file, it's already in the output dir
                try:
                    old_file = io.open(output_file, 'rb')
                    old_file_bytes = old_file.read()
                    old_file.close()
                    sys.stdout.write('[%s] Patching %s ...\n' % (patch_dir, patch_fname))
                    new_file_contents = patch.apply(old_file_bytes)
                except ValueError, e:
                    sys.stderr.write('%s: %s\n' % (output_file, e))
                    continue
                
                # new_file contains the file as a string, write it out to out_dir
                new_file = io.open(output_file, r'w+b')
                new_file.write(new_file_contents)
                new_file.close()
