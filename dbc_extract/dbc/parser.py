import os, io, struct, stat, sys
import data

DBC_HDR = r'WDBC'

class DBCParser(object):
    def __init__(self, options, fname):
        object.__init__(self)
        
        self._fname          = fname
        self._dbc_file       = None
        self._options        = options
        self._last_record_id = 0
        self._class          = None
        
    def name(self):
        return os.path.basename(self._fname).split('.')[0].lower()

    def find(self, id):
        original_offset = self._dbc_file.tell()

        raw_data = self.seek(0, self._records, id)

        self._dbc_file.seek(original_offset)

        return self._class(self, raw_data)

    def seek(self, begin, end, value):
        if begin > end:
            return None

        self._dbc_file.seek(20 + begin * self._record_size + ((end - begin) / 2) * self._record_size)
        raw_data = self._dbc_file.read(self._record_size)
        record_id = struct.unpack('I', raw_data[:4])[0]
        #print begin, end, begin + (end - begin) / 2, value, record_id, self._dbc_file.tell()
        if record_id < value:
            return self.seek(begin + (end - begin) / 2 + 1, end, value)
        elif record_id > value:
            return self.seek(begin, begin + (end - begin) / 2 - 1, value)
        else:
            return raw_data

    def open_dbc(self):
        if self._dbc_file:
            return self
            
        if not os.access(os.path.abspath(self._fname), os.R_OK):
            sys.stderr.write('%s: Not found\n' % os.path.abspath(self._fname))
            sys.exit(1)

        f = io.open(os.path.abspath(self._fname), mode = 'rb')

        if f.read(len(DBC_HDR)) != DBC_HDR:
            sys.stderr.write('%s: Not a World of Warcraft DBC File\n' % os.path.abspath(self._fname))
            f.close()
            sys.exit(1)

        try:
            records, fields, record_size, string_block_size = struct.unpack('IIII', f.read(16))
        except:
            sys.stderr.write('%s: Invalid DBC header format\n' % os.path.abspath(self._fname))
            f.close()
            sys.exit(1)

        self._records           = records
        self._fields            = fields
        self._record_size       = record_size
        self._string_block_size = string_block_size
        size                    = 20 + records * record_size + string_block_size
        statinfo                = os.fstat(f.fileno())
        
        #print records, fields, record_size, string_block_size, self._fname
        # Figure out a valid class for us from data, now that we know the DBC is sane
        if '%s%d' % ( os.path.basename(self._fname).split('.')[0], self._options.build or data.current_patch_level() ) in dir(data):
            self._class = getattr(data, '%s%d' % ( 
                os.path.basename(self._fname).split('.')[0], self._options.build or data.current_patch_level() ) )
        #else:
        #    sys.stderr.write('Warning, could not deduce a proper class for dbc file "%s.dbc", wow build %d. You need exact capitalization.\n' % (
        #        os.path.basename(self._fname).split('.')[0], self._options.build or data.current_patch_level() ))

        # Sanity check file size
        if statinfo[stat.ST_SIZE] != size:
            sys.stderr.write('%s: Invalid DBC File size, expected %u, got %u\n' % (os.path.abspath(self._fname), size, statinfo[stat.ST_SIZE]))
            f.close()
            sys.exit(1)
        
        # Make sure our data format is correct size
        if self._class and self._class.data_size() != self._record_size:
            sys.stderr.write('%s: DBC record size mismatch, expected %u (data type), got %u (dbc file)\n' % (
                os.path.abspath(self._fname), self._record_size, self._class.data_size()))
            f.close()
            sys.exit(1)

        self._dbc_file = f

        old_pos = self._dbc_file.tell()

        # String block caching to avoid seeking all over the place
        if self._string_block_size > 1:
            self._dbc_file.seek(20 + self._records * self._record_size)
            self._string_block = self._dbc_file.read(self._string_block_size)
            self._dbc_file.seek(old_pos)
        else:
            self._string_block = None

        # Figure out DBC max identifier, as it's the first uint32 field in every dbc record
        if self._records > 0:
            self._dbc_file.seek(20 + (self._records - 1) * self._record_size)
            self._last_id = struct.unpack('I', self._dbc_file.read(4))[0]
            self._dbc_file.seek(old_pos)

        return self

    def get_string_block(self, offset):
        if not self._dbc_file and not self.open_dbc():
            return ''

        if not self._string_block or offset > self._string_block_size:
            return ''
        
        return self._string_block[offset:self._string_block.find('\0', offset)]

    def next_record(self):
        if not self._dbc_file and not self.open_dbc():
            return None

        if self._last_record_id == self._last_id:
            return None

        raw_record = self._dbc_file.read(self._record_size)
        if len(raw_record) != self._record_size:
            return None

        parsed_record = self._class(self, raw_record)
        parsed_record.parse()

        self._last_record_id = parsed_record.id

        return parsed_record

class RawDBCParser(DBCParser):
    def __init__(self, options, fname):
        DBCParser.__init__(self, options, fname)

    def open_dbc(self):
        if self._dbc_file:
            return self
            
        if not os.access(os.path.abspath(self._fname), os.R_OK):
            sys.stderr.write('%s: Not found\n' % os.path.abspath(self._fname))
            sys.exit(1)

        f = io.open(os.path.abspath(self._fname), mode = 'rb')

        if f.read(len(DBC_HDR)) != DBC_HDR:
            sys.stderr.write('%s: Not a World of Warcraft DBC File\n' % os.path.abspath(self._fname))
            f.close()
            sys.exit(1)

        try:
            records, fields, record_size, string_block_size = struct.unpack('IIII', f.read(16))
        except:
            sys.stderr.write('%s: Invalid DBC header format\n' % os.path.abspath(self._fname))
            f.close()
            sys.exit(1)

        self._records           = records
        self._fields            = fields
        self._record_size       = record_size
        self._string_block_size = string_block_size
        size                    = 20 + records * record_size + string_block_size
        statinfo                = os.fstat(f.fileno())
        
        # Sanity check file size
        if statinfo[stat.ST_SIZE] != size:
            sys.stderr.write('%s: Invalid DBC File size, expected %u, got %u\n' % (os.path.abspath(self._fname), size, statinfo[stat.ST_SIZE]))
            f.close()
            sys.exit(1)
        
        self._dbc_file = f

        old_pos = self._dbc_file.tell()

        # String block caching to avoid seeking all over the place
        if self._string_block_size > 1:
            self._dbc_file.seek(20 + self._records * self._record_size)
            self._string_block = self._dbc_file.read(self._string_block_size)
            self._dbc_file.seek(old_pos)
        else:
            self._string_block = None

        # Figure out DBC max identifier, as it's the first uint32 field in every dbc record
        self._dbc_file.seek(20 + (self._records - 1) * self._record_size)
        self._last_id = struct.unpack('I', self._dbc_file.read(4))[0]
        self._dbc_file.seek(old_pos)

        return self

    def next_record(self):
        if not self._dbc_file and not self.open_dbc():
            return None

        if self._last_record_id == self._last_id:
            return None

        raw_record = self._dbc_file.read(self._record_size)
        if len(raw_record) != self._record_size:
            return None

        self._last_record_id = struct.unpack('I', raw_record[:4])[0]

        return raw_record
