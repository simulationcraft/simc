import os, mmap, logging, struct, collections

try:
    from pefile import PE
except Exception as error:
    import sys
    print(('ERROR: %s, dbc_extract.py requires the Python pefile (https://pypi.org/project/pefile/)'
           ' package to perform client data file validation') % error, file = sys.stderr)
    sys.exit(1)

from dbc.file import DBCFile
from dbc import wdc1

_DB_STRUCT = struct.Struct('QIIIIIIQQQQQQQIIIII')

_DB_STRUCT_HEADER_FIELDS = [
    'va_name',
    'file_data_id',
    'fields',
    'size',
    'unk_5',
    'unk_6',
    'unk_7',
    'va_byte_offset',
    'va_elements',
    'va_field_format',
    'va_flags',
    'va_elements_file',
    'va_field_format_file',
    'va_flags_file',
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
    0: ('int32', 'i', 4),
    1: ('int64', 'q', 8),
    2: ('string', 'S', 0),
    3: ('float', 'f', 4),
    4: ('int8', 'b', 1),
    5: ('int16', 'h', 2)
}

_DB_FLAG_STRING   = 0x8 # All strings have this set
_DB_FLAG_UNSIGNED = 0x4 # Possibly ...
_DB_FLAG_UNK      = 0x2 # Numbered fields have this, but not always

# Calculate the negative offset from table_hash to the start, based on the struct info
def compute_field_offset(field):
    offset = 0
    for idx in range(0, len(_DB_STRUCT.format)):
        if _DB_STRUCT_HEADER_FIELDS[idx] == field:
            return offset

        nibble = _DB_STRUCT.format[idx]
        if isinstance(nibble, int):
            nibble = chr(nibble)

        if nibble in ['I', 'i']:
            offset += 4
        elif nibble in ['Q', 'q']:
            offset += 8
        elif nibble in ['H', 'h']:
            offset += 2
        elif nibble in ['B', 'b']:
            offset += 1
        else:
            print('Unknown field format {}'.format(_DB_STRUCT.format[idx]))

    return 0

class DBStructureHeader(collections.namedtuple('DBStructureHeader', _DB_STRUCT_HEADER_FIELDS)):
    def name(self, parser):
        va = self.file_offset(parser, 'va_name')

        null = parser.handle.find(b'\x00', va)

        return parser.handle[va:null].decode('utf-8')

    # VA -> Physical file offset
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

    def data(self, parser, field):
        format_offset = self.file_offset(parser, field)

        unpacker = struct.Struct('i' * self.fields)

        return unpacker.unpack_from(parser.handle, format_offset)

    def formats(self, parser, raw = False):
        format_offset = self.file_offset(parser, 'va_field_format')

        unpacker = struct.Struct('i' * self.fields)

        formats = unpacker.unpack_from(parser.handle, format_offset)
        if raw:
            return formats

        flags = unpacker.unpack_from(parser.handle, self.file_offset(parser, 'va_flags'))

        struct_formats = []
        for i in range(0, len(formats)):
            format_value = formats[i]
            flag_value = flags[i]

            if format_value not in _DB_FIELD_FORMATS:
                logging.warn('Format value type %s not known', format_value)
                struct_formats.append(('unknown', 'z'))
            else:
                fmt = _DB_FIELD_FORMATS[format_value]
                if flag_value & _DB_FLAG_UNSIGNED:
                    fmt = ('u' + fmt[0], fmt[1], fmt[2])

                struct_formats.append(fmt)

        return struct_formats

    def elements(self, parser):
        element_offset = self.file_offset(parser, 'va_elements')

        unpacker = struct.Struct('i' * self.fields)

        return unpacker.unpack_from(parser.handle, element_offset)

    def flags(self, parser):
        flags_offset = self.file_offset(parser, 'va_flags')

        unpacker = struct.Struct('I' * self.fields)

        return unpacker.unpack_from(parser.handle, flags_offset)

    def offsets(self, parser):
        byte_offset = self.file_offset(parser, 'va_byte_offset')

        unpacker = struct.Struct('I' * self.fields)

        return unpacker.unpack_from(parser.handle, byte_offset)

    def valid(self):
        # Ensure fields even have valid values
        fields_non_zero = 0 not in [self.fields, self.size, self.file_data_id,
                self.va_byte_offset, self.va_elements, self.va_field_format,
                self.va_flags, self.va_elements_file,
                self.va_field_format_file, self.va_flags_file, self.table_hash,
                self.layout_hash]

        # Then, some sensible bounds checking for selected fields
        if self.size > 65536:
            return False

        if self.fields > 512:
            return False

        if self.layout_hash < 0xff:
            return False

        if self.table_hash < 0xff:
            return False

        if self.file_data_id > 10000000:
            return False

        return fields_non_zero

    def __str__(self):
        s = []

        for idx in range(0, len(self._fields)):
            ss = '{}={}'.format(
                _DB_STRUCT_HEADER_FIELDS[idx],
                _DB_STRUCT_HEADER_FORMATS[idx].format(self[idx]))
            s.append(ss)

        return ', '.join(s)

