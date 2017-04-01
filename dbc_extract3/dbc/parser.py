import os, io, struct, sys, logging, math, re, binascii

import dbc.fmt

_BASE_HEADER = struct.Struct('<IIII')
_DB_HEADER_1 = struct.Struct('<III')
_DB_HEADER_2 = struct.Struct('<IIIHH')
_DB_HEADER_3 = struct.Struct('<II')
_WCH5_HEADER = struct.Struct('<IIIIIII')
_ID          = struct.Struct('<I')
_CLONE       = struct.Struct('<II')
_ITEMRECORD  = struct.Struct('<IH')
_WCH_ITEMRECORD = struct.Struct('<IIH')
_WCH7_BASE_HEADER = struct.Struct('<IIIII')

_WDB6_BLOCK_HEADER = struct.Struct('<IB')

# WDB5 field data, size (32 - size) // 8, offset tuples
_FIELD_DATA = struct.Struct('<HH')

_BYTE = struct.Struct('B')
_SHORT = struct.Struct('<H')
_INT32 = struct.Struct('<I')

X_ID_BLOCK = 0x04
X_OFFSET_MAP = 0x01

def _do_parse(unpackers, data, record_offset, record_size):
    full_data = []
    field_offset = 0
    for i24, unpacker in unpackers:
        full_data += unpacker.unpack_from(data, record_offset + field_offset)
        field_offset += unpacker.size
        if i24:
            field_offset -= 1
            full_data[-1] &= 0xFFFFFF
    return full_data

