import logging, math, collections


from struct import Struct

from bitarray import bitarray

from dbc.parser import LegionWDBParser, _DB_HEADER_1, _DB_HEADER_2

_WDC1_HEADER = Struct('<IIIIIIIII')
_WDC1_KEY_HEADER = Struct('<III')

_WDC1_COLUMN_INFO = Struct('<HHIIIII')

_WDC1_COLUMN_DATA = Struct('<I')

_COLUMN_INFO = Struct('<hh')

# Column size can be found from the separate column information block
WDC1_SPECIAL_COLUMN = 32

# Packed column types

# A normal bit-packed column, value is used as-is
COLUMN_TYPE_BIT     = 1

# Contents of the column index the sparse data block, the ID is used to index this data
COLUMN_TYPE_SPARSE  = 2

# Contents of the column directly index a column data block in the db2 file,
# column data can contain multiple sub-blocks (for different columns).
COLUMN_TYPE_INDEXED = 3

# Contents of the column is an array of values, array entries are 4 bytes each
COLUMN_TYPE_ARRAY   = 4

def get_struct_type(is_float, is_signed, bit_size):
    if is_float:
        return 'f'

    if bit_size == 64:
        base_type = 'q'
    elif bit_size == 32:
        base_type = 'i'
    elif bit_size == 16:
        base_type = 'h'
    elif bit_size == 8:
        base_type = 'b'
    else:
        logging.error('Invalid bit length %u', bit_size)
        raise NotImplementedError

    if not is_signed:
        base_type = base_type.upper()

    return base_type

# Creates a data decored for the packed bitfields, based on the extended column
# information type
def get_decoder(column):
    if column.ext_data().block_type() == COLUMN_TYPE_BIT:
        return WDC1BitPackedValue(column)
    elif column.ext_data().block_type() == COLUMN_TYPE_SPARSE:
        return WDC1SparseDataValue(column)
    elif column.ext_data().block_type() == COLUMN_TYPE_INDEXED:
        return WDC1ColumnDataValue(column)
    elif column.ext_data().block_type() == COLUMN_TYPE_ARRAY:
        return WDC1ArrayDataValue(column)

    return None

# Parse a segment of the record data, and return a tuple containing the column data
class WDC1SegmentParser:
    def __init__(self, record_parser, columns):
        if isinstance(columns, collections.Iterable):
            self.columns = columns
        else:
            self.columns = [ columns ]

        self.record_parser = record_parser

        # Index for the id column inside the segment, if it exists
        self.id_index = -1
        self.parser = self.columns[0].parser()

    # Size of the data segment.
    # Note, this is the size of the actual record data segment in bytes, not
    # necessarily the field length (which due to the new format can be
    # anything)
    def size(self):
        raise NotImplementedError

    def __str__(self):
        fields = []
        for column in self.columns:
            fields.append(column.short_type())

        return ', '.join(fields)

    # Parse columns out, return a tuple
    # id: dbc id (as in, the record's id column value)
    # data: (full) data buffer to unpack value from
    # offset: absolute byte offset to the beginning of the segment in data
    # bytes_left: bytes left in the record
    def __call__(self, id, data, offset, bytes_left):
        raise NotImplementedError

# Parse a block of data from a record using the python struct module
class WDC1StructSegmentParser(WDC1SegmentParser):
    def __init__(self, record_parser, columns, expanded_data = False):
        super().__init__(record_parser, columns)

        total_columns = 0

        fmt = '<'
        for column in self.columns:
            column_type = column.struct_type()
            if expanded_data:
                if column.is_signed():
                    column_type = 'i'
                elif not column.is_float():
                    column_type = 'I'

            fmt += column.elements() * column_type

            if not self.parser.has_id_block() and column.index() == self.parser.id_index:
                self.id_index = total_columns

            total_columns += column.elements()

        self.unpacker = Struct(fmt)

    # Size of the unpacker for columns
    def size(self):
        return self.unpacker.size

    def __call__(self, id, data, offset, bytes_left):
        unpacked_data = self.unpacker.unpack_from(data, offset)

        if self.id_index > -1:
            self.record_parser.set_record_id(unpacked_data[self.id_index])

        return unpacked_data

