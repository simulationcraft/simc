import os, io, struct, stat, sys, mmap, math
import dbc.data

_BASE_HEADER = struct.Struct('IIII')
_DB_HEADER_1 = struct.Struct('III')
_DB_HEADER_2 = struct.Struct('IIII')
_ID          = struct.Struct('I')
_CLONE       = struct.Struct('II')

class DBCParser(object):

    def __init__(self, options, fname):
        self._fname          = fname
        self._options        = options
        self._last_record_id = 0
        self._data = None
        self._class = None
        self._idtable = []

        # Figure out a valid class for us from data, now that we know the DBC is sane
        table_name = None
        if len(self._options.as_dbc) > 0:
            table_name = self._options.as_dbc
        else:
            table_name = os.path.basename(self._fname).split('.')[0].replace('-', '_')

        if not self._options.raw and self._options.type != 'header' and '%s' % table_name in dir(dbc.data):
            self._class = getattr(dbc.data, '%s' % table_name)

    # DBC IDs in an idblock + potential ID cloning
    def _build_idtable(self):
        if self._id_offset == 0:
            return

        idtable = []
        indexdict = {}
        # Process ID block
        for record_id in range(0, self._records):
            record_offset = self._id_offset + record_id * _ID.size
            dbc_id = _ID.unpack_from(self._data, record_offset)[0]
            idtable.append((dbc_id, record_id))
            indexdict[dbc_id] = record_id

        # Process clones
        if self._clone_segment_offset > 0:
            for clone_id in range(0, self.n_cloned_records()):
                clone_offset = self._clone_segment_offset + clone_id * _CLONE.size
                target_id, source_id = _CLONE.unpack_from(self._data, clone_offset)
                if source_id not in indexdict:
                    continue

                idtable.append((target_id, indexdict[source_id]))

        self._idtable = idtable

    def full_name(self):
        return os.path.basename(self._fname)

    def file_name(self):
        return os.path.basename(self._fname).split('.')[0]

    def name(self):
        return os.path.basename(self._fname).split('.')[0].replace('-', '_').lower()

    def count_pad_bytes(self):
        max_nonzero_byte = 3
        n = 0

        while True:
            if n == self._records:
                return max_nonzero_byte

            dbc_id = record_offset = 0
            if self._id_offset > 0:
                dbc_id, record_id = self._idtable[n]
                record_offset = self._data_offset + record_id * self._record_size
            else:
                record_offset = self._data_offset + n * self._record_size

            for i in range(max_nonzero_byte, 0, -1):
                if self._data[record_offset + self._record_size - i] != 0:
                    max_nonzero_byte = i - 1
                    if i == 1:
                        return 0

            n += 1

    def find(self, id):
        dbc_id = record_offset = 0
        if self._id_offset > 0:
            for dbc_id, record_id in self._idtable:
                if dbc_id != id:
                    continue

                record_offset = self._data_offset + record_id * self._record_size
                break
        else:
            for record_id in range(0, self._records):
                record_offset = self._data_offset + record_id * self._record_size
                if _ID.unpack_from(self._data, record_offset)[0] == id:
                    break

        return self._class(self, self._data[record_offset:record_offset + self._record_size], dbc_id, record_offset, self._record_size)

    def open_dbc(self):
        if self._data:
            return self

        f = None

        if os.access(os.path.abspath(self._fname), os.R_OK):
            f = io.open(os.path.abspath(self._fname), mode = 'rb')
        elif os.access(os.path.abspath(self._fname + '.db2'), os.R_OK):
            f = io.open(os.path.abspath(self._fname + '.db2'), mode = 'rb')
        elif os.access(os.path.abspath(self._fname + '.dbc'), os.R_OK):
            f = io.open(os.path.abspath(self._fname + '.dbc'), mode = 'rb')
        else:
            sys.stderr.write('%s: Not found\n' % os.path.abspath(self._fname))
            sys.exit(1)

        self._f = f
        self._data = mmap.mmap(self._f.fileno(), 0, access = mmap.ACCESS_READ)

        self._magic = self._data[:4]
        offset = len(self._magic)

        if self._magic not in [b'WDBC', b'WDB2', b'WDB3', b'WDB4', b'WCH4']:
            self._data.close()
            self._f.close()
            sys.stderr.write('Invalid file format, got %s.\n' % self._magic)
            sys.exit(1)

        self._records, self._fields, self._record_size, self._string_block_size = _BASE_HEADER.unpack_from(self._data, offset)
        offset += _BASE_HEADER.size

        #if self._class and self._record_size != self._class._parser.size:
        #    raise Exception('Invalid data format size in %s, record_size=%u, dataformat_size=%u' % (
        #        self._fname, self._record_size, self._class._parser.size))

        if self._magic in [b'WDB2', b'WDB3', b'WCH4', b'WDB4']:
            self._table_hash, self._build, self._timestamp = _DB_HEADER_1.unpack_from(self._data, offset)
            offset += _DB_HEADER_1.size

            self._first_id, self._last_id, self._locale, self._clone_segment_size = _DB_HEADER_2.unpack_from(self._data, offset)
            offset += _DB_HEADER_2.size

            # Another unknown field, yay. TODO: Where is this used? Item-sparse has value 5, the
            # rest of the db2 files have value of 4.
            if self._magic == b'WDB4':
                self._unk = struct.unpack_from('I', self._data, offset)[0]
                offset += 4
            else:
                self._unk = 0

        # Figure out first/last ids manually
        else:
            self._table_hash = self._build = self._timestamp = self._locale = self._clone_segment_size = 0
            self._first_id = struct.unpack_from('I', self._data, offset)[0]
            self._last_id = struct.unpack_from('I', self._data, offset + (self._records - 1) * self._record_size)[0]
            self._unk = 0

        # Setup offsets
        self._data_offset = offset
        if self._string_block_size > 2:
            self._sb_offset = offset + self._records * self._record_size
        else:
            self._sb_offset = 0

        if self._data_offset + self._records * self._record_size + self._string_block_size < len(self._data):
            self._id_offset = self._data_offset + self._records * self._record_size + self._string_block_size
        else:
            self._id_offset = 0

        if self._clone_segment_size > 0:
            self._clone_segment_offset = self._id_offset + self._records * _ID.size
        else:
            self._clone_segment_size = 0
            self._clone_segment_offset = 0

        self._build_idtable()

        if not self._class:
            self._class = dbc.data.proxy_class(self.file_name(), self._fields, self._record_size, self._id_offset > 0,
                    0)
                    #self._magic in [ b'WDB3', b'WDB4' ] and self.count_pad_bytes() or 0)

        if not self._options.raw and hasattr(self._class, '_ff') and self._records > 0:
            self._class._ff = (self.compute_id_output_format(),) + self._class._ff[1:]

        # Make sure our data format is correct size
        if self._class and self._class and self._class.data_size() != self._record_size:
            sys.stderr.write('%s: DBC/DB2 record size mismatch, expected %u (dbc file), got %u (dbc data type)\n' % (
                os.path.abspath(self._fname), self._record_size, self._class.data_size()))
            self._data.close()
            self._f.close()
            sys.exit(1)

        return self

    def n_records(self):
        if self._idtable:
            return len(self._idtable)
        else:
            return self._records

    def n_cloned_records(self):
        return self._clone_segment_size // 8

    def get_string_block(self, offset):
        end_offset = self._data.find(b'\x00', self._sb_offset + offset)

        if end_offset == -1:
            return None

        return self._data[self._sb_offset + offset:end_offset].decode('utf-8')

    def compute_id_output_format(self):
        n_digits = int(math.log10(self._last_id) + 1)

        return '%%%uu' % n_digits

    def next_record(self):
        if not self._data and not self.open_dbc():
            return None

        if self._records == 0 or self._last_record_id == self.n_records():
            return None

        dbc_id = record_offset = 0
        if self._id_offset > 0:
            dbc_id, record_id = self._idtable[self._last_record_id]
            record_offset = self._data_offset + record_id * self._record_size
        else:
            record_offset = self._data_offset + self._last_record_id * self._record_size

        parsed_record = self._class(self, self._data[record_offset:record_offset + self._record_size], dbc_id, record_offset, self._record_size)
        self._last_record_id += 1

        return parsed_record

    def __str__(self):
        return '%s::%s(byte_size=%u, build=%u, timestamp=%u, locale=%#.8x, records=%u+%u, fields=%u, record_size=%u, string_block_size=%u, clone_segment_size=%u, first_id=%u, last_id=%u, data_offset=%u, sblock_offset=%u, id_offset=%u, clone_offset=%u, unk=%u, hash=%#.8x)' % (
                self.full_name(), self._magic.decode('ascii'), len(self._data), self._build, self._timestamp, self._locale, 
                self._records, self.n_cloned_records(), self._fields, self._record_size,
                self._string_block_size, self._clone_segment_size, self._first_id,
                self._last_id, self._data_offset, self._sb_offset, self._id_offset,
                self._clone_segment_offset, self._unk, self._table_hash)