class DBCParserBase:
    def is_magic(self):
        raise Exception()

    def __init__(self, options, fname):
        self.file_name_ = None
        self.options = options

        # Data stuff
        self.data = None
        self.parse_offset = 0
        self.data_offset = 0
        self.string_block_offset = 0

        # Bytes to set of byte fields handling
        self.field_data = []
        self.record_parser = None

        # Searching
        self.id_data = None

        self.id_format_str = None

        # See that file exists already
        normalized_path = os.path.abspath(fname)
        for i in ['', '.db2', '.dbc', '.adb']:
            if os.access(normalized_path + i, os.R_OK):
                self.file_name_ = normalized_path + i
                logging.debug('WDB file found at %s', self.file_name_)

        if not self.file_name_:
            logging.error('No WDB file found based on "%s"', fname)
            sys.exit(1)

        # Data format storage
        self.fmt = dbc.fmt.DBFormat(options)
        # Static data fields
        try:
            self.data_format = self.fmt.objs(self.class_name())
        except Exception as e:
            self.data_format = None

    def is_wch(self):
        return False

    def id_format(self):
        if self.id_format_str:
            return self.id_format_str

        if self.last_id == 0:
            return '%u'

        # Adjust the size of the id formatter so we get consistent length
        # id field where we use it, and don't have to guess on the length
        # in the format file.
        n_digits = int(math.log10(self.last_id) + 1)
        self.id_format_str = '%%%uu' % n_digits
        return self.id_format_str

    def use_inline_strings(self):
        return False

    def build_parser(self):
        if self.options.raw:
            self.record_parser = self.create_raw_parser()
        else:
            self.record_parser = self.create_formatted_parser(self.use_inline_strings())

        return True

    # Build a raw parser out of field data
    def create_raw_parser(self):
        field_idx = 0
        format_str = '<'
        unpackers = []

        for field_data_idx in range(0, len(self.field_data)):
            field_size = self.field_data[field_data_idx][1]
            field_count = self.field_data[field_data_idx][2]

            for sub_idx in range(0, field_count):
                if field_size == 1:
                    format_str += 'b'
                elif field_size == 2:
                    format_str += 'h'
                elif field_size >= 3:
                    format_str += 'i'
                field_idx += 1

                if field_size == 3:
                    logging.debug('Unpacker has a 3-byte field (pos=%d): terminating (%s) and beginning a new unpacker',
                        field_idx, format_str)
                    unpacker = struct.Struct(format_str)
                    unpackers.append((True, unpacker))
                    format_str = '<'

        if len(format_str) > 1:
            unpackers.append((self.field_data[-1][1] == 3, struct.Struct(format_str)))

        logging.debug('Unpacking plan for %s: %s',
            self.full_name(),
            ', '.join(['%s (len=%d, i24=%s)' % (u.format.decode('utf-8'), u.size, i24) for i24, u in unpackers]))

        if len(self.unpackers) == 1:
            return lambda data, ro, rs: unpackers[0][1].unpack_from(data, ro)
        else:
            return lambda data, ro, rs: _do_parse(unpackers, data, ro, rs)

    # Validate the json-based data format against the DB2-included data format
    # for field information.
    def validate_format(self):
        if not self.data_format:
            logging.error('No data format found for %s', self.class_name())
            return False

        if len(self.data_format) != len(self.field_data):
            logging.error('%s: Data format mismatch, record has %u fields, format file %u',
                self.full_name(), len(self.field_data), len(self.data_format))
            return False

        # Ensure fields from our json formats and internal field data stay consistent
        for field_data_idx in range(0, len(self.field_data)):
            field_data = self.field_data[field_data_idx]
            field_format = self.data_format[field_data_idx]

            field_size = field_data[1]
            field_count = field_data[2]

            if (field_data_idx < len(self.field_data) - 1 and field_count != field_format.elements) or \
                    field_count < field_format.elements:
                logging.error('%s: Internal field count mismatch for field %u (%s), record=%u, format=%u',
                    self.full_name(), field_data_idx + 1, field_format.base_name(), field_count, field_format.elements)
                return False

            if field_size not in field_format.field_size():
                logging.error('%s: Internal field size mismatch for field %u (%s), record=%u, format=%s (%s)',
                    self.full_name(), field_data_idx + 1, field_format.base_name(), field_size,
                    ', '.join([str(x) for x in field_format.field_size()]),
                    field_format.data_type)
                return False


        return True

    # Sanitize data, blizzard started using dynamic width ints in WDB5, so
    # 3-byte ints have to be expanded to 4 bytes to parse them properly (with
    # struct module)
    def create_formatted_parser(self, inline_strings):
        field_idx = 0
        format_str = '<'
        unpackers = []

        if not self.validate_format():
            return None

        for field_data_idx in range(0, len(self.field_data)):
            # Ensure fields from our json formats and internal field data stay consistent
            field_data = self.field_data[field_data_idx]
            field_format = self.data_format[field_data_idx]

            field_size = field_data[1]
            field_count = field_data[2]

            # The final field needs special handling because of padding
            # concerns. For formatted data, always presume that the format
            # (json) file has the correct length.
            if field_data_idx == len(self.field_data) - 1:
                field_count = min(field_count, field_format.elements)

            for sub_idx in range(0, field_count):
                # String format fields get converted to something Struct module
                # understands. Depending on the DB2 file type, customized
                # string parsing may be neeedd
                if field_format.data_type == 'S':
                    # No offset map, translate 'S' based on the DB2 field
                    # length to an unsigned Byte, Short, or Integer.
                    if not inline_strings:
                        if field_size == 1:
                            format_str += field_format.data_type.replace('S', 'B')
                        elif field_size == 2:
                            format_str += field_format.data_type.replace('S', 'H')
                        elif field_size >= 3:
                            format_str += field_format.data_type.replace('S', 'I')
                    # Offset map, create an inline string parser (StringUnpacker)
                    else:
                        logging.debug('Inline string field (name=%s, pos=%d [%d]), end previous parser',
                            field_format.base_name(), field_idx, field_data_idx)
                        if len(format_str) > 1:
                            unpackers.append((field_size == 3, struct.Struct(format_str)))
                            format_str = '<'
                        unpackers.append((field_size == 3, StringUnpacker(field_size)))
                # Normal (unsigned) byte/short/int/float field, apply to Struct parser as-is
                else:
                    format_str += field_format.data_type

                field_idx += 1

                # 3-byte fields ends the current Struct parser and start a new
                # one. Struct does not support 24bit ints, so we need to parse
                # a 32bit one, mask away the excess and adjust parser offset.
                # Indicate 24bitness as the first element of the tuple (the
                # True value).
                if field_size == 3 and len(format_str) > 1:
                    logging.debug('Unpacker has a 3-byte field (name=%s pos=%d): terminating (%s) and beginning a new unpacker',
                        field_format.base_name(), field_idx, format_str)
                    unpackers.append((True, struct.Struct(format_str)))
                    format_str = '<'

        if len(format_str) > 1:
            unpackers.append((self.field_data[-1][1] == 3, struct.Struct(format_str)))

        logging.debug('Unpacking plan for %s: %s',
            self.full_name(),
            ', '.join(['%s (len=%d, i24=%s)' % (u.format.decode('utf-8'), u.size, i24) for i24, u in unpackers]))

        # One parser unpackers don't need to go through a function, can just do
        # the parsing in one go through a lambda function. Multi-parsers require state to be kept.
        if len(unpackers) == 1:
            return lambda data, ro, rs: unpackers[0][1].unpack_from(data, ro)
        else:
            return lambda data, ro, rs: _do_parse(unpackers, data, ro, rs)

    def n_expanded_fields(self):
        return sum([ fd[2] for fd in self.field_data ])

    # Can we search for data? This is only true, if we know where the ID in the data resides
    def searchable(self):
        return False

    def raw_outputtable(self):
        return False

    def full_name(self):
        return os.path.basename(self.file_name_)

    def file_name(self):
        return os.path.basename(self.file_name_).split('.')[0]

    def class_name(self):
        return os.path.basename(self.file_name_).split('.')[0]

    def name(self):
        return os.path.basename(self.file_name_).split('.')[0].replace('-', '_').lower()

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
            logging.error('%s: Invalid data file format %s', self.class_name(), self.data[:4].decode('utf-8'))
            return False

        self.parse_offset += 4
        self.records, self.fields, self.record_size, self.string_block_size = _BASE_HEADER.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _BASE_HEADER.size

        return True

    def fields_str(self):
        fields = []

        fields.append('byte_size=%u' % len(self.data))
        fields.append('records=%u (%u)' % (self.records, self.n_records()))
        fields.append('fields=%u (%u)' % (self.fields, self.n_fields()))
        fields.append('o_data=%u' % self.data_offset)
        fields.append('record_size=%u' % self.record_size)
        if self.string_block_size > 0:
            fields.append('sb_size=%u' % self.string_block_size)
        if self.string_block_offset > 0:
            fields.append('o_sb=%u' % self.string_block_offset)

        return fields

    def __str__(self):
        return '%s::%s(%s)' % (self.full_name(), self.magic.decode('ascii'), ', '.join(self.fields_str()))

    def build_field_data(self):
        types = self.fmt.types(self.class_name())
        field_offset = 0
        self.field_data = []
        for t in types:
            if t in ['I', 'i', 'f', 'S']:
                self.field_data.append((field_offset, 4, 1))
            elif t in ['H', 'h']:
                self.field_data.append((field_offset, 2, 1))
            elif t in ['B', 'b']:
                self.field_data.append((field_offset, 1, 1))
            else:
                # If the format file has padding fields, add them as correct
                # length fields to the data format. These are only currently
                # used to align offset map files correctly, so data model
                # validation will work.
                m = re.match('([0-9]+)x', t)
                if m:
                    self.field_data.append((field_offset, int(m.group(1)), 1))

            field_offset += self.field_data[-1][1]

        return True

    def open(self):
        if self.data:
            return True

        f = io.open(self.file_name_, mode = 'rb')
        self.data = f.read()
        f.close()

        if not self.parse_header():
            return False

        # Build auxilary structures to parse raw data out of the DBC file
        if not self.build_field_data():
            return False

        # Build a parser out of field data (+ file format), if this succeeds things are looking ok
        if not self.build_parser():
            return False

        # Figure out the position of the id column. This may be none if WDB4/5
        # and id_block_offset > 0
        if not self.options.raw:
            for idx in range(0, len(self.data_format)):
                if self.data_format[idx].base_name() == 'id':
                    self.id_data = self.field_data[idx]
                    break

        # After headers begins data, always
        self.data_offset = self.parse_offset

        # If this is an actual WDB file (or WCH file with -t view), setup the
        # correct id format to the formatter
        if not self.options.raw and (not self.is_wch() or self.options.type == 'view'):
            dbc.data._FORMATDB.set_id_format(self.class_name(), self.id_format())

        return True

    def get_string(self, offset):
        if offset == 0:
            return None

        end_offset = self.data.find(b'\x00', self.string_block_offset + offset)

        if end_offset == -1:
            return None

        return self.data[self.string_block_offset + offset:end_offset].decode('utf-8')

    def find_record_offset(self, id_):
        unpacker = None
        if self.id_data[1] == 1:
            unpacker = _BYTE
        elif self.id_data[1] == 2:
            unpacker = _SHORT
        elif self.id_data[1] >= 3:
            unpacker = _INT32

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
    def get_record_info(self, record_id):
        return -1, self.data_offset + record_id * self.record_size, self.record_size

    def get_record(self, offset, size):
        return self.record_parser(self.data, offset, size)

    def find(self, id_):
        dbc_id, record_offset, record_size = self.find_record_offset(id_)

        if record_offset > 0:
            return dbc_id, self.record_parser(record_offset, record_size)
        else:
            return 0, tuple()

