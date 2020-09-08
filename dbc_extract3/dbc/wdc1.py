import logging, math, collections, sys, binascii

from struct import Struct

from bitarray import bitarray

import dbc

from dbc import HeaderFieldInfo, DBCRecordInfo

from dbc.parser import DBCParserBase

X_BITPACKED  = 0x10
X_ID_BLOCK   = 0x04
X_OFFSET_MAP = 0x01

_WDC1_KEY_HEADER = Struct('<III')

_WDC1_COLUMN_INFO = Struct('<HHIIIII')

_WDC1_COLUMN_DATA = Struct('<I')

_COLUMN_INFO = Struct('<hh')
_CLONE_INFO  = Struct('<II')

_ITEMRECORD  = Struct('<IH')

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

# A signed bitpacked column
COLUMN_TYPE_BIT_S   = 5

# Values from bitarray come as unsigned, transform to twos complement signed
# value if the data format indicates a signed column
def transform_sign(value, mask, bit_size):
    if (value & (1 << bit_size - 1)) != 0:
        return -((value ^ mask) + 1)
    else:
        return value

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
    elif bit_size == 0:
        base_type = 'f'
    else:
        logging.error('Invalid bit length %u', bit_size)
        raise NotImplementedError

    if not is_signed:
        base_type = base_type.upper()

    return base_type

# Creates a data decored for the packed bitfields, based on the extended column
# information type
def get_decoder(column):
    if column.field_ext_type() == COLUMN_TYPE_BIT:
        return WDC1BitPackedValue(column)
    elif column.field_ext_type() == COLUMN_TYPE_SPARSE:
        return WDC1SparseDataValue(column)
    elif column.field_ext_type() == COLUMN_TYPE_INDEXED:
        return WDC1ColumnDataValue(column)
    elif column.field_ext_type() == COLUMN_TYPE_ARRAY:
        return WDC1ArrayDataValue(column)
    elif column.field_ext_type() == COLUMN_TYPE_BIT_S:
        return WDC1BitPackedValue(column)

    return None

# A specialized parser for the hotfix file that dynamically parses the
# remaining bytes of the record, based on the magnitude of the existing key
# block info (i.e., the number of bytes required to store the high key id of
# the client data file)
#
# TODO: 24 bits?
class WDC1HotfixKeyIdParser:
    def __init__(self, record_parser, unpack_bytes):
        self.record_parser = record_parser
        self.field_size = unpack_bytes
        self.u32 = Struct('<I')
        self.u16 = Struct('<H')
        self.u8 = Struct('<B')

    def __str__(self):
        return 'uint:key({})'.format(self.field_size * 8)

    def size(self):
        return self.field_size

    def __call__(self, id, data, offset, bytes_left):
        if bytes_left == 4:
            unpacker = self.u32
        elif bytes_left == 3 or bytes_left == 2:
            unpacker = self.u16
        elif bytes_left == 1:
            unpacker = self.u8
        else:
            raise ValueError('Unknown key field length {} in {}'.format(bytes_left,
                self.record_parser.parser()))

        return unpacker.unpack_from(data, offset)

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
    def __init__(self, record_parser, columns):
        super().__init__(record_parser, columns)

        total_columns = 0

        fmt = '<'
        for column in self.columns:
            if self.record_parser._hotfix_parser and \
                dbc.use_hotfix_expanded_field(self.parser.class_name()):
                column_type = dbc.translate_hotfix_expanded_field(column.struct_type())
            else:
                column_type = column.struct_type()

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
        return math.ceil(sum([c.field_bit_size() for c in self.columns]) / 8)

    def __call__(self, id, data, offset, bytes_left):
        barr = bitarray(endian = 'little')

        # Read enough bytes to the bitarray from the segment
        barr.frombytes(data[offset:offset + self.size()])

        parsed_data = ()
        bit_offset = 0
        for idx in range(0, len(self.columns)):
            column = self.columns[idx]
            decoder = self.decoders[idx]
            size = column.field_bit_size()

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

        self.element_mask = 0
        for bit in range(0, self.column.value_bit_size()):
            self.element_mask |= (1 << bit)

        self.cell_mask = 0
        for bit in range(0, self.column.field_bit_size()):
            self.cell_mask |= (1 << bit)

    def __call__(self, id, data, bytes_):
        raise NotImplementedError

class WDC1BitPackedValue(WDC1ExtendedColumnValue):
    def __init__(self, column):
        super().__init__(column)

        self.unpacker = None
        if column.value_bit_size() != 24 and column.value_bit_size() % 8 == 0:
            struct_type = get_struct_type(column.is_float(),
                                          column.is_signed(),
                                          column.value_bit_size())
            self.unpacker = Struct('<{}'.format(struct_type))

    def __call__(self, id_, data, bytes_):
        if self.unpacker:
            value = self.unpacker.unpack_from(bytes_, 0)[0]
        else:
            value = int.from_bytes(bytes_, byteorder = 'little')

            if self.column.is_signed():
                value = transform_sign(value, self.element_mask, self.column.value_bit_size())

        return (value,)