_ITEMRECORD = struct.Struct('IH')

class ItemSparseParser(DBCParser):
    def __init__(self, options, fname):
        DBCParser.__init__(self, options, fname)

        self._class = dbc.data.Item_sparse

    # WDB4 Item-sparse changes things somewhat. Need to build an idtable for it just like any
    # WDB3-based file, but the ID block offset needs to be computed from the end of the file for
    # now, instead of relying on string block offset + size (string block size seems to be used
    # as an end-of-record data marker).
    def _build_idtable(self):
        if self._magic == b'WDB4':
            self._sb_offset = self._string_block_size
            self._id_offset = self._string_block_size + ((self._last_id - self._first_id) + 1) * _ITEMRECORD.size
            self._string_block_size = 0

            # Process ID block, and customize idtable stored data to contain
            # dbc_id, record_offset, record_size triples.
            for record_id in range(0, self._records):
                record_offset = self._id_offset + record_id * _ID.size
                dbc_id = _ID.unpack_from(self._data, record_offset)[0]

                record_index = dbc_id - self._first_id
                record_data_offset = self._sb_offset + record_index * _ITEMRECORD.size
                file_offset, record_size = _ITEMRECORD.unpack_from(self._data, record_data_offset)

                self._idtable.append((dbc_id, file_offset, record_size))
        # WDB3 Item-sparse does nothing of the sort
        else:
            self._sb_offset = 0
            self._id_offset = 0

    def get_string_block(self, offset):
        if offset == 0:
            return 0

        end_offset = self._data.find(b'\x00', offset)

        if end_offset == -1:
            return None

        return self._data[offset:end_offset].decode('utf-8')

    def find(self, dbcid):
        if not self._data and not self.open_dbc():
            return None

        if self._magic == b'WDB3':
            return self.__find_wdb3(dbcid)
        else:
            return self.__find_wdb4(dbcid)

    def __find_wdb3(self, dbcid):
        while self._last_record_id < (self._last_id - self._first_id + 1):
            record_offset, record_bytes = _ITEMRECORD.unpack_from(self._data, self._data_offset + _ITEMRECORD.size * self._last_record_id)
            self._last_record_id += 1
            while record_offset == 0:
                record_offset, record_bytes = _ITEMRECORD.unpack_from(self._data, self._data_offset + _ITEMRECORD.size * self._last_record_id)
                self._last_record_id += 1

            if self._first_id + self._last_record_id - 1 != dbcid:
                continue

            return self._class(self, self._data, self._first_id + self._last_record_id - 1, record_offset, record_bytes)

    def __find_wdb4(self, dbcid):
        for dbc_id, record_offset, record_bytes in self._idtable:
            if dbc_id != dbcid:
                continue
            break

        return self._class(self, self._data, dbc_id, record_offset, record_bytes)

    def next_record(self):
        if not self._data and not self.open_dbc():
            return None

        if self._magic == b'WDB4':
            return self.__next_record_wdb4()
        else:
            return self.__next_record_wdb3()

    def __next_record_wdb3(self):
        if self._records == 0 or self._last_record_id == (self._last_id - self._first_id + 1):
            return None

        record_offset, record_bytes = _ITEMRECORD.unpack_from(self._data, self._data_offset + _ITEMRECORD.size * self._last_record_id)
        self._last_record_id += 1
        while record_offset == 0:
            record_offset, record_bytes = _ITEMRECORD.unpack_from(self._data, self._data_offset + _ITEMRECORD.size * self._last_record_id)
            self._last_record_id += 1

        return self._class(self, self._data, self._first_id + self._last_record_id - 1, record_offset, record_bytes)

    def __next_record_wdb4(self):
        if self._records == 0 or self._last_record_id == self._records:
            return None

        dbc_id, record_offset, record_bytes = self._idtable[self._last_record_id]
        parsed_record = self._class(self, self._data, dbc_id, record_offset, record_bytes)
        self._last_record_id += 1

        return parsed_record

def get_parser(opts, for_file):
    if 'Item-sparse' in for_file:
        return ItemSparseParser(opts, for_file)
    else:
        return DBCParser(opts, for_file)

