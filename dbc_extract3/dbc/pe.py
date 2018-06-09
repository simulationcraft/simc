import os, mmap, logging, struct, collections

try:
    from pefile import PE
except Exception as error:
    print(('ERROR: %s, dbc_extract.py requires the Python pefile (https://pypi.org/project/pefile/)'
           ' package to perform client data file validation') % error, file = sys.stderr)
    sys.exit(1)

from dbc.file import DBCFile
from dbc import wdc1

_DB_STRUCT = struct.Struct('QIIIIIIQQQQQQQIIIII')

_DB_STRUCT_HEADER_FIELDS = [
    'va_name',
    'unk_2',
    'fields',
    'unk_4',
    'unk_5',
    'unk_6',
    'unk_7',
    'va_byte_offset',
    'va_elements',
    'va_field_format',
    'va_unk_11',
    'va_unk_12',
    'va_unk_13',
    'va_unk_14',
    'unk_15',
    'table_hash',
    'unk_17',
    'layout_hash',
    'unk_19'
]

_DB_STRUCT_HEADER_FORMATS = [
    '{:#016x}',
    '{}',
    '{}',
    '{}',
    '{}',
    '{:#08x}',
    '{}',
    '{:#016x}',
    '{:#016x}',
    '{:#016x}',
    '{:#016x}',
    '{:#016x}',
    '{:#016x}',
    '{:#016x}',
    '{}',
    '{:#08x}',
    '{}',
    '{:#08x}',
    '{}'
]

_DB_FIELD_FORMATS = {
    0: ('int32', 'i'),
    1: ('int64', 'q'),
    2: ('string', 'S'),
    3: ('float', 'f'),
    4: ('int8', 'b'),
    5: ('int16', 'h')
}

# Calculate the negative offset from table_hash to the start, based on the struct info
def compute_field_offset(field):
    offset = 0
    for idx in range(0, len(_DB_STRUCT.format)):
        if _DB_STRUCT_HEADER_FIELDS[idx] == field:
            return offset

        if chr(_DB_STRUCT.format[idx]) in ['I', 'i']:
            offset += 4
        elif chr(_DB_STRUCT.format[idx]) in ['Q', 'q']:
            offset += 8
        elif chr(_DB_STRUCT.format[idx]) in ['H', 'h']:
            offset += 2
        elif chr(_DB_STRUCT.format[idx]) in ['B', 'b']:
            offset += 1
        else:
            print('Unknown field format {}'.format(_DB_STRUCT.format[idx]))

    return 0

class DBStructureHeader(collections.namedtuple('DBStructureHeader', _DB_STRUCT_HEADER_FIELDS)):
    def name(self, parser):
        va = self.file_offset(parser, 'va_name')

        null = parser.handle.find(b'\x00', va)

        return parser.handle[va:null].decode('utf-8')

    def file_offset(self, parser, field_name):
        index = -1
        for idx in range(0, len(self._fields)):
            if self._fields[idx] == field_name:
                index = idx
                break

        if index == -1:
            return -1

        va = self[index]
        va -= parser.image_base
        va -= parser.rdata_voffset
        va += parser.rdata_start

        return va

    def formats(self, parser):
        format_offset = self.file_offset(parser, 'va_field_format')

        unpacker = struct.Struct('i' * self.fields)

        formats = unpacker.unpack_from(parser.handle, format_offset)
        struct_formats = []
        for format_value in formats:
            if format_value not in _DB_FIELD_FORMATS:
                logging.warn('Format value type {} not known', format_value)
                struct_formats.append(('unknown', 'z'))
            else:
                struct_formats.append(_DB_FIELD_FORMATS[format_value])

        return struct_formats

    def elements(self, parser):
        element_offset = self.file_offset(parser, 'va_elements')

        unpacker = struct.Struct('i' * self.fields)

        return unpacker.unpack_from(parser.handle, element_offset)

    def __str__(self):
        s = []

        for idx in range(0, len(self._fields)):
            if idx < len(_DB_STRUCT_HEADER_FORMATS):
                ss = '{}='.format(_DB_STRUCT_HEADER_FIELDS[idx])
                ss += _DB_STRUCT_HEADER_FORMATS[idx].format(self[idx])
                s.append(ss)
            else:
                s.append('{}={}'.format(_DB_STRUCT_HEADER_FIELDS[idx], self[idx]))

        return ', '.join(s)