# Gets a value from the data file's column data block
class WDC1ColumnDataValue(WDC1ExtendedColumnValue):
    def __init__(self, column):
        super().__init__(column)

        self.unpacker = Struct('<' + column.struct_type())

        # We need to compute the column data offset for this column
        column_data_offset = 0
        for column_idx in range(0, self.column.parser().fields):
            if column_idx == self.column.index():
                break

            other_column = self.column.parser().column(column_idx)
            if other_column.field_ext_type() in [COLUMN_TYPE_INDEXED, COLUMN_TYPE_ARRAY]:
                column_data_offset += other_column.field_block_size()

        self.base_offset = self.column.parser().column_data_block_offset + column_data_offset
        logging.debug('%s column data for %s at base offset %d',
            self.column.parser().full_name(), self.column, self.base_offset)

    def __call__(self, id_, data, bytes_):
        # Convert the bytes to a proper int value (our relative id into the column data block).
        ptr_ = int.from_bytes(bytes_, byteorder = 'little') & self.cell_mask

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
            return (self.column.default(),)

        return self.unpacker.unpack_from(data, value_offset)

class WDC1ArrayDataValue(WDC1ColumnDataValue):
    def __init__(self, column):
        super().__init__(column)

        bytes_per_element = self.column.format_bit_size() // 8
        # Array values are stored as 32-bit values, however the data type may be <4 bytes
        self.__array_size = 4 * self.column.elements()

        pad_bytes = 4 - bytes_per_element
        pad_str = ''
        if pad_bytes > 0:
            pad_str = '{}x'.format(pad_bytes)

        struct_type = get_struct_type(column.is_float(), column.is_signed(), self.column.format_bit_size())
        element_type = '{data}{pad}'.format(
            data = struct_type,
            pad = pad_str)

        self.unpacker = Struct('<{}'.format(element_type * column.elements()))

    def __call__(self, id_, data, bytes_):
        # Convert the bytes to a proper int value (our id into the block)
        ptr_ = int.from_bytes(bytes_, byteorder = 'little') & self.cell_mask

        value_offset = self.base_offset + ptr_ * self.__array_size

        if value_offset > self.base_offset + self.column.field_block_size():
            logging.error('%s array column exceeds block size, offset %d, max %s',
                self.column.parser().full_name(), value_offset,
                self.base_offset + self.column.field_block_size())
            raise IndexError

        return self.unpacker.unpack_from(data, value_offset)

class RecordParser:
    def __init__(self, parser, hotfix = False):
        self._parser = parser

        # Record segment decoders
        self._decoder = []

        # Iterator state
        self._record_id = 0
        self.__dbc_id = -1

        # Records are sourced from a hotfix file, instead of the normal client data file
        self._hotfix_parser = hotfix

        self._n_records = parser.records
        self._record_size = parser.record_size
        self._data = parser.data

        self.build_decoder()

    def __iter__(self):
        return self

    def __next__(self):
        if self._record_id == self._n_records:
            raise StopIteration

        self.__dbc_id, _, base_offset, size, _, _ = self._parser.get_record_info(self._record_id)
        self._record_id += 1

        # Byte offset into current record, incremented after each segment parser
        record_offset = 0

        data = ()
        for decoder in self.__decoders:
            segment_offset = base_offset + record_offset
            bytes_left = self.__parser.record_size - record_offset

            data += decoder(self.__dbc_id, self.__data, segment_offset, bytes_left)

            decoded = decoder.size();
            if decoded < 0:
                raise ValueError('Decoded negative bytes ({}), record={}, data={}, bytes={}'.format(
                    decoded, self.__dbc_id, parsed_data,
                    binascii.hexlify(self.__data[base_offset:base_offset+size])))

            record_offset += decoded

        unparsed_bytes = size - record_offset

        # Sanity check parsing when offset map is not used
        if not self.parser().has_offset_map() and (unparsed_bytes < 0 or unparsed_bytes > 3):
            raise ValueError('Parse error: parsed_bytes={}, bytes_left={}, record={}, data={}, remains={}'.format(
                record_offset, unparsed_bytes, size, parsed_data,
                binascii.hexlify(data[offset + record_offset:offset + size])))

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

            decoded = decoder.size();
            if decoded < 0:
                raise ValueError('{}: Decoded negative bytes ({}), record={}, data={}, bytes={}'.format(
                    self.parser().class_name(), decoded, dbc_id, parsed_data, binascii.hexlify(data[offset:offset+size])))

            record_offset += decoded

        unparsed_bytes = size - record_offset

        # Sanity check parsing when offset map is not used
        if not self.parser().has_offset_map() and (unparsed_bytes < 0 or unparsed_bytes > 3):
            raise ValueError('Parse error: file={}, offset={}, parsed_bytes={}, bytes_left={}, record={}, id={}, data={}, remains={}'.format(
                self.parser().file_name(), offset, record_offset, unparsed_bytes, size, dbc_id, parsed_data,
                binascii.hexlify(data[offset + record_offset:offset + size])))

        return parsed_data

    # Called from the segment parsers to set the id of the record for the rest
    # of the parsing process
    def set_record_id(self, id_):
        self.__dbc_id = id_

    def __str__(self):
        return ', '.join([ str(decoder) for decoder in self._decoders ])

    def parser(self):
        return self._parser

