import logging, struct, collections, math, sys

from bitarray import bitarray

from dbc import HeaderFieldInfo, DBCRecordInfo
from dbc import wdc1
from dbc.wdc1 import WDC1Parser

class WDC2Section:
    def __init__(self, parser, id):
        self.parser = parser
        self.id = id

        self.record_dbc_id_table = []
        self.clone_block_data = []
        self.key_block = collections.defaultdict(lambda : 0)
        self.offset_map = []

    def __str__(self):
        return ('Section{}: key_id={:#018x}, ptr_records={}, total_records={}, sz_string_block={}, ' +
               'sz_clone_block={}, ptr_offset_map={}, sz_id_block={}, sz_key_block={}').format(
                    self.id,
                    self.tact_key_id, self.ptr_records, self.total_records, self.string_block_size,
                    self.clone_block_size, self.ptr_offset_map, self.id_block_size,
                    self.key_block_size
                )

    def set_data(self, data):
        self.tact_key_id = data[0]
        self.ptr_records = data[1]
        self.total_records = data[2]
        self.string_block_size = data[3]
        self.clone_block_size = data[4]
        self.ptr_offset_map = data[5]
        self.id_block_size = data[6]
        self.key_block_size = data[7]

    def compute_block_offsets(self):
        if self.ptr_offset_map > 0:
            # String block follows immediately after the base data block
            self.ptr_string_block = 0
            running_offset = self.ptr_offset_map + (self.parser.last_id - self.parser.first_id + 1) * 6
        else:
            # Note, string block offset in WDC2 is not really interesting as
            # string offsets in records are in terms of the data offset
            self.ptr_string_block = self.ptr_records + self.parser.record_size * self.total_records
            running_offset = self.ptr_string_block + self.string_block_size

        # Next, ID block follows immmediately after the new record + string block
        if self.id_block_size > 0:
            self.ptr_id_block = running_offset
            running_offset = self.ptr_id_block + self.id_block_size
        else:
            self.ptr_id_block = 0

        if self.clone_block_size > 0:
            # Then, a possible clone block
            self.ptr_clone_block = running_offset
            running_offset = self.ptr_clone_block + self.clone_block_size
        else:
            self.ptr_clone_block = 0

        if self.key_block_size > 0:
            self.ptr_key_block = running_offset
            running_offset = self.ptr_key_block + self.key_block_size
        else:
            self.ptr_key_block = 0

        logging.debug('Computed offsets for section=%d, records=%d, offset_map=%d, strings=%d, ids=%d, ' +
                      'clones=%d, keys=%d',
            self.id, self.ptr_records, self.ptr_offset_map, self.ptr_string_block, self.ptr_id_block,
            self.ptr_clone_block, self.ptr_key_block)

        return running_offset

    def parse_offset_map(self):
        if self.ptr_offset_map == 0:
            return True

        for record_id in range(0, self.parser.last_id - self.parser.first_id + 1):
            ofs = self.ptr_offset_map + record_id * wdc1._ITEMRECORD.size

            data_offset, size = wdc1._ITEMRECORD.unpack_from(self.parser.data, ofs)
            if size == 0:
                continue

            dbc_id = record_id + self.parser.first_id

            self.offset_map.append((dbc_id, data_offset, size))

        return True

    def parse_id_block(self):
        if self.id_block_size == 0:
            return True

        unpacker = struct.Struct('%dI' % (self.id_block_size // 4))
        self.record_dbc_id_table = unpacker.unpack_from(self.parser.data, self.ptr_id_block)

        logging.debug('Parsed id block for section %d (%d records)', self.id,
                self.id_block_size // 4)

        return True

    def parse_id_data(self):
        if self.id_block_size > 0:
            return True

        column = self.parser.column_info[self.parser.id_index]

        logging.debug('Index for record in %s', column)

        # Start bit in the bytes that include the id column
        start_offset = column.field_bit_offset() % 8
        end_offset = start_offset + column.value_bit_size()
        # Length of the id field in whole bytes
        n_bytes = math.ceil((end_offset - start_offset) / 8)

        for record_id in range(0, self.total_records):
            record_start = self.ptr_records + record_id * self.parser.record_size
            bitfield_start = record_start + column.field_byte_offset()

            # Whole bytes, just direct convert the bytes
            if column.field_bit_size() % 8 == 0:
                field_data = self.parser.data[bitfield_start:bitfield_start + n_bytes]
            # Grab specific bits
            else:
                # Read enough bytes to the bitarray from the segment
                barr = bitarray(endian = 'little')
                barr.frombytes(self.parser.data[bitfield_start:bitfield_start + n_bytes])
                # Grab bits, convert to unsigned int
                field_data = barr[start_offset:end_offset].tobytes()

            self.record_dbc_id_table.append(int.from_bytes(field_data, byteorder = 'little'))

        logging.debug('Parsed id information for section %d', self.id)

        return True

    def parse_clone_block(self):
        if self.clone_block_size == 0:
            return True

        # Process clones
        for clone_id in range(0, self.clone_block_size // wdc1._CLONE_INFO.size):
            clone_offset = self.ptr_clone_block + clone_id * wdc1._CLONE_INFO.size
            self.clone_block_data.append(wdc1._CLONE_INFO.unpack_from(self.parser.data, clone_offset))

        logging.debug('Parsed clone block for section %d (%d clones)', self.id,
                self.clone_block_size // wdc1._CLONE_INFO.size)

        return True

    def parse_key_block(self):
        if self.key_block_size == 0:
            return True

        offset = self.ptr_key_block
        records, min_id, max_id = wdc1._WDC1_KEY_HEADER.unpack_from(self.parser.data, offset)

        offset += wdc1._WDC1_KEY_HEADER.size

        # Value, record id pairs
        unpacker = struct.Struct('II')
        for index in range(0, records):
            parent_id, record_idx = unpacker.unpack_from(self.parser.data, offset)
            self.key_block[record_idx] = parent_id
            offset += unpacker.size

        logging.debug('%s parsed %u keys', self.parser.full_name(), records)

        return True

    def construct_record_info(self):
        records = []
        record_id_map = {}

        if len(self.offset_map) > 0:
            for index in range(0, len(self.offset_map)):
                dbc_id, offset, size = self.offset_map[index]

                parent_id = self.key_block[index]

                record = DBCRecordInfo(dbc_id, offset, size, parent_id, self.id)
                records.append(record)
                record_id_map[record.dbc_id] = record

        elif len(self.record_dbc_id_table) > 0:
            for index in range(0, len(self.record_dbc_id_table)):
                dbc_id = self.record_dbc_id_table[index]
                offset = self.ptr_records + index * self.parser.record_size
                parent_id = self.key_block[index]

                record = DBCRecordInfo(dbc_id, offset, self.parser.record_size, parent_id, self.id)
                records.append(record)
                record_id_map[record.dbc_id] = record

        for target_dbc_id, source_dbc_id in self.clone_block_data:
            if source_dbc_id not in record_id_map:
                logging.warn('Unable to find clone source id %d in section %d', source_dbc_id, self.id)
                continue

            source = record_id_map[source_dbc_id]
            record = DBCRecordInfo(target_dbc_id, source.record_offset,
                    source.record_size, source.parent_id, self.id)
            records.append(record)

        records.sort(key = lambda v: v.dbc_id)

        return records

class WDC2Parser(WDC1Parser):
    __WDC2_HEADER_FIELDS = [
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
        HeaderFieldInfo('flags',                  'H' ),
        HeaderFieldInfo('id_index',               'H' ),
        HeaderFieldInfo('total_fields',           'I' ),
        HeaderFieldInfo('packed_data_offset',     'I' ),

        HeaderFieldInfo('wdc2_unk1',              'I' ),
        HeaderFieldInfo('column_info_block_size', 'I' ),
        HeaderFieldInfo('sparse_block_size',      'I' ),
        HeaderFieldInfo('column_data_block_size', 'I' ),
        HeaderFieldInfo('sections',               'I' )
    ]

    SECTION_FIELDS = [
        HeaderFieldInfo('tact_key_id',            'Q' ),
        HeaderFieldInfo('offset_records',         'I' ),
        HeaderFieldInfo('total_records',          'I' ),
        HeaderFieldInfo('string_block_size',      'I' ),
        HeaderFieldInfo('clone_block_size',       'I' ),
        HeaderFieldInfo('offset_map_offset',      'I' ),
        HeaderFieldInfo('id_block_size',          'I' ),
        HeaderFieldInfo('key_block_size',         'I' )
    ]

    SECTION_OBJECT = WDC2Section

    def is_magic(self): return self.magic == b'WDC2'

    def __init__(self, options, fname):
        super().__init__(options, fname)

        # Set heder format
        self.header_format = self.__WDC2_HEADER_FIELDS
        self.section_data = []

    def has_key_block(self):
        # Presume if the first section has a key block, the rest will too
        if self.sections > 0:
            return self.section_data[0].key_block_size > 0
        else:
            return False

    def parse_extended_header(self):
        parser_str = '<'
        for info in self.SECTION_FIELDS:
            parser_str += info.format

        parser = struct.Struct(parser_str)

        for section_id in range(0, self.sections):
            data = parser.unpack_from(self.data, self.parse_offset)
            if len(data) != len(self.SECTION_FIELDS):
                logging.error('%s: Header field count mismatch', self.class_name())
                return False

            new_section = self.SECTION_OBJECT(self, section_id)
            new_section.set_data(data)

            self.section_data.append(new_section)
            self.parse_offset += parser.size

        return True

    def parse_offset_map(self):
        for section in filter(lambda s: s.tact_key_id == 0, self.section_data):
            if not section.parse_offset_map():
                return False

        return True

    def parse_id_block(self):
        for section in filter(lambda s: s.tact_key_id == 0, self.section_data):
            if not section.parse_id_block():
                return False

        return True

    def parse_id_data(self):
        for section in filter(lambda s: s.tact_key_id == 0, self.section_data):
            if not section.parse_id_data():
                return False

        return True

    def parse_clone_block(self):
        for section in filter(lambda s: s.tact_key_id == 0, self.section_data):
            if not section.parse_clone_block():
                return False

        return True

    def parse_key_block(self):
        for section in filter(lambda s: s.tact_key_id == 0, self.section_data):
            if not section.parse_key_block():
                return False

            if section.key_block:
                key_high = max(section.key_block, key = lambda k: section.key_block[k])
                if section.key_block[key_high] > self._key_high:
                    self._key_high = section.key_block[key_high]

        return True

    def parse_blocks(self):
        if not super().parse_blocks():
            return False

        # Construct DBCRecord information since all sections are now parsed
        if not self.build_record_info():
            return False

        return True

    def build_record_info(self):
        # Index by record index
        self.record_table = []
        # Index by record id (aka dbc id)
        self.id_table = collections.defaultdict(lambda : None)

        for section in filter(lambda s: s.tact_key_id == 0, self.section_data):
            records = section.construct_record_info()
            self.record_table += records
            for record in records:
                if record.dbc_id in self.id_table:
                    logging.warn('Duplicate record with dbc id %d found', record.dbc_id)

                self.id_table[record.dbc_id] = record

        return True

    def get_string_offset(self, raw_offset, dbc_id, field_index):
        column = self.data_column(field_index)
        record = self.id_table[dbc_id]
        section = self.section_data[record.section_id]
        # String arrays need to figure out the element indices so that string
        # offsets begin from the start of the corrent array element (instead
        # from the beginning of the array)
        element_index = field_index - column.index()

        # Compute relative (to the section's string block offset) string offset inside the section
        string_offset = raw_offset + record.record_offset + column.field_byte_offset()
        string_offset += element_index * column.field_whole_bytes()
        string_offset -= (self.records - section.total_records) * self.record_size

        return string_offset

    # WDC2 string field offsets are not relative to the string block, but
    # rather relative to the position of the (data, field index) tuple of the
    # record in the file
    def get_string(self, raw_offset, dbc_id, field_index):
        if raw_offset == 0:
            return None

        record = self.id_table[dbc_id]
        section = self.section_data[record.section_id]
        if section.ptr_offset_map == 0:
            start_offset = self.get_string_offset(raw_offset, dbc_id, field_index)
        else:
            start_offset = raw_offset

        end_offset = self.data.find(b'\x00', start_offset)

        if end_offset == -1:
            return None

        return self.data[start_offset:end_offset].decode('utf-8')

    # Compute offset into the file, based on what blocks we have
    def compute_block_offsets(self):
        # Extended column information follows immediately after the static header
        self.column_info_block_offset = self.parse_offset
        running_offset = self.column_info_block_offset + self.column_info_block_size

        # Followed by the column-specific data block(s)
        self.column_data_block_offset = running_offset
        running_offset = self.column_data_block_offset + self.column_data_block_size

        # Then, the sparse block(s)
        self.sparse_block_offset = running_offset
        running_offset = self.sparse_block_offset + self.sparse_block_size

        logging.debug('Computed offsets, column_infos=%d, column_datas=%d, sparse_datas=%d',
            self.column_info_block_offset, self.column_data_block_offset, self.sparse_block_offset)

        last_offset = 0
        for section in self.section_data:
            last_offset = section.compute_block_offsets()

        return last_offset

    def fields_str(self):
        fields = []

        fields.append('magic={}'.format(self.magic.decode('utf-8')))
        fields.append('byte_size={}'.format(len(self.data)))

        fields.append('records={} ({})'.format(self.records, self.n_records()))
        fields.append('fields={} ({})'.format(self.fields, self.total_fields))
        fields.append('sz_record={}'.format(self.record_size))
        fields.append('sz_string_block={}'.format(self.string_block_size))
        fields.append('table_hash={:#010x}'.format(self.table_hash))
        fields.append('layout_hash={:#010x}'.format(self.layout_hash))
        fields.append('first_id={}'.format(self.first_id))
        fields.append('last_id={}'.format(self.last_id))
        fields.append('locale={:#010x}'.format(self.locale))

        fields.append('flags={:#06x}'.format(self.flags))
        fields.append('id_index={}'.format(self.id_index))
        fields.append('total_fields={}'.format(self.total_fields))
        fields.append('ptr_record_packed_data={}'.format(self.packed_data_offset))

        fields.append('wdc2_unk1={}'.format(self.wdc2_unk1))
        fields.append('sz_column_info_block={}'.format(self.column_info_block_size))
        fields.append('sz_sparse_block={}'.format(self.sparse_block_size))
        fields.append('sz_column_data_block={}'.format(self.column_data_block_size))
        fields.append('sections={}'.format(self.sections))

        return fields

    def __str__(self):
        s = super().__str__()

        if self.sections > 0:
            s += '\n'
            s += '\n'.join([str(v) for v in self.section_data])

        return s;