class PeStructParser:
    def __init__(self, options, wow_binary, files):
        self.dbfiles = files
        self.options = options
        self.wow_binary = wow_binary

    def initialize(self):
        self.file_handle = open(self.wow_binary, 'rb')
        self.handle = mmap.mmap(self.file_handle.fileno(), 0, access = mmap.ACCESS_READ)
        self.pe = PE(data = self.handle, fast_load = True)

        logging.info('{} ImageBase={:#016x}'.format(self.wow_binary,
            self.pe.OPTIONAL_HEADER.ImageBase))

        self.image_base = self.pe.OPTIONAL_HEADER.ImageBase

        for section in self.pe.sections:
            if b'.rdata' in section.Name:
                self.rdata_start = section.PointerToRawData
                self.rdata_end   = self.rdata_start + section.SizeOfRawData
                self.rdata_voffset = section.VirtualAddress

                logging.info(('{} .rdata found at PhysicalAddress={:#08x} '
                                                 'VirtualAddress={:#08x}').format(
                    self.wow_binary, self.rdata_start, self.rdata_voffset))

        return True

    def validate_file(self, file_):
        dbfile = DBCFile(self.options, file_)
        try:
            if not dbfile.open():
                return True
            field_formats = dbfile.parser.fmt.objs(dbfile.class_name(), True)

        except Exception:
            logging.debug('No data format for {}'.format(file_))
            return True

        # Dun care, nothing to check
        if dbfile.parser.records == 0:
            return True

        locations = []
        search_for = bytes(dbfile.class_name(), 'utf-8')

        logging.debug(dbfile.parser)

        # Construct an unique fingerprint to find in file, based on table and layout hashes

        table_hash = struct.pack('I', dbfile.parser.table_hash)
        layout_hash = struct.pack('I', dbfile.parser.layout_hash)

        # Offset of table hash in the DB structure header from the start
        table_hash_offset = compute_field_offset('table_hash')

        header_start = 0
        search_offset = self.rdata_start
        while search_offset < self.rdata_end + 8:
            # First, find the table hash
            th_offset = self.handle.find(table_hash, search_offset, self.rdata_end)
            if th_offset == -1:
                return True

            # Then the layout hash should be at th_offset + 8 (one 4 byt efield inbetween)
            if self.handle[th_offset + 8:th_offset + 12] != layout_hash:
                search_offset = th_offset + 4
                continue

            logging.info(('Found {} layout_hash={:#8x}, table_hash={:#8x} at '
                          'offset {}, DB Structure start at {}').format(
                os.path.basename(dbfile.file_name), dbfile.parser.table_hash,
                dbfile.parser.layout_hash,
                th_offset, th_offset - table_hash_offset
            ))

            header_start = th_offset - table_hash_offset
            break

        if header_start == 0:
            logging.info('No DB structure found for {}'.format(dbfile.class_name()))
            return True

        unpack = _DB_STRUCT.unpack_from(self.handle, header_start)
        # Header must match table and layout hashes

        header = DBStructureHeader(*unpack)

        formats = header.formats(self)
        elements = header.elements(self)

        if len(formats) != len(field_formats):
            logging.warn('Field count mismatch for {}, binary={}, format_file={}'.format(
                dbfile.class_name(), len(formats), len(field_formats)))

        for field_idx in range(0, len(formats)):
            col = dbfile.parser.column(field_idx)
            field_file_format = field_formats[field_idx].data_type.lower()

            # For "bit" fields, allow tightening of the field size, since we
            # only need to parse a specific number of bytes, even though
            # internally (in the client) the value may be stored in whatever
            # size
            compare_widths = col.field_ext_type() in [wdc1.COLUMN_TYPE_BIT, wdc1.COLUMN_TYPE_BIT_S]

            if compare_widths:
                field_bytes = col.field_whole_bytes()
                if field_bytes not in field_formats[field_idx].field_size():
                    logging.warn(('Bitpacked field type discrepancy for {} field {} "{}", '
                                  'wow_file={}, format_file={}, metadata={} ({})').format(
                        os.path.basename(dbfile.file_name), field_idx + 1,
                        field_formats[field_idx].base_name(),
                        formats[field_idx][0],
                        field_formats[field_idx].type_name(),
                        col.short_type(),
                        col.field_whole_bytes()))
            else:
                # Ensure formats match, without checking signedness. Signedness can
                # be checked only for bit-packed fields.
                if field_file_format != formats[field_idx][1].lower():
                    logging.warn(('Field type discrepancy for {} field {} "{}", '
                                  'wow_file={}, format_file={}').format(
                        os.path.basename(dbfile.file_name), field_idx + 1,
                        field_formats[field_idx].base_name(), formats[field_idx][0],
                        field_formats[field_idx].type_name()))

            # Check integer signedness on fields
            check_signed = field_file_format != 'f' and \
                           col.field_ext_type() in [wdc1.COLUMN_TYPE_BIT, wdc1.COLUMN_TYPE_BIT_S]
            format_is_signed = field_formats[field_idx].data_type.islower()

            if check_signed and col.is_signed() != format_is_signed:
                logging.warn(('Field signedness discrepancy for {} field {} "{}", '
                              'wow_file={}, format_file={}').format(
                    os.path.basename(dbfile.file_name), field_idx + 1,
                    field_formats[field_idx].base_name(),
                    col.is_signed() and 'Signed' or 'Unsigned',
                    format_is_signed and 'Signed' or 'Unsigned'))

            # Check field counts
            format_elements = field_formats[field_idx].elements
            if format_elements != elements[field_idx]:
                logging.warn(('Element count discrepancy for {} field {} "{}", '
                              'wow_file={}, format_file={}').format(
                    os.path.basename(dbfile.file_name), field_idx + 1,
                    field_formats[field_idx].base_name(),
                    elements[field_idx], format_elements))

        return True

    def validate(self):
        if self.options.raw:
            logging.error('Cannot validate field format with --raw mode')
            return True

        for dbfile in self.dbfiles:
            if not self.validate_file(dbfile):
                return False

        return True