# Iterator to spit out values from the record block
class WDC1RecordParser(RecordParser):
    # Build a decoder for the base data record
    def build_decoder(self):
        struct_columns = []
        bitpack_columns = []

        decoders = []

        for column_idx in range(0, self.parser().fields):
            column = self.parser().column(column_idx)

            # Hotfixed columns always have inlined strings
            if column.is_string() and (self.parser().has_offset_map() or self._hotfix_parser):
                if len(struct_columns) > 0:
                    decoders.append(WDC1StructSegmentParser(self, struct_columns))
                    struct_columns = []

                decoders.append(WDC1StringSegmentParser(self, column))

            # Presume that bitpack is a continious segment that runs to the end of the record
            elif column.field_base_type() == WDC1_SPECIAL_COLUMN:
                # Bit-packed data (special columns) seem to be expanded for cache files
                if self._hotfix_parser:
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

        # Grab the key block id from the end of the hotfix record, if this
        # hotfix entry is not for one of the client data files where the data
        # is already incorporated into the record itself
        if self._hotfix_parser and self.parser().has_key_block() and \
                not dbc.use_hotfix_key_field(self.parser().class_name()):
            decoders.append(WDC1HotfixKeyIdParser(self, self.parser().key_bytes()))

        self._decoders = decoders