# Parse a block of data from a record that is based on the new packed bit format
class WDC1PackedBitSegmentParser(WDC1SegmentParser):
    def __init__(self, record_parser, columns):
        super().__init__(record_parser, columns)

        self.decoders = []
        # Generate decoders for the packed bit fields
        for column in self.columns:
            decoder = get_decoder(column)
            if not decoder:
                logging.error('%s could not create column decoder for block type %s',
                    record_parser.parser().full_name(), column)
                raise NotImplementedError

            self.decoders.append(decoder)

            if not self.parser.has_id_block() and column.index() == self.parser.id_index:
                self.id_index = len(self.decoders) - 1

    # Note, presumes that all bit packed fields are going to be parsed in a
    # single WDC1PackedBitSegmentParser. Should be a safe assumption, they seem
    # to be always inserted at the end of the DB2 file in a continuous segment.
    def size(self):
        return math.ceil(sum([c.ext_data().bit_size() for c in self.columns]) / 8)

    def __call__(self, id, data, offset, bytes_left):
        barr = bitarray(endian = 'little')

        # Read enough bytes to the bitarray from the segment
        barr.frombytes(data[offset:offset + self.size()])

        parsed_data = ()
        bit_offset = 0
        for idx in range(0, len(self.columns)):
            column = self.columns[idx]
            decoder = self.decoders[idx]
            size = column.ext_data().bit_size()

            raw_value = barr[bit_offset:bit_offset + size].tobytes()

            parsed_data += decoder(id, data, raw_value)

            bit_offset += size

            # Setup the record id for subsequent decoders in this segment, as
            # well as the rest of the segments that follow (note, should not
            # really happen as bit-packed columns are all handled currently in
            # a single segment)
            if idx == self.id_index:
                id = parsed_data[-1]
                self.record_parser.set_record_id(id)

        return parsed_data

class WDC1StringSegmentParser(WDC1SegmentParser):
    def __init__(self, record_parser, columns):
        super().__init__(record_parser, columns)

        self.string_size = 0

    # Size of the inline string, needs to be computed in association with the parse call
    def size(self):
        return self.string_size

    # Returns the offset into the file, where the string begins, much like what
    # the normal string block fields do
    def __call__(self, id, data, offset, bytes_left):
        # Find first \x00 byte
        pos = data.find(b'\x00', offset, offset + bytes_left)
        self.string_size = (pos - offset) + 1

        # Inline strings with first character 0 denotes disabled field
        if data[offset] == 0:
            return (0,)
        else:
            return (offset,)

# Bit-packed column generic class, how data is parsed depends on the meta type
# (column struct type)
class WDC1ExtendedColumnValue:
    def __init__(self, column):
        self.column = column

    def __call__(self, id, data, bytes_):
        raise NotImplementedError

class WDC1BitPackedValue(WDC1ExtendedColumnValue):
    def __init__(self, column):
        super().__init__(column)

        self.mask = 0
        for idx in range(0, self.column.ext_data().bit_size()):
            self.mask |= (1 << idx)

    def __call__(self, id_, data, bytes_):
        value = int.from_bytes(bytes_, byteorder = 'little')

        if self.column.is_signed():
            is_negative = (value & (1 << self.column.ext_data().bit_size() - 1)) != 0
            if is_negative:
                value = -((value ^ self.mask) + 1)

        return (value,)

# Gets a value from the data file's column data block
#
# TODO: Column block values are always 4 bytes?
class WDC1ColumnDataValue(WDC1ExtendedColumnValue):
    def __init__(self, column):
        super().__init__(column)

        self.unpacker = Struct('<' + column.struct_type())

        # We need to compute the column data offset for this column
        column_data_offset = 0
        for column_idx in range(0, self.column.parser().n_fields()):
            if column_idx == self.column.index():
                break

            other_column = self.column.parser().column(column_idx)
            if other_column.ext_data().block_type() in [COLUMN_TYPE_INDEXED, COLUMN_TYPE_ARRAY]:
                column_data_offset += other_column.ext_data().block_size()

        self.base_offset = self.column.parser().column_data_block_offset + column_data_offset
        logging.debug('%s column data for %s at base offset %d',
            self.column.parser().full_name(), self.column, self.base_offset)

    def __call__(self, id_, data, bytes_):
        # Convert the bytes to a proper int value (our relative id into the column data block).
        ptr_ = int.from_bytes(bytes_, byteorder = 'little')

        # Value is the {ptr_}th value in the block
        value_offset = self.base_offset + ptr_ * 4

        return self.unpacker.unpack_from(data, value_offset)