class PeStructParser:
    def __init__(self, options, wow_binary, files = None):
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

    def find_db_header_by_name(self, name):
        name_bytes = name.encode('utf-8') + b'\x00'
        # Find the absolute offset to the file name in .rdata
        ofs_name = self.handle.find(name_bytes, self.rdata_start, self.rdata_end)
        while ofs_name < self.rdata_end:
            if ofs_name == -1:
                return None

            logging.debug('{} found at {:#08x}, looking for db2 meta structure ...'.format(name, ofs_name))

            # Compute relative offset inside .rdata
            relative_offset = ofs_name
            relative_offset -= self.rdata_start
            relative_offset += self.rdata_voffset
            relative_offset += self.image_base

            # Look for the relative offset value in the .rdata section, could be before or past the name (cstring) offset
            rofs_bytes = relative_offset.to_bytes(8, byteorder = 'little')
            ofs_db2_meta = self.handle.find(rofs_bytes, self.rdata_start, ofs_name)
            # Nothing on [.rdata_start, ofs_name]
            if ofs_db2_meta == -1:
                ofs_db2_meta = self.handle.find(rofs_bytes, ofs_name + len(name_bytes), self.rdata_end)

            # Nothing on [ofs_name + len(name), .rdata_end], cannot find DB2 meta struct so bail out
            if ofs_db2_meta == -1:
                ofs_name = self.handle.find(name_bytes, ofs_name + len(name_bytes), self.rdata_end)
                logging.debug('Relative offset {:#016x} not found in .rdata, continuing ...'.format(relative_offset))
                continue

            # DB2 meta structure found, unpack and return
            unpack = _DB_STRUCT.unpack_from(self.handle, ofs_db2_meta)
            db2meta = DBStructureHeader(*unpack)
            if not db2meta.valid():
                logging.debug('DB2 meta structure contains invalid information, continuing...')
                ofs_name = self.handle.find(name_bytes, ofs_name + len(name_bytes), self.rdata_end)
                continue

            return db2meta

        return None

    def find_db_header(self, dbcfile):
        table_hash = struct.pack('I', dbcfile.parser.table_hash)
        layout_hash = struct.pack('I', dbcfile.parser.layout_hash)

        # Offset of table hash in the DB structure header from the start
        table_hash_offset = compute_field_offset('table_hash')

        header_start = 0
        search_offset = self.rdata_start
        while search_offset < self.rdata_end + 8:
            # First, find the table hash
            th_offset = self.handle.find(table_hash, search_offset, self.rdata_end)
            if th_offset == -1:
                return False

            # Then the layout hash should be at th_offset + 8 (one 4 byt efield inbetween)
            if self.handle[th_offset + 8:th_offset + 12] != layout_hash:
                search_offset = th_offset + 4
                continue

            logging.info(('Found {} layout_hash={:#8x}, table_hash={:#8x} at '
                          'offset {}, DB Structure start at {}').format(
                os.path.basename(dbcfile.file_name), dbcfile.parser.table_hash,
                dbcfile.parser.layout_hash,
                th_offset, th_offset - table_hash_offset
            ))

            header_start = th_offset - table_hash_offset
            break

        if header_start == 0:
            return None

        unpack = _DB_STRUCT.unpack_from(self.handle, header_start)
        # Header must match table and layout hashes

        return DBStructureHeader(*unpack)

    def find_by_name(self, name):
        header = self.find_db_header_by_name(name)
        if not header:
            return False

        logging.debug('{:s} ({:d}): {:s}'.format(header.name(self), header.file_offset(self, 'va_name'), str(header)))
        logging.debug('Offsets   {}'.format(header.offsets(self)))
        logging.debug('Types     {}'.format(header.formats(self, True)))
        logging.debug('Types2    {}'.format(header.data(self, 'va_field_format_file')))
        logging.debug('Elements  {}'.format(header.elements(self)))
        logging.debug('Elements2 {}'.format(header.data(self, 'va_elements_file')))
        logging.debug('Flags     {}'.format(header.flags(self)))
        logging.debug('Flags2    {} ({})'.format(header.data(self, 'va_flags_file'), header.file_offset(self, 'va_flags_file')))

        offsets = header.offsets(self)
        formats = header.formats(self)
        elements = header.elements(self)

        logging.info('{}.db2: fields={}, record_size={}, file_data_id={}, table_hash={:#08x}, layout_hash={:#08x}'.format(
            name, header.fields, header.size, header.file_data_id, header.table_hash, header.layout_hash))

        fields_str = []
        offsets_str = []
        for i in range(0, len(offsets)):
            if elements[i] > 1:
                fields_str.append('{}[{}]'.format(formats[i][0], elements[i]))
            else:
                fields_str.append('{}'.format(formats[i][0]))

            chars = len(fields_str[-1])
            offset_fmt = '{: <%dd}' % chars
            offsets_str.append(offset_fmt.format(offsets[i]))

        logging.info('Offsets: %s', ' '.join(offsets_str))
        logging.info('Fields : %s', ' '.join(fields_str))

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

        header = self.find_db_header(dbfile)
        if not header:
            logging.debug('No DB structure header found for {}'.format(dbfile.class_name()))
            return True

        logging.debug('{:s} ({:d}): {:s}'.format(header.name(self), header.file_offset(self, 'va_name'), str(header)))
        logging.debug('Offsets   {}'.format(header.offsets(self)))
        logging.debug('Types     {}'.format(header.formats(self, True)))
        logging.debug('Types2    {}'.format(header.data(self, 'va_field_format_file')))
        logging.debug('Elements  {}'.format(header.elements(self)))
        logging.debug('Elements2 {}'.format(header.data(self, 'va_elements_file')))
        logging.debug('Flags     {}'.format(header.flags(self)))
        logging.debug('Flags2    {} ({})'.format(header.data(self, 'va_flags_file'), header.file_offset(self, 'va_flags_file')))

        formats = header.formats(self)
        elements = header.elements(self)

        if len(formats) != len(field_formats):
            logging.warn('Field count mismatch for {}, binary={}, format_file={}'.format(
                dbfile.class_name(), len(formats), len(field_formats)))

        for field_idx in range(0, len(formats)):
            col = dbfile.parser.column(field_idx)
            field_file_format = field_formats[field_idx].data_type.lower()

            # Ensure formats match (sizes, really), without checking
            # signedness. Signedness can be checked only for bit-packed fields.
            # Note that for direct bit-packed fields, the internal
            # representation of the field is what the format file for simc
            # should have. The actual data size in the db2 file is in the field
            # width.
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

    def generate_format(self, name):
        header = self.find_db_header_by_name(name)
        if not header:
            logging.error('No DB structure header found for {}'.format(name))
            return False

        formats = header.formats(self)
        elements = header.elements(self)

        fields = []
        for i in range(0, len(formats)):
            if elements[i] > 1:
                fields.append({'data_type': formats[i][1], 'field': 'f_{}'.format(i + 1), 'elements': elements[i]})
            else:
                fields.append({'data_type': formats[i][1], 'field': 'f_{}'.format(i + 1)})

        return fields

    # Generate should not require knowing any formats
    def generate(self):
        self.options.raw = True

        self.generated_formats = {}

        for dbfile in self.dbfiles:
            f = self.generate_format(dbfile)
            if not f:
                continue

            self.generated_formats[dbfile] = f

        return self.generated_formats