# Proxy string unpacker for inlined strings. The size of the most recent parse
# operation is stored in the size variable
class StringUnpacker:
    def __init__(self, default_field_size):
        # set the default field size to the DB2 field size, it will be wrong
        # but will work in terms of the header computation sanity checks
        self.size = default_field_size

    # Mimick Struct interface, unpack_from simply returns the offset as the
    # single value for the unpack operation. Additionally, unpack_from sets the
    # size of the field by seeking the first null-byte in the inline string +
    # 1. If the field is empty, the inline string field size will be 1,
    # regardless of what the db2 field format value claims.
    def unpack_from(self, data, offset):
        # Find first \x00 byte
        pos = data.find(b'\x00', offset)
        self.size = (pos - offset) + 1
        return (offset,)

    def __getattribute__(self, attr):
        if attr == 'format':
            return b'I' # Always pretend to be an unsigned integer (offset into file)

        return super().__getattribute__(attr)

class LegionWDBParser(DBCParserBase):
    def __init__(self, options, fname):
        super().__init__(options, fname)

        self.id_block_offset = 0
        self.clone_block_offset = 0
        self.offset_map_offset = 0

        self.id_table = []

    def use_inline_strings(self):
        return self.has_offset_map()

    def has_offset_map(self):
        return (self.flags & X_OFFSET_MAP) == X_OFFSET_MAP

    def has_id_block(self):
        return (self.flags & X_ID_BLOCK) == X_ID_BLOCK

    def n_cloned_records(self):
        return self.clone_segment_size // _CLONE.size

    def n_records(self):
        if self.id_block_offset:
            return len(self.id_table)
        else:
            return self.records

    # Can search through WDB4/5 files if there's an id block, or if we can find
    # an ID from the formatted data
    def searchable(self):
        if self.options.raw:
            return self.id_block_offset > 0
        else:
            return True

    def find_record_offset(self, id_):
        if self.has_id_block():
            for record_id in range(0, self.n_records()):
                if self.id_table[record_id][0] == id_:
                    return self.id_table[record_id]
            return -1, 0, 0
        else:
            return super().find_record_offset(id_)

    def offset_map_entry(self, offset):
        return _ITEMRECORD.unpack_from(self.data, offset)

    def offset_map_entry_size(self):
        return _ITEMRECORD.size

    def build_id_table(self):
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
            if self.has_offset_map():
                record_index = dbc_id - self.first_id
                record_data_offset = self.offset_map_offset + record_index * self.offset_map_entry_size()
                data_offset, size = self.offset_map_entry(record_data_offset)
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
        # If we have an idtable, just index directly to it
        self.get_record_info = lambda record_id: self.id_table[record_id]

    def open(self):
        if not super().open():
            return False

        self.build_id_table()

        logging.debug('Opened %s' % self.full_name())
        return True

    def fields_str(self):
        fields = super().fields_str()

        fields.append('table_hash=%#.8x' % self.table_hash)
        if hasattr(self, 'build'):
            fields.append('build=%u' % self.build)
        if hasattr(self, 'timestamp'):
            fields.append('timestamp=%u' % self.timestamp)
        fields.append('first_id=%u' % self.first_id)
        fields.append('last_id=%u' % self.last_id)
        fields.append('locale=%#.8x' % self.locale)
        if self.clone_segment_size > 0:
            fields.append('clone_size=%u' % self.clone_segment_size)
            fields.append('o_clone_block=%u' % self.clone_block_offset)
        if self.id_block_offset > 0:
            fields.append('o_id_block=%u' % self.id_block_offset)
        if self.offset_map_offset > 0:
            fields.append('o_offset_map=%u' % self.offset_map_offset)
        if hasattr(self, 'flags'):
            fields.append('flags=%#.8x' % self.flags)

        return fields

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
        if self.has_id_block():
            self.id_block_offset = self.parse_offset + self.records * self.record_size + self.string_block_size

        # Offset map contains offset into file, record size entries in a sparse structure
        if self.has_offset_map():
            self.offset_map_offset = self.string_block_size
            self.string_block_offset = 0
            if self.has_id_block():
                self.id_block_offset = self.string_block_size + ((self.last_id - self.first_id) + 1) * _ITEMRECORD.size

        # Has clone block
        if self.clone_segment_size > 0:
            self.clone_block_offset = self.id_block_offset + self.records * _ID.size

        return True

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

        self.table_hash, self.layout_hash, self.first_id = _DB_HEADER_1.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _DB_HEADER_1.size

        self.last_id, self.locale, self.clone_segment_size, self.flags, self.id_index = _DB_HEADER_2.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _DB_HEADER_2.size

        # Setup offsets into file, first string block, skip field data information of WDB5 files
        if self.string_block_size > 2:
            self.string_block_offset = self.parse_offset + self.records * self.record_size
            self.string_block_offset += self.fields * 4

        # Has ID block, note that ID-block needs to skip the field data information on WDB5 files
        if self.has_id_block():
            self.id_block_offset = self.parse_offset + self.records * self.record_size + self.string_block_size
            self.id_block_offset += self.fields * 4

        # Offset map contains offset into file, record size entries in a sparse structure
        if self.has_offset_map():
            self.offset_map_offset = self.string_block_size
            self.string_block_offset = 0
            if self.has_id_block():
                self.id_block_offset = self.string_block_size + ((self.last_id - self.first_id) + 1) * _ITEMRECORD.size

        # Has clone block
        if self.clone_segment_size > 0:
            self.clone_block_offset = self.id_block_offset + self.records * _ID.size

        return True

    def fields_str(self):
        fields = super().fields_str()

        fields.append('layout_hash=%#.8x' % self.layout_hash)
        if self.id_index > 0:
            fields.append('id_index=%u' % self.id_index)

        return fields

    # WDB5 can always output some human readable data, even if the field types
    # are not correct, but only do this if --raw is enabled
    def raw_outputtable(self):
        return self.options.raw

    def n_fields(self):
        return sum([fd[2] for fd in self.field_data])

    def parsed_record_size(self):
        return len(self.unpackers) and (self.unpackers[-1][2] + self.unpackers[-1][1].size) or 0

    def __str__(self):
        s = super().__str__()

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