class WDC1SparseDataValue(WDC1ExtendedColumnValue):
    def __init__(self, column):
        super().__init__(column)

        self.unpacker = Struct('<' + column.struct_type())

    # Bytes are not needed for sparse data, sparse blocks are directly indexed
    # with the id column of the record
    def __call__(self, id_, data, bytes_):
        # Value is the id_th value in the block
        value_offset = self.column.parser().sparse_data_offset(self.column, id_)
        if value_offset == -1:
            return (0,)

        # Return the value
        return self.unpacker.unpack_from(data, value_offset)

class WDC1ArrayDataValue(WDC1ColumnDataValue):
    def __init__(self, column):
        super().__init__(column)

        self.unpacker = Struct('<' + column.struct_type())

        self.__array_size = 4 * self.column.ext_data().elements()

        struct_type = get_struct_type(column.is_float(), column.is_signed(), 32)
        self.unpacker = Struct('<{}{}'.format(column.ext_data().elements(), struct_type))

        # TODO: if element_bit_size is < 32, should we mask out bits? Do the
        # files even indicate what the element size is (cell size?)

    def __call__(self, id_, data, bytes_):
        # Convert the bytes to a proper int value (our id into the block)
        ptr_ = int.from_bytes(bytes_, byteorder = 'little')

        value_offset = self.base_offset + ptr_ * self.__array_size

        if value_offset > self.base_offset + self.column.ext_data().block_size():
            logging.error('%s array column exceeds block size, offset %d, max %s',
                self.column.parser().full_name(), value_offset, self.base_offset + self.column.ext_data().block_size())
            raise IndexError

        return self.unpacker.unpack_from(data, value_offset)

class RecordParser:
    def __init__(self, parser, cache = False):
        self._parser = parser

        # Record segment decoders
        self._decoder = []

        # Iterator state
        self._record_id = 0
        self.__dbc_id = -1

        self._cache = cache

        self._n_records = parser.records
        self._base_offset = parser.data_offset
        self._record_size = parser.record_size
        self._data = parser.data

        self.build_decoder()

    def __iter__(self):
        return self

    def __next__(self):
        if self._record_id == self._n_records:
            raise StopIteration

        # Base offset into the record start
        base_offset = self._base_offset + self._record_id * self._record_size

        if self.__parser.has_id_block():
            self.__dbc_id, _, _ = self._parser.get_record_info(self._record_id)
        else:
            self.__dbc_id = -1

        self._record_id += 1

        # Byte offset into current record, incremented after each segment parser
        record_offset = 0

        data = ()
        for decoder in self.__decoders:
            segment_offset = base_offset + record_offset
            bytes_left = self.__parser.record_size - record_offset

            data += decoder(self.__dbc_id, self.__data, segment_offset, bytes_left)

            record_offset += decoder.size()

        return data

    # Support old interface as well as the new iterator-based one
    def __call__(self, dbc_id, data, offset, size):
        # Byte offset into current record, incremented after each segment parser
        record_offset = 0

        parsed_data = ()
        for decoder in self._decoders:
            segment_offset = offset + record_offset
            bytes_left = size - record_offset

            # We need to be able to support WDC1 files where the id is given as
            # a static column, instead of just an id block. In this case, the
            # dbc_id passed to the old-style iteration will be -1, as the data
            # table holding record offsets and such do not know what the ID is.
            if self.__dbc_id > -1:
                dbc_id = self.__dbc_id

            parsed_data += decoder(dbc_id, data, segment_offset, bytes_left)

            record_offset += decoder.size()

        return parsed_data

    # Called from the segment parsers to set the id of the record for the rest
    # of the parsing process
    def set_record_id(self, id_):
        self.__dbc_id = id_

    def __str__(self):
        return ', '.join([ str(decoder) for decoder in self._decoders ])

    def parser(self):
        return self._parser