# A representation of a data column type in a WDC1 file
class WDC1Column:
    def __init__(self, parser, fobj, index, size_type, offset):
        self.__parser = parser
        self.__format = fobj
        self.__index = index
        self.__size_type = size_type
        self.__offset = offset
        self.__elements = 1

        self.__bit_offset = 0
        self.__packed_bit_offset = 0
        self.__field_size = 0
        self.__block_size = 0
        self.__block_type = 0
        self.__value_size = 0
        self.__is_signed = False
        self.__default_value = 0

        if not self.__parser.options.raw and not self.__format:
            raise ValueError('{}: Column {} must have a format defined'.format(
                self.__parser.class_name(), index))

    def parser(self):
        return self.__parser

    def format(self):
        return self.__format

    def format_bit_size(self):
        if not self.__format:
            return 0
        elif self.__format.data_type in ['b', 'B']:
            return 8
        elif self.__format.data_type in ['h', 'H']:
            return 16
        elif self.__format.data_type in ['i', 'I', 'f']:
            return 32
        elif self.__format.data_type in ['q', 'Q']:
            return 64
        else:
            return 0

    def index(self):
        return self.__index

    def set_ext_data(self, ext_data):
        self.__bit_offset = ext_data[0]
        self.__field_size = ext_data[1]
        self.__block_size = ext_data[2]
        self.__block_type = ext_data[3]

        # Conditional fields
        if self.__block_type == COLUMN_TYPE_BIT:
            self.__packed_bit_offset = ext_data[4]
            self.__value_size = ext_data[5]
            self.__is_signed = ext_data[6] != 0
        elif self.__block_type == COLUMN_TYPE_SPARSE:
            self.__default_value = ext_data[4]

            # Sparse float fields definitely have float-valued defaults.
            # This likely generalizes to arbitrary field types, but for now,
            # we conservatively use this hack.
            if not self.__parser.options.raw and self.__format.data_type == 'f':
                packed = Struct('<I').pack(self.__default_value)
                self.__default_value = Struct('<f').unpack(packed)[0]

            if ext_data[5] > 0 or ext_data[6] > 0:
                logging.warn('Sparse data field has unexpted non-zero values (%d, %d)',
                    ext_data[5], ext_data[6])

        elif self.__block_type == COLUMN_TYPE_INDEXED or self.__block_type == COLUMN_TYPE_ARRAY:
            self.__packed_bit_offset = ext_data[4]

            if self.__field_size != ext_data[5]:
                logging.warn('Column %s field sizes differ (%d vs %d)',
                    self.__index, self.__field_size, ext_data[5])

            if self.__block_type == COLUMN_TYPE_INDEXED and ext_data[6] != 0:
                logging.warn('Column %s for %s has non-zero elements (%d)',
                    self.__index, self.__parser.file_name(), ext_data[6])

            self.__field_size = ext_data[5]
            self.__elements   = ext_data[6]
        elif self.__block_type == COLUMN_TYPE_BIT_S:
            self.__packed_bit_offset = ext_data[4]
            self.__value_size = ext_data[5]
            self.__is_signed = True

    def field_base_type(self):
        return self.__size_type

    def field_ext_type(self):
        return self.__block_type

    def field_block_size(self):
        return self.__block_size

    def field_offset(self):
        return self.__offset

    def field_bit_offset(self):
        if self.__bit_offset > 0:
            return self.__bit_offset
        else:
            return self.__offset * 8

    def field_packed_bit_offset(self):
        return self.__packed_bit_offset

    def field_byte_offset(self):
        bit_offset = self.field_bit_offset()

        return bit_offset // 8

    def field_bit_size(self):
        if self.__size_type in [0, 8, 16, 24, -32]:
            return (32 - self.__size_type)
        else:
            return self.__field_size

    def field_whole_bytes(self):
        if self.__size_type in [0, 8, 16, 24, -32]:
            return (32 - self.__size_type) // 8
        else:
            if self.__field_size < 9:
                return 1
            elif self.__field_size < 17:
                return 2
            elif self.__field_size < 33:
                return 4
            elif self.__field_size < 65:
                return 8

            return 0

    def value_bit_size(self):
        if self.__size_type in [0, 8, 16, 24, -32]:
            return (32 - self.__size_type)
        elif self.__block_type in [COLUMN_TYPE_BIT, COLUMN_TYPE_BIT_S]:
            return self.__value_size
        else:
            format_bit_size = self.format_bit_size()
            if format_bit_size > 0:
                return format_bit_size
            # Just default to 32 bits for the other extended column types
            # if we can find nothing else
            else:
                return 32

    def default(self):
        return self.__default_value

    def elements(self):
        if self.__size_type in [0, 8, 16, 24, -32]:
            return self.__field_size // self.field_bit_size()
        else:
            return self.__elements or 1

    def struct_type(self):
        if self.__size_type != WDC1_SPECIAL_COLUMN:
            if not self.__format:
                return get_struct_type(False, self.is_signed(), self.value_bit_size())
            else:
                return self.__format and self.__format.data_type.replace('S', 'I')
        else:
            if not self.__format:
                return get_struct_type(False, True, 32)
            else:
                return self.__format.data_type.replace('S', 'I')

    def is_float(self):
        return self.__format and self.__format.data_type == 'f' or False

    def is_signed(self):
        if self.__block_type == COLUMN_TYPE_BIT:
            return self.__is_signed
        elif self.__block_type == COLUMN_TYPE_BIT_S:
            return True
        else:
            if self.__format:
                return self.__format.data_type in ['b', 'h', 'i', 'q']
            else:
                return True

    def is_string(self):
        if self.__format:
            return self.__format.data_type == 'S'
        else:
            return False

    def short_type(self):
        if self.is_float():
            base_type = 'float'
        elif self.is_string():
            base_type = 'string'
        elif not self.is_signed():
            base_type = 'uint'
        else:
            base_type = 'int'

        bit_size = self.value_bit_size()

        return '{}{}'.format(base_type, bit_size)

    def __str__(self):
        fields = []
        fields.append('byte_offset={:<3d}'.format(self.__offset))
        if self.__block_type == COLUMN_TYPE_BIT:
            fields.append('type={:<6s}'.format('bits'))
        elif self.__block_type == COLUMN_TYPE_SPARSE:
            fields.append('type={:<6s}'.format('sparse'))
        elif self.__block_type == COLUMN_TYPE_INDEXED:
            fields.append('type={:<6s}'.format('index'))
        elif self.__block_type == COLUMN_TYPE_ARRAY:
            fields.append('type={:<6s}'.format('array'))
        elif self.__block_type == COLUMN_TYPE_BIT_S:
            fields.append('type={:<6s}'.format('sbits'))
        else:
            fields.append('type={:<6s}'.format('bytes'))
        fields.append('{:<10s}'.format('({})'.format(self.short_type())))

        fields.append('bit_offset={:<3d}'.format(self.field_bit_offset()))

        if self.__block_type in [COLUMN_TYPE_BIT, COLUMN_TYPE_INDEXED, COLUMN_TYPE_ARRAY, COLUMN_TYPE_BIT_S]:
            fields.append('packed_bit_offset={:<3d}'.format(self.__packed_bit_offset))

        if self.__block_type in [COLUMN_TYPE_BIT]:
            fields.append('signed={!s:<5}'.format(self.__is_signed))

        if self.__block_type in [COLUMN_TYPE_BIT_S]:
            fields.append('signed=True')

        if self.elements() > 1:
            fields.append('elements={:<2d}'.format(self.elements()))

        if self.__block_type in [COLUMN_TYPE_SPARSE]:
            fields.append('default={}'.format(self.__default_value))

        if self.__block_type in [COLUMN_TYPE_SPARSE, COLUMN_TYPE_INDEXED, COLUMN_TYPE_ARRAY]:
            fields.append('block_size={:<7d}'.format(self.field_block_size()))

        return 'Field{:<2d}: {}'.format(self.__index, ' '.join(fields))