class WDB6RecordParser:
    def __init__(self, parser, static_parser):
        self.parser = parser
        self.record_parser = static_parser
        self.fields = self.parser.fmt.objs(self.parser.class_name(), True)

        if not self.parser.id_index:
            raise AttributeError

        self.id_index = 0
        for i in range(0, len(self.fields)):
            if i == self.parser.id_index:
                break

            self.id_index += self.fields[i].elements

    def __call__(self, file_data, offset, size):
        data = self.record_parser(file_data, offset, size)
        idx = 0
        for i in range(0, len(self.fields)):
            value = self.parser.get_field_value(i, data[self.id_index])
            if idx < len(data) -1:
                if value != None:
                    data[idx] = value
            else:
                if value != None:
                    data.append(value)
                else:
                    data.append(0)

            idx += self.fields[i].elements

        return data

class WDB6Parser(WDB5Parser):
    def is_magic(self): return self.magic == b'WDB6'

    def __init__(self, options, fname):
        super().__init__(options, fname)

        self.field_block_offset = 0
        self.field_blocks = []

    def get_field_value(self, field_index, id_):
        if len(self.field_blocks) < field_index:
            return None

        offset = self.field_blocks[field_index]['values'].get(id_, 0)
        if offset == 0:
            return None

        return self.field_blocks[field_index]['parser'].unpack_from(self.data, offset)[0]

    def build_parser(self):
        # Always create a default parser
        if not super().build_parser():
            return False

        # With field blocks, we need a significantly different parser to spit
        # out raw data, so instantiate a wrapper class to do the stitching of
        # data together
        if self.field_block_offset > 0:
            self.record_parser = WDB6RecordParser(self, self.record_parser)

        return True

    def parse_header(self):
        if not super().parse_header():
            return False

        self.n_real_fields, self.field_block_size = _DB_HEADER_3.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _DB_HEADER_3.size

        # Setup offsets into file, first string block, skip field data information of WDB5 files
        if self.string_block_size > 2:
            self.string_block_offset = self.parse_offset + self.records * self.record_size
            self.string_block_offset += self.fields * 4

        # Has ID block, note that ID-block needs to skip the field data information on WDB5 files
        if self.has_id_block():
            self.id_block_offset = self.parse_offset + self.records * self.record_size + self.string_block_size
            self.id_block_offset += self.fields * 4

        # Offset map contains offset into file, record size entries in a sparse structure
        if self.has_offset_map():
            self.offset_map_offset = self.string_block_size
            self.string_block_offset = 0
            if self.has_id_block():
                self.id_block_offset = self.string_block_size + ((self.last_id - self.first_id) + 1) * _ITEMRECORD.size

        # Has clone block
        if self.clone_segment_size > 0:
            self.clone_block_offset = self.id_block_offset + self.records * _ID.size

        # Has WDB6 specific field block segment, for now it seems it's the
        # final block of the file, this may change though
        if self.field_block_size > 0:
            self.field_block_offset = len(self.data) - self.field_block_size

        return True

    def parse_field_block(self):
        if self.field_block_size == 0:
            return True

        # Field block starts with a 4 byte field (could be 2x 2 bytes too or
        # anything else that sums to 4 bytes) indicating the number of field
        # blocks
        offset = self.field_block_offset
        n_blocks = struct.unpack_from(u'I', self.data, offset)[0]
        offset += 4

        self.field_blocks = [ {} for i in range(0, n_blocks) ]

        id_parser = struct.Struct('I')

        # Then, each field block follows, record the data into self.field_blocks for later use
        # The data recorded includes the number of entries, and a dictionary of
        # id, offset pairs that allow the parser to access the data of the
        # field for the (dbc) id record
        index = 0
        while index < n_blocks:
            # First comes a block header that contains at least the number of
            # values, and (possibly?) a type of the block (e.g., int/float etc)
            n_values, type_ = _WDB6_BLOCK_HEADER.unpack_from(self.data, offset)
            offset += _WDB6_BLOCK_HEADER.size

            self.field_blocks[index] = {
                'type': type_,
                'values': {}
            }

            if type_ == 3:
                self.field_blocks[index]['parser'] = struct.Struct('f')
            elif type_ == 4:
                self.field_blocks[index]['parser'] = struct.Struct('i')

            # Then scan through the values to record pointers to data
            value_index = 0
            while value_index < n_values:
                # Only grab the ID for now, other option is to parse the data already here ..
                id_ = id_parser.unpack_from(self.data, offset)[0]
                self.field_blocks[index]['values'][id_] = offset + id_parser.size

                offset += id_parser.size + 4
                value_index += 1

            index += 1

        return True

    def fields_str(self):
        fields = super().fields_str()

        fields.append('n_real_fields=%u' % self.n_real_fields)
        fields.append('field_block_size=%u' % self.field_block_size)

        return fields

    def open(self):
        if not super().open():
            return False

        if not self.parse_field_block():
            return False

        return True

