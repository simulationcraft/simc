import os, io, struct, sys, logging, math

import dbc.fmt

_BASE_HEADER = struct.Struct('IIII')
_DB_HEADER_1 = struct.Struct('III')
_DB_HEADER_2 = struct.Struct('IIII')
_ID          = struct.Struct('I')
_CLONE       = struct.Struct('II')
_ITEMRECORD  = struct.Struct('IH')

# WDB5 field data, size (32 - size) // 8, offset tuples
_FIELD_DATA  = struct.Struct('HH')

X_ID_BLOCK = 0x04
X_OFFSET_MAP = 0x01

class DBCParserBase:
    def is_magic(self):
        raise Exception()

    def __init__(self, options, fname):
        self.file_name = None
        self.options = options

        # Data format storage
        self.fmt = dbc.fmt.DBFormat(options)

        # Data stuff
        self.data = None
        self.parse_offset = 0
        self.data_offset = 0
        self.string_block_offset = 0

        # Bytes to set of byte fields handling
        self.field_data = []
        self.record_parser = None

        # Iteration
        self.record_id = 0

        # Searching
        self.id_data = None

        # Parsing
        self.unpackers = []

        self.id_format_str = None

        # See that file exists already
        normalized_path = os.path.abspath(fname)
        for i in ['', '.db2', '.dbc']:
            if os.access(normalized_path + i, os.R_OK):
                self.file_name = normalized_path + i
                logging.debug('WDB file found at %s', self.file_name)

        if not self.file_name:
            logging.error('No WDB file found based on "%s"', fname)
            sys.exit(1)

    def id_format(self):
        if self.id_format_str:
            return self.id_format_str

        # Adjust the size of the id formatter so we get consistent length
        # id field where we use it, and don't have to guess on the length
        # in the format file.
        n_digits = int(math.log10(self.last_id) + 1)
        self.id_format_str = '%%%uu' % n_digits
        return self.id_format_str

    # Sanitize data, blizzard started using dynamic width ints in WDB5, so
    # 3-byte ints have to be expanded to 4 bytes to parse them properly (with
    # struct module)
    def build_parser(self):
        format_str = '<'

        data_fmt = field_names = None
        if not self.options.raw:
            data_fmt = self.fmt.types(self.class_name())
            field_names = self.fmt.fields(self.class_name())
        field_idx = 0

        for field_data_idx in range(0, len(self.field_data)):
            field_data = self.field_data[field_data_idx]
            type_idx = min(field_idx, data_fmt and len(data_fmt) - 1 or field_idx)
            field_size = field_data[1]
            for sub_idx in range(0, field_data[2]):
                if field_size == 1:
                    format_str += data_fmt and data_fmt[type_idx] or 'b'
                elif field_size == 2:
                    format_str += data_fmt and data_fmt[type_idx] or 'h'
                elif field_size >= 3:
                    format_str += data_fmt and data_fmt[type_idx].replace('S', 'I') or 'i'
                field_idx += 1

                if field_size == 3:
                    if not self.options.raw:
                        logging.debug('Unpacker has a 3-byte field (name=%s pos=%d): terminating (%s) and beginning a new unpacker',
                            field_names[type_idx], field_idx, format_str)
                    else:
                        logging.debug('Unpacker has a 3-byte field (pos=%d): terminating (%s) and beginning a new unpacker',
                            field_idx, format_str)
                    self.unpackers.append((True, struct.Struct(format_str)))
                    format_str = '<'

        if len(format_str) > 1:
            self.unpackers.append((self.field_data[-1][1] == 3, struct.Struct(format_str)))

        logging.debug('Unpacking plan: %s', ', '.join(['%s (len=%d)' % (u.format.decode('utf-8'), u.size) for _, u in self.unpackers]))
        if len(self.unpackers) == 1:
            self.record_parser = lambda ro, rs: self.unpackers[0][1].unpack_from(self.data, ro)
        else:
            self.record_parser = self.__do_parse

    def __do_parse(self, record_offset, record_size):
        size_left = record_size
        unpacker_offset = 0
        full_data = []
        for int24, unpacker in self.unpackers:
            full_data += unpacker.unpack_from(self.data, record_offset + unpacker_offset)
            if int24:
                full_data[-1] &= 0xFFFFFF
                size_left += 1

            size_left -= unpacker.size
            unpacker_offset = record_size - size_left
        return full_data

    def n_expanded_fields(self):
        return sum([ fd[2] for fd in self.field_data ])

    # Can we search for data? This is only true, if we know where the ID in the data resides
    def searchable(self):
        return False

    def raw_outputtable(self):
        return False

    def full_name(self):
        return os.path.basename(self.file_name)

    def file_name(self):
        return os.path.basename(self.file_name).split('.')[0]

    def class_name(self):
        return os.path.basename(self.file_name).split('.')[0]

    def name(self):
        return os.path.basename(self.file_name).split('.')[0].replace('-', '_').lower()

    # Real record size is fields padded to powers of two
    def parsed_record_size(self):
        return self.record_size

    def n_records(self):
        return self.records

    def n_fields(self):
        return len(self.field_data)

    def n_records_left(self):
        return 0

    def parse_header(self):
        self.magic = self.data[:4]

        if not self.is_magic():
            logging.error('Invalid data file format %s', self.data[:4].decode('utf-8'))
            return False

        self.parse_offset += 4
        self.records, self.fields, self.record_size, self.string_block_size = _BASE_HEADER.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _BASE_HEADER.size

        return True

    def build_field_data(self):
        types = self.fmt.types(self.class_name())
        field_offset = 0
        for t in types:
            if t in ['I', 'i', 'f', 'S']:
                self.field_data.append((field_offset, 4, 1))
            elif t in ['H', 'h']:
                self.field_data.append((field_offset, 2, 1))
            elif t in ['B', 'b']:
                self.field_data.append((field_offset, 1, 1))

            field_offset += self.field_data[-1][1]

    def validate_data_model(self):
        unpacker = None
        # Sanity check the data
        try:
            unpacker = self.fmt.parser(self.class_name())
        except:
            # We did not find a parser for the file, so then we need to figure
            # out if we can output it anyhow in some reduced form, since some
            # DBC versions are like that
            if not self.raw_outputtable():
                logging.error("Unable to parse %s, no formatting data found", self.class_name())
                return False

        # Build auxilary structures to parse raw data out of the DBC file
        self.build_field_data()
        self.build_parser()

        if not unpacker:
            return True

        # Check that at the end of the day, we have a sensible record length
        if not self.raw_outputtable() and unpacker.size > self.parsed_record_size():
            logging.error("Record size mismatch for %s, expected %u, format has %u",
                    self.class_name(), self.record_size, unpacker.size)
            return False

        # Skip the native specifier
        if not self.raw_outputtable() and len(unpacker.format[1:]) > self.n_fields():
            logging.error("Record field count mismatch for %s, expected %u, format has %u",
                    self.class_name(), self.n_fields(), len(unpacker.format[1:]))
            return False

        # Figure out the position of the id column. This may be none if WDB4/5
        # and id_block_offset > 0
        if not self.options.raw:
            fields = self.fmt.fields(self.class_name())
            for idx in range(0, len(fields)):
                if fields[idx] == 'id':
                    self.id_data = self.field_data[idx]
                    break

        return True

    def open(self):
        if self.data:
            return True

        f = io.open(self.file_name, mode = 'rb')
        self.data = f.read()
        f.close()

        if not self.parse_header():
            return False

        if not self.validate_data_model():
            return False

        # After headers begins data, always
        self.data_offset = self.parse_offset

        return True

    def get_string(self, offset):
        end_offset = self.data.find(b'\x00', self.string_block_offset + offset)

        if end_offset == -1:
            return None

        return self.data[self.string_block_offset + offset:end_offset].decode('utf-8')

    def find_record_offset(self, id_):
        unpacker = None
        if self.id_data[1] == 1:
            unpacker = struct.Struct('B')
        elif self.id_data[1] == 2:
            unpacker = struct.Struct('H')
        elif self.id_data[1] >= 3:
            unpacker = struct.Struct('I')

        for record_id in range(0, self.n_records()):
            offset = self.data_offset + self.record_size * record_id + self.id_data[0]

            dbc_id = unpacker.unpack_from(self.data, offset)[0]
            # Hack to fix 3 byte fields, need to zero out the high byte
            if self.id_data[1] == 3:
                dbc_id &= 0x00FFFFFF

            if dbc_id == id_:
                return -1, self.data_offset + self.record_size * record_id, self.record_size

        return -1, 0, 0

    # Returns dbc_id (always 0 for base), record offset into file
    def get_next_record_info(self):
        return -1, self.data_offset + self.record_id * self.record_size, self.record_size

    def next_record(self):
        if not self.n_records_left():
            return None

        dbc_id, record_offset, record_size = self.get_next_record_info()
        self.record_id += 1
        return dbc_id, self.record_parser(record_offset, record_size)

    def find(self, id_):
        dbc_id, record_offset, record_size = self.find_record_offset(id_)

        if record_offset > 0:
            return dbc_id, self.record_parser(record_offset, record_size)
        else:
            return 0, tuple()

