import os, io, struct, stat, sys
import data

DBC_HDR = r'WDBC'
DB2_HDR = r'WDB2'
WC2_HDR = r'WCH2'

class DBCParser(object):
    def __init__(self, options, fname):
        object.__init__(self)
        
        self._fname          = fname
        self._dbc_file       = None
        self._options        = options
        self._last_record_id = 0
        self._class          = None
        
    def name(self):
        return os.path.basename(self._fname).split('.')[0].replace('-', '_').lower()

    def find(self, id):
        original_offset = self._dbc_file.tell()

        raw_data = self.seek(0, self._records, id)

        self._dbc_file.seek(original_offset)

        return self._class(self, raw_data)

    def seek(self, begin, end, value):
        if begin > end:
            return None

        self._dbc_file.seek(self._hdr_size + begin * self._record_size + ((end - begin) / 2) * self._record_size)
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
        is_db2 = False
        is_adb = False
        hdr_size = 20
        f = None
        timestamp = 0

        if self._dbc_file:
            return self

        if os.access(os.path.abspath(self._fname), os.R_OK):
            f = io.open(os.path.abspath(self._fname), mode = 'rb')
        elif os.access(os.path.abspath(self._fname + '.db2'), os.R_OK):
            f = io.open(os.path.abspath(self._fname + '.db2'), mode = 'rb')
        elif os.access(os.path.abspath(self._fname + '.dbc'), os.R_OK):
            f = io.open(os.path.abspath(self._fname + '.dbc'), mode = 'rb')
        else:
            sys.stderr.write('%s: Not found\n' % os.path.abspath(self._fname))
            sys.exit(1)

        hdr = f.read(4)

        if hdr == DB2_HDR:
            is_db2 = True
            hdr_size = 48

        if hdr == WC2_HDR: 
            is_adb = True
            hdr_size = 48

        if hdr != DBC_HDR and hdr != DB2_HDR and hdr != WC2_HDR:
            sys.stderr.write('%s: Not a World of Warcraft DBC/DB2/WCH2 File\n' % os.path.abspath(self._fname))
            f.close()
            sys.exit(1)

        first_id = 0
        last_id = 0
        try:
            records, fields, record_size, string_block_size = struct.unpack('IIII', f.read(16))
            if is_db2 or is_adb:
                table_hash, build, timestamp = struct.unpack('III', f.read(12))
                if build > 12880:
                    first_id, last_id, locale, unk_5 = struct.unpack('IIII', f.read(16))
            
                    # Skip index table before real data starts
                    if last_id != 0:
                        f.seek( ( last_id - first_id + 1 ) * 4 + ( last_id - first_id + 1 ) * 2, os.SEEK_CUR )
                        hdr_size += ( last_id - first_id + 1 ) * 4 + ( last_id - first_id + 1 ) * 2
                
                # print hex(hdr_size), table_hash, build, unk_1, first_id, last_id, locale, unk_5
            
            # Cache files need some field mangling
            if is_adb:
                # Presume adb fields are all 4 bytes, as "fields" no longer always carries the amount 
                # of fields in the record
                fields = record_size / 4
                first_id = 0
                last_id = 0
        except:
            sys.stderr.write('%s: Invalid DBC/DB2/WCH2 header format\n' % os.path.abspath(self._fname))
            f.close()
            sys.exit(1)

        self._hdr_size          = hdr_size
        self._records           = records
        self._fields            = fields
        self._record_size       = record_size
        self._string_block_size = string_block_size
        self._hdr_size          = hdr_size
        self._timestamp         = timestamp
        size                    = hdr_size + records * record_size + string_block_size
        statinfo                = os.fstat(f.fileno())
        
        # Figure out a valid class for us from data, now that we know the DBC is sane
        if '%s%d' % ( os.path.basename(self._fname).split('.')[0].replace('-', '_'), self._options.build ) in dir(data):
            self._class = getattr(data, '%s%d' % ( 
                os.path.basename(self._fname.replace('-', '_')).split('.')[0], self._options.build ) )
        
        # Sanity check file size
        if statinfo[stat.ST_SIZE] != size:
            sys.stderr.write('%s: Invalid DBC/DB2 File size, expected %u, got %u\n' % (os.path.abspath(self._fname), size, statinfo[stat.ST_SIZE]))
            f.close()
            sys.exit(1)
        
        # Make sure our data format is correct size
        if self._class and self._class.data_size() != self._record_size:
            sys.stderr.write('%s: DBC/DB2 record size mismatch, expected %u (dbc file), got %u (dbc data type)\n' % (
                os.path.abspath(self._fname), self._record_size, self._class.data_size()))
            f.close()
            sys.exit(1)

        self._dbc_file = f

        old_pos = self._dbc_file.tell()

        # String block caching to avoid seeking all over the place
        if self._string_block_size > 1:
            self._dbc_file.seek(hdr_size + self._records * self._record_size)
            self._string_block = self._dbc_file.read(self._string_block_size)
            self._dbc_file.seek(old_pos)
        else:
            self._string_block = None

        # Figure out DBC max identifier, as it's the first uint32 field in every dbc record
        if ( is_db2 and not last_id ) or ( not is_db2 and not is_adb and self._records > 0 ):
            self._dbc_file.seek(hdr_size + (self._records - 1) * self._record_size)
            self._last_id = struct.unpack('I', self._dbc_file.read(4))[0]
            self._dbc_file.seek(old_pos)
        else:
            self._last_id = last_id
        
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
        
        if self._last_id > 0 and self._last_record_id == self._last_id:
            return None
        # No last id, go by file size (needed for cache files)
        elif self._dbc_file.tell() == self._hdr_size + self._records * self._record_size:
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
        print self._last_id

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