class LegionWCHParser(LegionWDBParser):
    # For some peculiar reason, some WCH files are completely alien, compared
    # to the rest, and expand all the dynamic width variables to 4 byte fields.
    # Manually make a list here to support those files in build_parser.
    __override_dbcs__ = [ 'SpellEffect' ]

    def is_magic(self): return self.magic == b'WCH5' or self.magic == b'WCH6'

    def __init__(self, options, wdb_parser, fname):
        super().__init__(options, fname)

        self.clone_segment_size = 0 # WCH files never have a clone segment
        self.wdb_parser = wdb_parser

    def has_id_block(self):
        return self.wdb_parser.has_id_block()

    def has_offset_map(self):
        return self.wdb_parser.has_offset_map()

    def parse_header(self):
        if not super().parse_header():
            return False

        self.table_hash, self.layout_hash, self.build, self.timestamp, self.first_id, self.last_id, self.locale = _WCH5_HEADER.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _WCH5_HEADER.size

        # Setup offsets into file, first string block
        if self.string_block_size > 2:
            self.string_block_offset = self.parse_offset + self.records * self.record_size

        # Offset map contains offset into file, record size entries in a sparse structure
        if self.has_offset_map():
            self.offset_map_offset = self.parse_offset
            self.string_block_offset = 0
            self.id_block = 0

        # Has ID block
        if self.has_id_block():
            self.id_block_offset = self.parse_offset + self.records * self.record_size + self.string_block_size

        return True

    def build_id_table(self):
        if self.id_block_offset == 0 and self.offset_map_offset == 0:
            return

        record_id = 0
        if self.has_offset_map():
            while record_id < self.records:
                ofs_offset_map_entry = self.offset_map_offset + record_id * _WCH_ITEMRECORD.size
                dbc_id, data_offset, size = _WCH_ITEMRECORD.unpack_from(self.data, ofs_offset_map_entry)
                self.id_table.append((dbc_id, data_offset, size))
                record_id += 1
        elif self.has_id_block():
            unpacker = struct.Struct('%dI' % self.records)
            for dbc_id in unpacker.unpack_from(self.data, self.id_block_offset):
                size = self.record_size
                data_offset = self.data_offset + record_id * self.record_size
                self.id_table.append((dbc_id, data_offset, size))
                record_id += 1

        # If we have an idtable, just index directly to it
        self.get_record_info = lambda record_id: self.id_table[record_id]

    def is_wch(self):
        return True

    # Inherit field data from the WDB parser
    def build_field_data(self):
        self.field_data = self.wdb_parser.field_data

        return True

    # WCH files may need a specialized parser building if the parent WDB file
    # does not use an ID block. If ID block is used, the corresponding WCH file
    # will also have dynamic width fields. As an interesting side note, the WCF
    # files actually have record sizes at the correct length, and not padded.
    def build_parser(self):
        if self.class_name() in self.__override_dbcs__:
            logging.debug('==NOTE== Overridden DBC: Expanding all record fields to 4 bytes ==NOTE==')
            return self.build_parser_wch5()
        else:
            return super().build_parser()

    # Override record parsing completely by making a unpacker that generates 4
    # byte fields for the record. This is necessary in the case of SpellEffect
    # (as of 2016/7/17 at least), since the actual client data file format is
    # not honored at all. There does not seem to be an identifying marker in
    # the client data (or the cache file) to automatically determine when to
    # use the overridden builder, so __override_dbcs__ contains a list of files
    # where it is used.
    def build_parser_wch5(self):
        format_str = '<'

        data_fmt = field_names = None
        if not self.options.raw:
            data_fmt = self.fmt.types(self.class_name())
        field_idx = 0

        field_offset = 0
        for field_data_idx in range(0, len(self.field_data)):
            field_data = self.field_data[field_data_idx]
            type_idx = min(field_idx, data_fmt and len(data_fmt) - 1 or field_idx)
            for sub_idx in range(0, field_data[2]):
                if data_fmt[type_idx] in ['S', 'H', 'B']:
                    format_str += 'I'
                elif data_fmt[type_idx] in ['h', 'b']:
                    format_str += 'i'
                else:
                    format_str += data_fmt[type_idx]
                field_idx += 1

        if len(format_str) > 1:
            self.unpackers.append((0xFFFFFFFF, struct.Struct(format_str), field_offset))

        logging.debug('Unpacking plan for static data of %s: %s',
            self.full_name(),
            ', '.join(['%s (len=%d, offset=%d)' % (u.format.decode('utf-8'), u.size, o) for _, u, o in self.unpackers]))
        if len(self.unpackers) == 1:
            self.record_parser = lambda ro, rs: self.unpackers[0][1].unpack_from(self.data, ro)
        else:
            self.record_parser = self.__do_parse

        return True