class InlineStringRecordParser:
    # Presume that string fields are always bunched up togeher
    def __init__(self, parser):
        self.parser = parser
        self.unpackers = []
        self.string_field_offset = 0
        self.n_string_fields = 0
        try:
            self.types = self.parser.fmt.types(self.parser.class_name())
            self.n_string_fields = sum([field == 'S' for field in self.types])
        except:
            logging.error('Inline-string based file %s requires formatting information', self.parser.class_name())
            sys.exit(1)

        data_fmt = field_names = None
        if not self.parser.options.raw:
            data_fmt = self.parser.fmt.types(self.parser.class_name())
            field_names = self.parser.fmt.fields(self.parser.class_name())
        # Need to build a two-split custom parser here
        format_str = '<'
        format_idx = 0
        for field_idx in range(0, len(self.parser.field_data)):
            field_data = self.parser.field_data[field_idx]
            type_idx = min(format_idx, data_fmt and len(data_fmt) - 1 or format_idx)
            field_size = field_data[1]
            for sub_idx in range(0, field_data[2]):
                if data_fmt and data_fmt[type_idx] == 'S':
                    logging.debug('Unpacker has a inlined string field (name=%s pos=%d): terminating (%s), skipping all consecutive strings, and beginning a new unpacker',
                        field_names[type_idx], field_idx, format_str)
                    self.unpackers.append((False, struct.Struct(format_str)))
                    self.string_field_offset = self.unpackers[-1][1].size
                    format_str = '<'
                    format_idx += 1

                    while data_fmt[type_idx + 1] == 'S':
                        format_idx += 1
                        type_idx += 1
                        field_idx += 1
                    continue

                if field_size == 1:
                    format_str += data_fmt and data_fmt[type_idx] or 'b'
                elif field_size == 2:
                    format_str += data_fmt and data_fmt[type_idx] or 'h'
                elif field_size >= 3:
                    format_str += data_fmt and data_fmt[type_idx].replace('S', 'I') or 'i'
                format_idx += 1

                if field_size == 3:
                    if not self.options.raw:
                        logging.debug('Unpacker has a 3-byte field (name=%s pos=%d): terminating (%s) and beginning a new unpacker',
                            field_names[type_idx], field_idx, format_str)
                    else:
                        logging.debug('Unpacker has a 3-byte field (pos=%d): terminating (%s) and beginning a new unpacker',
                            field_idx, format_str)
                    unpackers.append((True, struct.Struct(format_str)))
                    format_str = '<'

        if len(format_str) > 1:
            self.unpackers.append((self.parser.field_data[-1][1] == 3, struct.Struct(format_str)))

        logging.debug('Unpacking plan: %s', ', '.join(['%s (len=%d)' % (u.format.decode('utf-8'), u.size) for _, u in self.unpackers]))

    def __call__(self, offset, size):
        full_data = []
        field_offset = 0
        parsed_string_fields = 0
        for int24, unpacker in self.unpackers:
            full_data += unpacker.unpack_from(self.parser.data, offset + field_offset)
            field_offset += unpacker.size

            # String fields begin
            if field_offset == self.string_field_offset:
                while self.parser.data[offset + field_offset] != 0 and parsed_string_fields < self.n_string_fields:
                    #start, end = self.__getstring(self.parser.data, offset + field_offset, offset + size)
                    end = self.parser.data.find(b'\x00', offset + field_offset, offset + size)
                    full_data.append(offset + field_offset)
                    if parsed_string_fields < self.n_string_fields - 1:
                        field_offset = end - offset + 4
                    else:
                        field_offset = end - offset + 1
                    parsed_string_fields += 1

                # If some of the consecutive string fields are empty, append
                # empty fields so we have enough fields in total
                # (n_string_fields)
                full_data += [0] * (self.n_string_fields - parsed_string_fields)
                if parsed_string_fields < self.n_string_fields:
                    field_offset += 1

            if int24:
                full_data[-1] &= 0xFFFFFF
                size_left += 1

        return full_data