# A specialized parser that considers all record fields to be expanded to 4 bytes
class ExpandedRecordParser(RecordParser):
    def build_decoder(self):
        columns = []
        decoders = []

        # Expanded parser always has the id column at the start, if the actual
        # db2 file has an id block
        if self.parser().has_id_block():
            columns.append(WDC1ProxyColumn(self.parser(), None, 0, 0, 0))

        # Then, iterate over the columns. Regardless of what sort of colun type
        # the original file has, expanded data is always normal struct form, 4
        # (or 8?) bytes per field
        for column_idx in range(0, self.parser().n_fields()):
            column = self.parser().column(column_idx)

            # Hotfixed columns always have inlined strings
            if column.is_string() and (self.parser().has_offset_map() or self._cache):
                if len(columns) > 0:
                    decoders.append(WDC1StructSegmentParser(self, columns, expanded_data = True))
                    columns = []

                decoders.append(WDC1StringSegmentParser(self, column))

            # Presume that bitpack is a continious segment that runs to the end of the record
            else:
                columns.append(column)

        # Aand expanded parser also has the key block value at the end, if the real db2 file has it
        if self.parser().has_key_block():
            columns.append(WDC1ProxyColumn(self.parser(), None, 0, 0, 0))

        # Build decoders when all columns are looped in
        if len(columns) > 0:
            decoders.append(WDC1StructSegmentParser(self, columns, expanded_data = True))

        self._decoders = decoders

# Iterator to spit out values from the record block
class WDC1RecordParser(RecordParser):
    # Build a decoder for the base data record
    def build_decoder(self):
        struct_columns = []
        bitpack_columns = []

        decoders = []

        for column_idx in range(0, self.parser().n_fields()):
            column = self.parser().column(column_idx)

            # Hotfixed columns always have inlined strings
            if column.is_string() and (self.parser().has_offset_map() or self._cache):
                if len(struct_columns) > 0:
                    decoders.append(WDC1StructSegmentParser(self, struct_columns))
                    struct_columns = []

                decoders.append(WDC1StringSegmentParser(self, column))

            # Presume that bitpack is a continious segment that runs to the end of the record
            elif column.size_type() == WDC1_SPECIAL_COLUMN:
                # Bit-packed data (special columns) seem to be expanded for cache files
                if self._cache:
                    struct_columns.append(column)
                else:
                    bitpack_columns.append(column)
            else:
                struct_columns.append(column)

        # Build decoders when all columns are looped in
        if len(struct_columns) > 0:
            decoders.append(WDC1StructSegmentParser(self, struct_columns))

        # Presume the record ends with a bit-packed set of columns
        if len(bitpack_columns) > 0:
            decoders.append(WDC1PackedBitSegmentParser(self, bitpack_columns))

        self._decoders = decoders

class WDC1ExtendedColumn:
    def __init__(self, column, data):
        self.__column = column

        # Data is a tuple containing all fields, unpack into values
        self.__record_offset = data[0]
        self.__size = data[1]
        self.__block_size = data[2]
        self.__block_type = data[3]
        self.__pack_offset = data[4]
        self.__element_size = data[5]
        self.__elements = data[6]

    def record_offset(self):
        return self.__record_offset

    def bit_size(self):
        return self.__size

    def block_size(self):
        return self.__block_size

    def block_type(self):
        return self.__block_type

    def element_bit_size(self):
        return self.__element_size

    def elements(self):
        return self.__elements

    def pack_offset(self):
        return self.__pack_offset

    def __str__(self):
        return 'Ext[ r_offset={}, size={}, b_size={}, b_type={}, p_offset={}, e_size={}, elements={} ]'.format(
            self.__record_offset, self.__size, self.__block_size, self.__block_type, self.__pack_offset,
            self.__element_size, self.__elements
        )