class WCH7Parser(LegionWCHParser):
    def is_magic(self): return self.magic == b'WCH7' or self.magic == b'WCH8'

    def __init__(self, options, wdb_parser, fname):
        super().__init__(options, wdb_parser, fname)

        self.wch7_unk_data = []

    # Completely rewrite WCH7 parser, since the base header gained a new field
    def parse_header(self):
        self.magic = self.data[:4]

        if not self.is_magic():
            logging.error('Invalid data file format %s', self.data[:4].decode('utf-8'))
            return False

        self.parse_offset += 4
        self.records, self.wch7_unk, self.fields, self.record_size, self.string_block_size = _WCH7_BASE_HEADER.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _WCH7_BASE_HEADER.size

        self.table_hash, self.layout_hash, self.build, self.timestamp, self.first_id, self.last_id, self.locale = _WCH5_HEADER.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _WCH5_HEADER.size

        # Setup offsets into file, first string block
        if self.string_block_size > 2:
            self.string_block_offset = self.parse_offset + self.records * self.record_size

        # If self.wch7_unk > 0, parse out the unknown data. For now presume it's a bunch of 4 byte fields
        for idx in range(0, self.wch7_unk):
            v = _ID.unpack_from(self.data, self.parse_offset + self.records * self.record_size + self.string_block_size + idx * 4)[0]
            self.wch7_unk_data.append(v)

        # Offset map contains offset into file, record size entries in a sparse structure
        if self.has_offset_map():
            self.offset_map_offset = self.parse_offset
            self.string_block_offset = 0
            self.id_block = 0

        # Has ID block
        if self.has_id_block():
            self.id_block_offset = self.parse_offset + self.records * self.record_size + self.string_block_size
            # A mysterious offset appears, for what reason, somebody probably knows
            self.id_block_offset += self.wch7_unk * 4

        return True

    def fields_str(self):
        fields = super().fields_str()

        fields.append('unk_wch7=%u' % self.wch7_unk)
        if len(self.wch7_unk_data) > 0:
            arr = [str(x) for x in self.wch7_unk_data]
            fields.append('unk_wch7_data={ %s }' % ', '.join(arr))

        return fields