class LegionWDBParser(DBCParserBase):
    def __init__(self, options, fname):
        super().__init__(options, fname)

        self.id_block_offset = 0
        self.clone_block_offset = 0
        self.offset_map_offset = 0

        self.id_table = []

    def n_cloned_records(self):
        return self.clone_segment_size // _CLONE.size

    def n_records_left(self):
        return self.n_records() - self.record_id

    def n_records(self):
        if self.id_block_offset:
            return len(self.id_table)
        else:
            return self.records

    # Inline strings need some (very heavy) custom parsing
    def build_parser(self):
        if self.flags & X_OFFSET_MAP:
            self.record_parser = InlineStringRecordParser(self)
        else:
            super().build_parser()

    # Can search through WDB4/5 files if there's an id block, or if we can find
    # an ID from the formatted data
    def searchable(self):
        if self.options.raw:
            return self.id_block_offset > 0
        else:
            return True

    def find_record_offset(self, id_):
        if self.flags & X_ID_BLOCK:
            for record_id in range(0, self.records):
                if self.id_table[record_id][0] == id_:
                    return self.id_table[record_id]
            return -1, 0, 0
        else:
            return super().find_record_offset(id_)

    def __build_id_table(self):
        if self.id_block_offset == 0:
            return

        idtable = []
        indexdict = {}

        # Process ID block
        unpacker = struct.Struct('%dI' % self.records)
        record_id = 0
        for dbc_id in unpacker.unpack_from(self.data, self.id_block_offset):
            data_offset = 0
            size = self.record_size

            # If there is an offset map, the correct data offset and record
            # size needs to be fetched from the offset map. The offset map
            # contains sparse entries (i.e., it has last_id-first_id entries,
            # some of which are zeros if there is no dbc id for a given item.
            if self.flags & X_OFFSET_MAP:
                record_index = dbc_id - self.first_id
                record_data_offset = self.offset_map_offset + record_index * _ITEMRECORD.size
                data_offset, size = _ITEMRECORD.unpack_from(self.data, record_data_offset)
            else:
                data_offset = self.data_offset + record_id * self.record_size

            idtable.append((dbc_id, data_offset, size))
            indexdict[dbc_id] = (data_offset, size)
            record_id += 1

        # Process clones
        for clone_id in range(0, self.n_cloned_records()):
            clone_offset = self.clone_block_offset + clone_id * _CLONE.size
            target_id, source_id = _CLONE.unpack_from(self.data, clone_offset)
            if source_id not in indexdict:
                continue

            idtable.append((target_id, indexdict[source_id][0], indexdict[source_id][1]))

        self.id_table = idtable

    def open(self):
        if not super().open():
            return False

        self.__build_id_table()

        logging.debug('Opened %s' % self.file_name)
        return True

    def get_next_record_info(self):
        if self.id_block_offset > 0:
            return self.id_table[self.record_id]
        else:
            return super().get_next_record_info()