# A representation of a data column type in a WDC1 file
class WDC1Column:
    def __init__(self, parser, fobj, index, size_type, offset):
        self.__parser = parser
        self.__format = fobj
        self.__index = index
        self.__size_type = size_type
        self.__offset = offset
        self.__ext = None

    def ext_data(self, tuple_ = None):
        if not tuple_:
            return self.__ext

        self.__ext = WDC1ExtendedColumn(self, tuple_)

    def bit_size(self):
        if self.__size_type in [0, 8, 16, 24, -32]:
            return (32 - self.__size_type)
        elif self.__ext.block_type() in [COLUMN_TYPE_SPARSE, COLUMN_TYPE_INDEXED, COLUMN_TYPE_ARRAY]:
            return 32
        else:
            return self.__ext.bit_size()

    def elements(self):
        bit_size_ = self.bit_size()

        field_width_ = self.ext_data().bit_size()
        elements_ = self.ext_data().elements()
        type_ = self.ext_data().block_type()

        if elements_ > 0:
            return elements_
        # TODO: Can these special columns have more fields?
        elif type_ == COLUMN_TYPE_INDEXED:
            return 1
        elif type_ == COLUMN_TYPE_SPARSE:
            return 1
        else:
            return field_width_ // bit_size_

    def parser(self):
        return self.__parser

    def index(self):
        return self.__index

    def size_type(self):
        return self.__size_type

    def size(self):
        return self.ext_data().bit_size()

    def offset(self):
        return self.__offset

    def format(self):
        return self.__format

    def is_float(self):
        return self.__format and self.__format.data_type == 'f' or False

    def is_signed(self):
        if self.__format:
            return self.__format.data_type in ['b', 'h', 'i']
        else:
            return True

    def is_string(self):
        if self.__format:
            return self.__format.data_type == 'S'
        else:
            return False

    def struct_type(self):
        if not self.__format:
            return get_struct_type(False, True, self.bit_size())
        else:
            return self.__format and self.__format.data_type.replace('S', 'I')

    def short_type(self):
        if self.is_float():
            base_type = 'float'
        elif self.is_string():
            base_type = 'string'
        elif not self.is_signed():
            base_type = 'uint'
        else:
            base_type = 'int'

        bit_size = self.bit_size()

        elements = 1
        extra_type = ''
        if self.ext_data().block_type() == COLUMN_TYPE_SPARSE:
            extra_type = 's'
        elif self.ext_data().block_type == COLUMN_TYPE_INDEXED:
            extra_type = 'i'


        return '{}:{}{}'.format(
            base_type, bit_size,
            elements > 1 and '[{}]'.format(elements) or (len(extra_type) and '[{}]'.format(extra_type) or '')
        )

    def __str__(self):
        return 'Column[ idx={}, offset={}, size={}, elements={}{} ]'.format(
            self.__index, self.__offset, self.bit_size(), self.elements(),
            self.ext_data().block_type() != 0 and (', info=' + str(self.ext_data())) or ''
        )

# A faked colum representing a 32-bit value in the record. Used in association
# with expanded parsers, where the record id and the potential key block id
# (parent id) is included in the record data.
class WDC1ProxyColumn(WDC1Column):
    def bit_size(self):
        return 32

    def elements(self):
        return 1

    def size_type(self):
        return 0

    def size(self):
        return 32

    def offset(self):
        return 0

    def format(self):
        return None

    def is_float(self):
        return False

    def is_signed(self):
        return False

    def is_string(self):
        return False

    def struct_type(self):
        return 'I'

    def short_type(self):
        return 'uint:32'

    def __str__(self):
        return 'Column[ idx={}, offset={}, size={}, elements={} ]'.format(
            self.__index, self.__offset, self.bit_size(), self.elements()
        )


