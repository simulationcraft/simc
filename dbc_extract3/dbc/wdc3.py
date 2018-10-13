import logging, struct, collections

from dbc import HeaderFieldInfo

from dbc import wdc1
from dbc.wdc2 import WDC2Parser, WDC2Section

class WDC3Section(WDC2Section):
    def __str__(self):
        return ('Section{}: key_id={:#018x}, ptr_records={}, total_records={}, sz_string_block={}, ' +
               'ptr_blocks={}, sz_id_block={}, sz_key_block={}, offset_map_entries={}, clone_entries={} ').format(
                    self.id,
                    self.tact_key_id, self.ptr_records, self.total_records, self.string_block_size,
                    self.ptr_blocks, self.id_block_size, self.key_block_size, self.offset_map_entries,
                    self.clone_block_entries
        )

    def set_data(self, data):
        # Redo the fields
        self.tact_key_id = data[0]
        self.ptr_records = data[1]
        self.total_records = data[2]
        self.string_block_size = data[3]
        # Note, used to be called (for us) "offset map offset" in WDC2
        self.ptr_blocks = data[4]
        self.id_block_size = data[5]
        self.key_block_size = data[6]
        self.offset_map_entries = data[7]
        self.clone_block_entries = data[8]

        self.clone_block_size = self.clone_block_entries * wdc1._CLONE_INFO.size
        self.ptr_offset_map = 0
        self.ptr_id_map = 0

    # WDC3 changes the meaning of the "offset map offset" field (from our
    # perspective) to mean the offset where the records end. In WDC2, it
    # pointed to the offset where the actual offset map starts. WDC3 has moved
    # the offset map (presumably) as the last block of the DB2 file. It could
    # also be moved to before key block, however it is for sure after id and
    # clone blocks.
    def compute_block_offsets(self):
        if self.ptr_blocks == 0:
            # Note, string block offset in WDC2 is not really interesting as
            # string offsets in records are in terms of the data offset
            self.ptr_string_block = self.ptr_records + self.parser.record_size * self.total_records
            running_offset = self.ptr_string_block + self.string_block_size
        else:
            self.ptr_string_block = 0
            running_offset = self.ptr_blocks

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

        if self.ptr_blocks > 0:
            self.ptr_offset_map = running_offset
            running_offset = self.ptr_offset_map + self.offset_map_entries * 6

            self.ptr_id_map = running_offset
            running_offset = self.ptr_id_map + self.offset_map_entries * 4

        if self.key_block_size > 0:
            self.ptr_key_block = running_offset
            running_offset = self.ptr_key_block + self.key_block_size
        else:
            self.ptr_key_block = 0

        logging.debug('Computed offsets for section=%d, records=%d, ' +
                      'strings=%d, ids=%d, clones=%d, offset_map=%d, offset_ids=%d, keys=%d',
            self.id, self.ptr_records, self.ptr_string_block,
            self.ptr_id_block, self.ptr_clone_block, self.ptr_offset_map, self.ptr_id_map, self.ptr_key_block)

        return running_offset

    # Offset map parsing changes in WDC3. It is no longer a sparse map
    # (lastid-firstid+1 entries with empty records), but seems to rather be
    # identical in size to the number of entries in id block.
    def parse_offset_map(self):
        if self.ptr_blocks == 0:
            return True

        id_parser = struct.Struct('<I')

        for record_id in range(0, len(self.record_dbc_id_table)):
            map_ofs = self.ptr_offset_map + record_id * wdc1._ITEMRECORD.size
            id_ofs = self.ptr_id_map + record_id * id_parser.size

            data_offset, size = wdc1._ITEMRECORD.unpack_from(self.parser.data, map_ofs)
            dbc_id = id_parser.unpack_from(self.parser.data, id_ofs)[0]
            if size == 0:
                continue

            self.offset_map.append((dbc_id, data_offset, size))

        return True

class WDC3Parser(WDC2Parser):
    SECTION_FIELDS = [
        HeaderFieldInfo('tact_key_id',           'Q' ),
        HeaderFieldInfo('ptr_records',           'I' ),
        HeaderFieldInfo('total_records',         'I' ),
        HeaderFieldInfo('string_block_size',     'I' ),
        # Seems to have changed to the offset where id block, clone block and such start
        HeaderFieldInfo('ptr_blocks',            'I' ),
        HeaderFieldInfo('id_block_size',         'I' ),
        HeaderFieldInfo('key_block_size',        'I' ),
        HeaderFieldInfo('offset_map_entries',    'I' ),
        HeaderFieldInfo('clone_block_entries',   'I' )
    ]

    SECTION_OBJECT = WDC3Section

    def is_magic(self): return self.magic == b'WDC3'