class WDC1Parser(DBCParserBase):
    __WDC1_HEADER_FIELDS = [
        HeaderFieldInfo('magic',                  '4s'),
        HeaderFieldInfo('records',                'I' ),
        HeaderFieldInfo('fields',                 'I' ),
        HeaderFieldInfo('record_size',            'I' ),
        HeaderFieldInfo('string_block_size',      'I' ),
        HeaderFieldInfo('table_hash',             'I' ),
        HeaderFieldInfo('layout_hash',            'I' ),
        HeaderFieldInfo('first_id',               'I' ),
        HeaderFieldInfo('last_id',                'I' ),
        HeaderFieldInfo('locale',                 'I' ),
        HeaderFieldInfo('clone_block_size',       'I' ),
        HeaderFieldInfo('flags',                  'H' ),
        HeaderFieldInfo('id_index',               'H' ),
        HeaderFieldInfo('total_fields',           'I' ),
        HeaderFieldInfo('packed_data_offset',     'I' ),
        HeaderFieldInfo('wdc_unk1',               'I' ),
        HeaderFieldInfo('offset_map_offset',      'I' ),
        HeaderFieldInfo('id_block_size',          'I' ),
        HeaderFieldInfo('column_info_block_size', 'I' ),
        HeaderFieldInfo('sparse_block_size',      'I' ),
        HeaderFieldInfo('column_data_block_size', 'I' ),
        HeaderFieldInfo('key_block_size',         'I' )
    ]

    def is_magic(self): return self.magic == b'WDC1'

    def __init__(self, options, fname):
        super().__init__(options, fname)

        # Set heder format
        self.header_format = self.__WDC1_HEADER_FIELDS
        self.parse_offset = 0

        # Lazy-computed key format for the foreign key values
        self.__key_format = None
        self._key_high = -1

        # Id format
        self.__id_format = None

        # Record parser for the file
        self.record_parser = None

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
        # Column information for raw data
        self.data_column_info = []

        # Sparse block data, preprocess if present so we can have fast lookups during parsing
        self.sparse_blocks = []

        # Foreign key block data, just a bunch of ids (on a per record basis)
        self.key_block = []

    def get_string(self, offset, dbc_id, field_index):
        if offset == 0:
            return None

        end_offset = self.data.find(b'\x00', self.string_block_offset + offset)

        if end_offset == -1:
            return None

        return self.data[self.string_block_offset + offset:end_offset].decode('utf-8')

    def get_record_info(self, record_id):
        if record_id > len(self.record_table):
            return dbc.EMPTY_RECORD

        record_info = self.record_table[record_id]
        if record_info:
            return record_info
        else:
            return dbc.EMPTY_RECORD

    def get_dbc_info(self, dbc_id):
        if dbc_id > len(self.id_table):
            return dbc.EMPTY_RECORD

        record_info = self.id_table[dbc_id]
        if record_info:
            return record_info
        else:
            return dbc.EMPTY_RECORD

    def get_record(self, dbc_id, offset, size):
        return self.record_parser(dbc_id, self.data, offset, size)

    def column(self, idx):
        return self.column_info[idx]

    def data_column(self, idx ):
        return self.data_column_info[idx]

    def id_format(self):
        if self.__id_format:
            return self.__id_format

        if self.last_id == 0:
            return '%u'

        # Adjust the size of the id formatter so we get consistent length
        # id field where we use it, and don't have to guess on the length
        # in the format file.
        n_digits = int(math.log10(self.last_id) + 1)
        self.__id_format = '%%%uu' % n_digits
        return self.__id_format

    # Bytes required for the foreign key, needed for hotfix cache to parse it out
    def key_bytes(self):
        length = self._key_high.bit_length()
        if length > 16:
            return 4
        elif length > 8:
            return 2
        else:
            return 1

    # Note, wdb_name is ignored, as WDC1 parsers are always bound to a single
    # wdb file (e.g., Spell.db2)
    def key_format(self):
        if not self.has_key_block():
            return '%u'

        if self.__key_format:
            return self.__key_format

        n_digits = int(math.log10(self._key_high) + 1)
        self.__key_format = '%%%uu' % n_digits
        return self.__key_format

    def sparse_data_offset(self, column, id_):
        if column.index() >= len(self.sparse_blocks):
            return -1

        return self.sparse_blocks[column.index()].get(id_, -1)

    def create_raw_parser(self, hotfix_parser = False):
        return WDC1RecordParser(self, hotfix = hotfix_parser)

    def create_formatted_parser(self, hotfix_parser = False):
        formats = self.fmt.objs(self.class_name(), True)

        if len(formats) != self.fields:
            logging.error('%s field count mismatch, format: %u, file: %u',
                self.full_name(), len(formats), self.fields)
            return None

        parser = WDC1RecordParser(self, hotfix = hotfix_parser)

        logging.debug('Unpacking plan for %s%s: %s',
                self.full_name(),
                hotfix_parser and ' (hotfix)' or '',
                parser)

        return parser

    # Compute offset into the file, based on what blocks we have
    def compute_block_offsets(self):
        # Offset immediately after the header
        self.data_offset = self.parse_offset

        if self.has_offset_map():
            # String block follows immediately after the base data block
            self.string_block_offset = 0
            running_offset = self.offset_map_offset + (self.last_id - self.first_id + 1) * 6
        else:
            # String block follows immediately after the base data block. All
            # normal db2 files have a string block, if there's no strings in
            # the file it'll just have two null bytes.
            self.string_block_offset = self.data_offset + self.record_size * self.records
            running_offset = self.string_block_offset + self.string_block_size

        # Next, ID block follows immmediately after string block
        self.id_block_offset = running_offset
        running_offset = self.id_block_offset + self.id_block_size

        # Then, a possible clone block offset
        self.clone_block_offset = running_offset
        running_offset = self.clone_block_offset + self.clone_block_size

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

        logging.debug('Computed offsets, data=%d, strings=%d, ids=%d, ' +
                      'clones=%d, column_infos=%d, column_datas=%d, sparse_datas=%d, ' +
                      'keys=%d',
            self.data_offset, self.string_block_offset, self.id_block_offset, self.clone_block_offset,
            self.column_info_block_offset, self.column_data_block_offset, self.sparse_block_offset,
            self.key_block_offset)

        return self.key_block_offset + self.key_block_size

    # Parse column information (right after static header)
    def parse_column_info(self):
        if not self.options.raw:
            try:
                formats = self.fmt.objs(self.class_name(), True)
            except Exception:
                formats = None
        else:
            formats = None

        for idx in range(0, self.fields):
            size_type, offset = _COLUMN_INFO.unpack_from(self.data, self.parse_offset)

            self.column_info.append(WDC1Column(self,
                (formats and idx < len(formats)) and formats[idx] or None,
                idx, size_type, offset))

            self.parse_offset += _COLUMN_INFO.size

        logging.debug('Parsed basic column info')

        return True

    def parse_id_data(self):
        if self.id_index >= len(self.column_info):
            logging.error('Id index column %d is too large', self.id_index)
            return False

        column = self.column_info[self.id_index]

        logging.debug('Index for record in %s', column)

        self.record_table = []
        self.id_table = [ None ] * (self.last_id + 1)

        # Start bit in the bytes that include the id column
        start_offset = column.field_bit_offset() % 8
        end_offset = start_offset + column.value_bit_size()
        # Length of the id field in whole bytes
        n_bytes = math.ceil((end_offset - start_offset) / 8)

        for record_id in range(0, self.records):
            record_start = self.data_offset + record_id * self.record_size
            bitfield_start = record_start + column.field_byte_offset()

            # Whole bytes, just direct convert the bytes
            if column.field_bit_size() % 8 == 0:
                field_data = self.data[bitfield_start:bitfield_start + n_bytes]
            # Grab specific bits
            else:
                # Read enough bytes to the bitarray from the segment
                barr = bitarray(endian = 'little')
                barr.frombytes(self.data[bitfield_start:bitfield_start + n_bytes])
                # Grab bits, convert to unsigned int
                field_data = barr[start_offset:end_offset].tobytes()

            dbc_id = int.from_bytes(field_data, byteorder = 'little')

            # Parse key block at this point too, so we can create full record information
            if self.has_key_block():
                key_id = self.key_block[record_id]
            else:
                key_id = 0

            record_info = DBCRecordInfo(dbc_id, record_id, record_start, self.record_size, key_id, -1)
            self.record_table.append(record_info)
            self.id_table[dbc_id] = record_info

        logging.debug('Parsed id information')

        return True

    def parse_offset_map(self):
        return True

    def parse_id_block(self):
        self.record_table = []
        self.id_table = [ None ] * (self.last_id + 1)

        unpacker = Struct('%dI' % self.records)
        record_id = 0
        for dbc_id in unpacker.unpack_from(self.data, self.id_block_offset):
            data_offset = 0
            size = self.record_size

            data_offset = self.data_offset + record_id * self.record_size

            # If the db2 file has a key block, we need to grab the key at this
            # point from the block, to record it into the data we have. Storing
            # the information now associates with the correct dbc id, since the
            # key block is record index based, not dbc id based.
            if self.has_key_block():
                key_id = self.key_block[record_id]
            else:
                key_id = 0

            record_info = DBCRecordInfo(dbc_id, record_id, data_offset, size, key_id, -1)
            self.record_table.append(record_info)
            self.id_table[dbc_id] = record_info

            record_id += 1

        logging.debug('Parsed id block')

        return True

    def parse_clone_block(self):
        # Cloned internal record id starts directly after the main data(?)
        record_id = len(self.record_table) + 1

        # Process clones
        for clone_id in range(0, self.clone_block_size // _CLONE_INFO.size):
            clone_offset = self.clone_block_offset + clone_id * _CLONE_INFO.size
            target_id, source_id = _CLONE_INFO.unpack_from(self.data, clone_offset)

            source = self.id_table[source_id]
            if not source:
                logging.warn('Unable to find source data with dbc_id %d for cloning for id %d', source_id, target_id)
                record_id += 1
                continue

            record_info = DBCRecordInfo(target_id, source.record_id, source.record_offset, source.record_size, source.parent_id, -1)
            self.record_table.append(record_info)

            if target_id > self.last_id:
                logging.error('Clone target id %d higher than last id %d', target_id, self.last_id)
                return False

            self.id_table[target_id] = record_info

            record_id += 1

        logging.debug('Parsed clone block')

        return True

    # Parse extended column information to supplement the basic one, if it exists
    def parse_extended_column_info_block(self):
        if self.column_info_block_size != self.fields * _WDC1_COLUMN_INFO.size:
            logging.error('Extended column info size mismatch, expected {}, got {}',
                    self.fields * _WDC1_COLUMN_INFO.size, self.column_info_block_size)
            return False

        offset = self.column_info_block_offset
        for idx in range(0, self.fields):
            data = _WDC1_COLUMN_INFO.unpack_from(self.data, offset)

            self.column_info[idx].set_ext_data(data)

            offset += _WDC1_COLUMN_INFO.size

        logging.debug('Parsed extended column info block')

        for column in self.column_info:
            self.data_column_info += [ column ] * column.elements()

        return True

    def parse_sparse_block(self):
        offset = self.sparse_block_offset

        for column_idx in range(0, self.fields):
            block = {}
            column = self.column(column_idx)

            if column.field_ext_type() != COLUMN_TYPE_SPARSE:
                self.sparse_blocks.append(block)
                continue

            # TODO: Are sparse blocks always <dbc_id, value> tuples with 4 bytes for a value?
            # TODO: Do we want to preparse the sparse block? Would save an
            # unpack call at the cost of increased memory
            if column.field_block_size() % 8 != 0:
                logging.error('%s: Unknown sparse block type for column %s',
                    self.class_name(), column)
                return False

            logging.debug('%s unpacking sparse block for %s at %d, %d entries',
                self.full_name(), column, offset, column.field_block_size() // 8)

            unpack_full_str = '<' + ((column.field_block_size() // 8) * 'I4x')
            unpacker = Struct(unpack_full_str)
            value_index = 0
            for dbc_id in unpacker.unpack_from(self.data, offset):
                # Store <dbc_id, offset> tuples into the sparse block
                block[dbc_id] = offset + value_index * 8 + 4

                value_index += 1

            offset += column.field_block_size()

            self.sparse_blocks.append(block)

        logging.debug('Parsed sparse blocks')

        return True

    def parse_key_block(self):
        if (self.key_block_size - _WDC1_KEY_HEADER.size) % 8 != 0:
            logging.error('%s invalid key block size, expected divisible by 8, got %u (%u)',
                self.full_name(), self.key_block_size, self.key_block_size % 8)
            return False

        offset = self.key_block_offset
        records, min_id, max_id = _WDC1_KEY_HEADER.unpack_from(self.data, offset)

        offset += _WDC1_KEY_HEADER.size

        # Just make a sparse table for this
        self.key_block = collections.defaultdict(lambda : 0)

        # Value, record id pairs
        unpacker = Struct('II')
        for index in range(0, records):
            value, record_id = unpacker.unpack_from(self.data, offset + index * unpacker.size)
            self.key_block[record_id] = value

            if value > self._key_high:
                self._key_high = value

        logging.debug('%s parsed %u keys', self.full_name(), self.records)

        return True

    def has_offset_map(self):
        return (self.flags & X_OFFSET_MAP) == X_OFFSET_MAP

    def has_string_block(self):
        return self.string_block_size > 2

    def has_id_block(self):
        return (self.flags & X_ID_BLOCK) == X_ID_BLOCK

    def has_clone_block(self):
        return self.clone_block_size > 0

    def has_extended_column_info_block(self):
        return self.column_info_block_size > 0

    def has_sparse_block(self):
        return self.sparse_block_size > 0

    def has_key_block(self):
        return self.key_block_size > 0

    def n_records(self):
        return self.records > 0 and len(self.record_table) or 0

    def build_parser(self):
        if self.options.raw:
            self.record_parser = self.create_raw_parser()
        else:
            self.record_parser = self.create_formatted_parser()

        return self.record_parser != None

    def parse_blocks(self):
        if self.has_extended_column_info_block() and not self.parse_extended_column_info_block():
            return False

        if self.has_sparse_block() and not self.parse_sparse_block():
            return False

        if not self.parse_key_block():
            return False

        if not self.parse_id_block():
            return False

        if not self.parse_clone_block():
            return False

        # If there is no ID block, generate a proxy ID block from actual record data
        if not self.parse_id_data():
            return False

        if not self.parse_offset_map():
            return False

        return True

    def open(self):
        if not super().open():
            return False

        if self.empty_file():
            return True

        if not self.build_parser():
            return False

        # Setup some data class-wide variables
        if self.options.raw:
            data_class = dbc.data.RawDBCRecord
        else:
            data_class = getattr(dbc.data, self.class_name(), None)

        if data_class:
            # Enable specific features for the data classes upon opening the WDC1
            # file. We can't set a class-wide parser here, as some of the data must
            # come from other than the WDC1 file (i.e., the Hotfix file)
            data_class.id_format(self.id_format())
            if self.has_id_block():
                data_class.has_id_block(True)
            else:
                data_class.id_index(self.id_index)

            if self.has_key_block():
                data_class.key_format(self.key_format())
                data_class.has_key_block(True)

        return True

    def fields_str(self):
        fields = super().fields_str()

        fields.append('magic={}'.format(self.magic.decode('utf-8')))
        fields.append('byte_size={}'.format(len(self.data)))

        fields.append('records={} ({})'.format(self.records, self.n_records()))
        fields.append('fields={} ({})'.format(self.fields, self.total_fields))
        fields.append('record_size={}'.format(self.record_size))
        fields.append('string_block_size={}'.format(self.string_block_size))
        fields.append('table_hash={:#010x}'.format(self.table_hash))
        fields.append('layout_hash={:#010x}'.format(self.layout_hash))
        fields.append('first_id={}'.format(self.first_id))
        fields.append('last_id={}'.format(self.last_id))
        fields.append('locale={:#010x}'.format(self.locale))

        fields.append('clone_block_size={}'.format(self.clone_block_size))
        fields.append('flags={:#06x}'.format(self.flags))
        fields.append('id_index={}'.format(self.id_index))
        fields.append('total_fields={}'.format(self.total_fields))

        fields.append('ofs_record_packed_data={}'.format(self.packed_data_offset))
        fields.append('wdc1_unk1={}'.format(self.wdc1_unk3))

        if not self.has_offset_map():       fields.append('ofs_data={}'.format(self.data_offset))
        fields.append('id_block_size={}'.format(self.id_block_size))
        fields.append('column_info_block_size={}'.format(self.column_info_block_size))
        fields.append('sparse_block_size={}'.format(self.sparse_block_size))
        fields.append('column_data_block_size={}'.format(self.column_data_block_size))
        fields.append('key_block_size={}'.format(self.key_block_size))

        # Offsets to blocks
        if self.string_block_size > 0:      fields.append('ofs_string_block={}'.format(self.string_block_offset))
        if self.has_offset_map():           fields.append('ofs_offset_map={}'.format(self.offset_map_offset))
        if self.has_id_block():             fields.append('ofs_id_block={}'.format(self.id_block_offset))
        if self.clone_block_size > 0:       fields.append('ofs_clone_block={}'.format(self.clone_block_offset))
        if self.column_info_block_size > 0: fields.append('ofs_column_info_block={}'.format(self.column_info_block_offset))
        if self.column_data_block_size > 0: fields.append('ofs_column_data_block={}'.format(self.column_data_block_offset))
        if self.sparse_block_size > 0:      fields.append('ofs_sparse_block={}'.format(self.sparse_block_offset))
        if self.has_key_block():            fields.append('ofs_key_block={}'.format(self.key_block_offset))

        return fields

    def __str__(self):
        s = super().__str__()

        if len(self.column_info):
            s += '\n'
            s += '\n'.join([ str(c) for c in self.column_info ])

        return s;