class WDC1Parser(LegionWDBParser):
    def is_magic(self): return self.magic == b'WDC1'

    def __init__(self, options, fname):
        super().__init__(options, fname)

        # Lazy-computed key format for the foreign key values
        self.__key_format = None
        self.__key_high = -1

        # New fields
        self.n_columns = 0
        self.packed_data_offset = 0
        self.wdc1_unk3 = 0
        self.offset_map_offset = 0
        self.id_block_size = 0
        # More specific column information for WDC1 fields
        self.column_info_block_size = 0
        self.sparse_block_size = 0
        self.column_data_block_size = 0
        self.key_block_size = 0

        # Computed offsets
        self.column_info_block_offset = 0
        self.sparse_block_offfset = 0
        self.column_data_block_offset = 0
        self.key_block_offset = 0

        # Column information
        self.column_info = []

        # Sparse block data, preprocess if present so we can have fast lookups during parsing
        self.sparse_blocks = []

        # Foreign key block data, just a bunch of ids (on a per record basis)
        self.key_block = []

    def n_fields(self):
        return self.n_columns

    def column(self, idx):
        return self.column_info[idx]

    def key(self, record_id):
        if len(self.key_block) == 0:
            return -1

        return self.key_block[record_id]

    # Note, wdb_name is ignored, as WDC1 parsers are always bound to a single
    # wdb file (e.g., Spell.db2)
    def key_format(self, wdb_name = None):
        if self.key_block_size == 0:
            return super().key_format()

        if self.__key_format:
            return self.__key_format

        n_digits = int(math.log10(self.__key_high) + 1)
        self.__key_format = '%%%uu' % n_digits
        return self.__key_format

    def sparse_data_offset(self, column, id_):
        return self.sparse_blocks[column.index()].get(id_, -1)

    # TODO: Some validation can, and should be done
    def validate_format(self):
        return True

    def create_raw_parser(self):
        return WDC1RecordParser(self)

    def create_formatted_parser(self, inline_strings = False, cache_parser = False, expanded_parser = False):
        formats = self.fmt.objs(self.class_name(), True)

        if len(formats) != self.n_fields():
            logging.error('%s field count mismatch, format: %u, file: %u',
                self.full_name(), len(formats), self.n_fields())
            return None

        if expanded_parser:
            parser = ExpandedRecordParser(self)
        else:
            parser = WDC1RecordParser(self, cache = cache_parser)

        logging.debug('Unpacking plan for %s: %s', self.full_name(), parser)

        return parser

    # Compute offset into the file, based on what blocks we have
    def __compute_offsets(self):
        # Offset immediately after the header (static + basic column info).
        # Contains the base record data
        self.data_offset = self.parse_offset

        block_offset = self.data_offset

        if self.has_offset_map():
            # String block follows immediately after the base data block
            self.string_block_offset = 0
            running_offset = self.offset_map_offset + (self.last_id - self.first_id + 1) * 6
        else:
            # String block follows immediately after the base data block
            self.string_block_offset = self.data_offset + self.record_size * self.records
            running_offset = self.string_block_offset + self.string_block_size

        # Next, ID block follows immmediately after string block
        self.id_block_offset = running_offset
        running_offset = self.id_block_offset + self.id_block_size

        # Then, a possible clone block offset
        self.clone_block_offset = running_offset
        running_offset = self.clone_block_offset + self.clone_segment_size

        # Next, extended column information block
        self.column_info_block_offset = running_offset
        running_offset = self.column_info_block_offset + self.column_info_block_size

        # Followed by the column-specific data block
        self.column_data_block_offset = running_offset
        running_offset = self.column_data_block_offset + self.column_data_block_size

        # Then, the sparse block
        self.sparse_block_offset = running_offset
        running_offset = self.sparse_block_offset + self.sparse_block_size

        # And finally, the "foreign key" block for the whole file
        self.key_block_offset = running_offset

        return True

    # Parse column information (right after static header)
    def __parse_basic_column_info(self):
        if not self.options.raw:
            formats = self.fmt.objs(self.class_name(), True)
        else:
            formats = None

        for idx in range(0, self.n_fields()):
            size_type, offset = _COLUMN_INFO.unpack_from(self.data, self.parse_offset)

            self.column_info.append(WDC1Column(self, (formats and len(formats) > idx) and formats[idx] or None, idx, size_type, offset))

            self.parse_offset += _COLUMN_INFO.size

        logging.debug('Parsed basic column info')

        return True

    # Parse extended column information to supplement the basic one, if it exists
    def __parse_extended_column_info(self):
        if self.column_info_block_size == 0:
            return True

        if self.column_info_block_size != self.n_fields() * _WDC1_COLUMN_INFO.size:
            log.error('Extended column info size mismatch, expected {}, got {}',
                    self.n_fields() * _WDC1_COLUMN_INFO.size, self.column_info_block_size)
            return False

        offset = self.column_info_block_offset
        for idx in range(0, self.n_fields()):
            data = _WDC1_COLUMN_INFO.unpack_from(self.data, offset)

            self.column_info[idx].ext_data(data)

            offset += _WDC1_COLUMN_INFO.size

        return True

    def __parse_sparse_blocks(self):
        if not self.has_sparse_blocks():
            return True

        offset = self.sparse_block_offset

        for column_idx in range(0, self.n_fields()):
            block = {}
            column = self.column(column_idx)

            # TODO: Are sparse blocks always <dbc_id, value> tuples with 4 bytes for a value?
            # TODO: Do we want to preparse the sparse block? Would save an
            # unpack call at the cost of increased memory
            if column.ext_data().block_type() == COLUMN_TYPE_SPARSE:
                if column.ext_data().block_size() == 0 or column.ext_data().block_size() % 8 != 0:
                    logging.error('%s unknown sparse block type for column %s',
                        self.full_name(), column)
                    return False

                logging.debug('%s unpacking sparse block for %s at %d, %d entries',
                    self.full_name(), column, offset, column.ext_data().block_size() // 8)

                unpack_full_str = '<' + ((column.ext_data().block_size() // 8) * 'I4x')
                unpacker = Struct(unpack_full_str)
                value_index = 0
                for dbc_id in unpacker.unpack_from(self.data, offset):
                    # Store <dbc_id, offset> tuples into the sparse block
                    block[dbc_id] = offset + value_index * 8 + 4

                    value_index += 1

                offset += column.ext_data().block_size()

            self.sparse_blocks.append(block)

        return True

    def __parse_key_block(self):
        if not self.has_key_block():
            return True

        if (self.key_block_size - _WDC1_KEY_HEADER.size) % 8 != 0:
            logging.error('%s invalid key block size, expected divisible by 8, got %u (%u)',
                self.full_name(), self.key_block_size, self.key_block_size % 8)
            return False

        offset = self.key_block_offset
        records, min_id, max_id = _WDC1_KEY_HEADER.unpack_from(self.data, offset)

        if records != self.records:
            logging.error('%s invalid number of keys, expected %u, got %u',
                    self.full_name(), self.records, records)
            return False

        offset += _WDC1_KEY_HEADER.size

        # Just make a sparse table for this
        self.key_block = [ 0 ] * self.records

        # Value, record id pairs
        unpacker = Struct('II')
        for index in range(0, self.records):
            value, record_id = unpacker.unpack_from(self.data, offset + index * unpacker.size)
            self.key_block[record_id] = value

            if value > self.__key_high:
                self.__key_high = value

        logging.debug('%s parsed %u keys', self.full_name(), self.records)

        return True

    def build_field_data(self):
        return True

    def raw_outputtable(self):
        return True

    def has_clone_block(self):
        return self.clone_segment_size > 0

    def has_key_block(self):
        return self.key_block_size > 0

    def has_sparse_blocks(self):
        return self.sparse_block_size > 0

    def has_id_block(self):
        return self.id_block_size > 0

    def has_offset_map(self):
        return self.offset_map_offset > 0

    def has_string_block(self):
        return self.string_block_size > 2

    def parse_header(self):
        if not super().parse_header():
            return False

        self.table_hash, self.layout_hash, self.first_id = _DB_HEADER_1.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _DB_HEADER_1.size

        self.last_id, self.locale, self.clone_segment_size, self.flags, self.id_index = _DB_HEADER_2.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _DB_HEADER_2.size

        self.n_columns, self.packed_data_offset, self.wdc1_unk3, self.offset_map_offset, \
            self.id_block_size, self.column_info_block_size, self.sparse_block_size, self.column_data_block_size, \
            self.key_block_size = _WDC1_HEADER.unpack_from(self.data, self.parse_offset)
        self.parse_offset += _WDC1_HEADER.size

        if not self.__parse_basic_column_info():
            return False

        if not self.__compute_offsets():
            return False

        if not self.__parse_extended_column_info():
            return False


        return True

    # Need to parse some other stuff too
    def open(self):
        if not super().open():
            return False

        if not self.__parse_sparse_blocks():
            return False

        if not self.__parse_key_block():
            return False

        return True

    def fields_str(self):
        fields = super().fields_str()

        # Offset map offset is printed from LegionWDBParser
        fields.append('layout_hash=%#.8x' % self.layout_hash)
        fields.append('id_index=%u' % self.id_index)
        fields.append('ob_packed_data=%u' % self.packed_data_offset)
        fields.append('wdc1_unk3=%u' % self.wdc1_unk3)
        fields.append('id_block_size=%u' % self.id_block_size)
        fields.append('column_info_block_size=%u' % self.column_info_block_size)
        fields.append('sparse_block_size=%u' % self.sparse_block_size)
        fields.append('column_data_block_size=%u' % self.column_data_block_size)
        fields.append('key_block_size=%u' % self.key_block_size)

        return fields

    def __str__(self):
        s = super().__str__()

        s += '\n'

        s += '\n'.join([ str(c) for c in self.column_info ])

        return s;