class WDB4Parser(LegionWDBParser):
    def is_magic(self): return self.magic == b'WDB4'

    # TODO: Move this to a defined set of (field, type, attribute) triples in DBCParser
    def parse_header(self):
        if not super().parse_header():
            return False

        self.table_hash, self.build, self.timestamp = _DB_HEADER_1.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _DB_HEADER_1.size

        self.first_id, self.last_id, self.locale, self.clone_segment_size = _DB_HEADER_2.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _DB_HEADER_2.size

        self.flags = struct.unpack_from('I', self.data, self.parse_offset)[0]
        self.parse_offset += 4

        # Setup offsets into file, first string block
        if self.string_block_size > 2:
            self.string_block_offset = self.parse_offset + self.records * self.record_size

        # Has ID block
        if self.flags & X_ID_BLOCK:
            self.id_block_offset = self.parse_offset + self.records * self.record_size + self.string_block_size

        # Offset map contains offset into file, record size entries in a sparse structure
        if self.flags & X_OFFSET_MAP:
            self.offset_map_offset = self.string_block_size
            self.string_block_offset = 0
            if self.flags & X_ID_BLOCK:
                self.id_block_offset = self.string_block_size + ((self.last_id - self.first_id) + 1) * _ITEMRECORD.size

        # Has clone block
        if self.clone_segment_size > 0:
            self.clone_block_offset = self.id_block_offset + self.records * _ID.size

        return True

    def __str__(self):
        s = '%s::%s(byte_size=%u, build=%u, timestamp=%u, locale=%#.8x, records=%u+%u, fields=%u, record_size=%u, string_block_size=%u, clone_segment_size=%u, first_id=%u, last_id=%u, data_offset=%u, sblock_offset=%u, id_offset=%u, clone_offset=%u, unk=%u, hash=%#.8x)' % (
                self.full_name(), self.magic.decode('ascii'), len(self.data), self.build, self.timestamp, self.locale,
                self.records, self.n_cloned_records(), self.fields, self.record_size,
                self.string_block_size, self.clone_segment_size, self.first_id,
                self.last_id, self.data_offset, self.string_block_offset, self.id_block_offset,
                self.clone_block_offset, self.flags, self.table_hash)

        return s

