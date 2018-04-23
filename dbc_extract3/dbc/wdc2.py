import logging, struct

from dbc import HeaderFieldInfo

from dbc.wdc1 import WDC1Parser

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
        HeaderFieldInfo('extended_fields',        'I' )
    ]

    EXTENDED_FIELDS = [
        HeaderFieldInfo('wdc2_unk6',              'I' ),
        HeaderFieldInfo('wdc2_unk7',              'I' ),
        HeaderFieldInfo('offset_records',         'I' ),
        HeaderFieldInfo('total_records',          'I' ),
        HeaderFieldInfo('unk_string_block_size',  'I' ),
        HeaderFieldInfo('clone_block_size',       'I' ),
        HeaderFieldInfo('offset_map_offset',      'I' ),
        HeaderFieldInfo('id_block_size',          'I' ),
        HeaderFieldInfo('key_block_size',         'I' )
    ]

    def is_magic(self): return self.magic == b'WDC2'

    def __init__(self, options, fname):
        super().__init__(options, fname)

        # Set heder format
        self.header_format = self.__WDC2_HEADER_FIELDS

    def parse_header(self):
        if not super().parse_header():
            return False

        if self.extended_fields == 0:
            return True

        # WDC2 has some conditional header information that depends on the "extended_fields" value
        parser_str = '<'
        for info in self.EXTENDED_FIELDS:
            parser_str += info.format

        parser = struct.Struct(parser_str)

        data = parser.unpack_from(self.data, self.parse_offset)
        if len(data) != len(self.EXTENDED_FIELDS):
            logging.error('%s: Header field count mismatch', self.class_name())
            return False

        for index in range(0, len(data)):
            setattr(self, self.EXTENDED_FIELDS[index].attr, data[index])

        self.parse_offset += parser.size

        return True

    # WDC2 string field offsets are not relative to the string block, but
    # rather relative to the position of the (data, field index) tuple of the
    # record in the file
    # TODO: Is the computation field width aware?
    def get_string(self, offset, record_id, field_index):
        # Offset map files use inline strings, so use WDC1 (direct offset) strings
        if self.has_offset_map():
            return super().get_string(offset, record_id, field_index)

        if offset == 0:
            return None

        start_offset = self.offset_records + offset + self.record_size * record_id + field_index * 4
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

        # After those, the set of records in the file
        self.data_offset = self.offset_records

        if self.has_offset_map():
            # String block follows immediately after the base data block
            self.string_block_offset = 0
            running_offset = self.offset_map_offset + (self.last_id - self.first_id + 1) * 6
        else:
            # Note, string block offset in WDC2 is not really interesting as
            # string offsets in records are in terms of the data offset
            self.string_block_offset = self.data_offset + self.record_size * self.records
            running_offset = self.data_offset + self.record_size * self.records + self.string_block_size

        # Next, ID block follows immmediately after the new record + string block
        self.id_block_offset = running_offset
        running_offset = self.id_block_offset + self.id_block_size

        # Then, a possible clone block 
        self.clone_block_offset = running_offset
        running_offset = self.clone_block_offset + self.clone_block_size

        # And finally, the "foreign key" block for the whole file
        self.key_block_offset = running_offset

        logging.debug('Computed offsets, data=%d, strings=%d, ids=%d, ' +
                      'clones=%d, column_infos=%d, column_datas=%d, sparse_datas=%d, ' +
                      'keys=%d',
            self.data_offset, self.string_block_offset, self.id_block_offset, self.clone_block_offset,
            self.column_info_block_offset, self.column_data_block_offset, self.sparse_block_offset,
            self.key_block_offset)

        return self.key_block_offset + self.key_block_size

    def fields_str(self):
        fields = []

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

        fields.append('flags={:#06x}'.format(self.flags))
        fields.append('id_index={}'.format(self.id_index))
        fields.append('total_fields={}'.format(self.total_fields))
        fields.append('ofs_record_packed_data={}'.format(self.packed_data_offset))

        fields.append('wdc2_unk1={}'.format(self.wdc2_unk1))
        fields.append('column_info_block_size={}'.format(self.column_info_block_size))
        fields.append('sparse_block_size={}'.format(self.sparse_block_size))
        fields.append('column_data_block_size={}'.format(self.column_data_block_size))
        fields.append('extended_fields={}'.format(self.extended_fields))

        if self.extended_fields:
            fields.append('wdc2_unk6={}'.format(self.wdc2_unk6))
            fields.append('wdc2_unk7={}'.format(self.wdc2_unk7))
            fields.append('offset_records={}'.format(self.offset_records))
            fields.append('total_records={}'.format(self.total_records))
            fields.append('unk_string_block_size={}'.format(self.unk_string_block_size))
            fields.append('clone_block_size={}'.format(self.clone_block_size))
            fields.append('ofs_offset_map={}'.format(self.offset_map_offset))
            fields.append('id_block_size={}'.format(self.id_block_size))
            fields.append('key_block_size={}'.format(self.key_block_size))

        if not self.empty_file():
            if self.column_info_block_size > 0: fields.append('ofs_column_info_block={}'.format(self.column_info_block_offset))
            if not self.has_offset_map():       fields.append('ofs_data={}'.format(self.data_offset))
            # Offsets to blocks
            if self.string_block_size > 0:      fields.append('ofs_string_block={}'.format(self.string_block_offset))
            if self.has_id_block():             fields.append('ofs_id_block={}'.format(self.id_block_offset))
            if self.clone_block_size > 0:       fields.append('ofs_clone_block={}'.format(self.clone_block_offset))
            if self.column_data_block_size > 0: fields.append('ofs_column_data_block={}'.format(self.column_data_block_offset))
            if self.sparse_block_size > 0:      fields.append('ofs_sparse_block={}'.format(self.sparse_block_offset))
            if self.has_key_block():            fields.append('ofs_key_block={}'.format(self.key_block_offset))

        return fields