class WDB5Parser(LegionWDBParser):
    def is_magic(self): return self.magic == b'WDB5'

    # Parses header field information to field_data. Note that the tail end has
    # to be guessed because we don't know whether there's padding in the file
    # or whether the last field is an array.
    def build_field_data(self):
        prev_field = None
        for field_idx in range(0, self.fields):
            raw_size, record_offset = _FIELD_DATA.unpack_from(self.data, self.parse_offset)
            type_size = (32 - raw_size) // 8

            self.parse_offset += _FIELD_DATA.size
            distance = 0
            if prev_field:
                distance = record_offset - prev_field[0]
                n_fields = distance // prev_field[1]
                self.field_data[-1][2] = n_fields

            self.field_data.append([record_offset, type_size, 0])

            prev_field = (record_offset, type_size)

        bytes_left = self.record_size - self.field_data[-1][0]
        while bytes_left >= self.field_data[-1][1]:
            self.field_data[-1][2] += 1
            bytes_left -= self.field_data[-1][1]

        logging.debug('Generated field data for %d fields (header is %d)', len(self.field_data), self.fields)

        return True

    # WDB5 allows us to build an automatic decoder for the data, because the
    # field widths are included in the header. The decoder cannot distinguish
    # between signed/unsigned nor floats, but it will still give same data for
    # the same byte values
    def build_decoder(self):
        fields = ['=',]
        for field_data in self.field_data:
            if field_data[1] > 2:
                fields.append('i')
            elif field_data[1] == 2:
                fields.append('h')
            elif field_data[1] == 1:
                fields.append('b')
        return struct.Struct(''.join(fields))

    # TODO: Move this to a defined set of (field, type, attribute) triples in DBCParser
    def parse_header(self):
        if not super().parse_header():
            return False

        self.table_hash, self.build, self.first_id = _DB_HEADER_1.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _DB_HEADER_1.size

        self.last_id, self.locale, self.clone_segment_size, self.flags = _DB_HEADER_2.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _DB_HEADER_2.size

        # Setup offsets into file, first string block, skip field data information of WDB5 files
        if self.string_block_size > 2:
            self.string_block_offset = self.parse_offset + self.records * self.record_size
            self.string_block_offset += self.fields * 4

        # Has ID block, note that ID-block needs to skip the field data information on WDB5 files
        if self.flags & X_ID_BLOCK:
            self.id_block_offset = self.parse_offset + self.records * self.record_size + self.string_block_size
            self.id_block_offset += self.fields * 4

        # Offset map contains offset into file, record size entries in a sparse structure
        if self.flags & X_OFFSET_MAP:
            self.offset_map_offset = self.string_block_size
            self.string_block_offset = 0
            if self.flags & X_ID_BLOCK:
                self.id_block_offset = self.string_block_size + ((self.last_id - self.first_id) + 1) * _ITEMRECORD.size

        # Has clone block
        if self.clone_segment_size > 0:
            self.clone_block_offset = self.id_block_offset + self.records * _ID.size

        return True

    # WDB5 can always output some human readable data, even if the field types
    # are not correct, but only do this if --raw is enabled
    def raw_outputtable(self):
        return self.options.raw

    def n_fields(self):
        return sum([fd[2] for fd in self.field_data])

    # This is the padded record size, meaning 3 byte fields are padded to 4 bytes
    def parsed_record_size(self):
        return sum([(fd[1] == 3 and 4 or fd[1]) * fd[2] for fd in self.field_data])

    def validate_data_model(self):
        if not super().validate_data_model():
            return False

        return True

    def __str__(self):
        s = '%s::%s(byte_size=%u, build=%u, locale=%#.8x, records=%u+%u, fields=%u, record_size=%u, string_block_size=%u, clone_segment_size=%u, first_id=%u, last_id=%u, data_offset=%u, sblock_offset=%u, id_offset=%u, clone_offset=%u, unk=%u, hash=%#.8x)' % (
                self.full_name(), self.magic.decode('ascii'), len(self.data), self.build, self.locale,
                self.records, self.n_cloned_records(), self.fields, self.record_size,
                self.string_block_size, self.clone_segment_size, self.first_id,
                self.last_id, self.data_offset, self.string_block_offset, self.id_block_offset,
                self.clone_block_offset, self.flags, self.table_hash)

        fields = []
        for i in range(0, len(self.field_data)):
            field_data = self.field_data[i]
            fields.append('#%d: %s%s@%d' % (
                len(fields) + 1, 'int%d' % (field_data[1] * 8), field_data[2] > 1 and ('[%d]' % field_data[2]) or '',
                field_data[0]
            ))

        if len(self.field_data):
            s += '\nField data: %s' % ', '.join(fields)

        return s

