import logging, struct, collections

from dbc import HeaderFieldInfo

from dbc.wdc4 import WDC4Parser, WDC4Section

class WDC5Parser(WDC4Parser):
    __WDC5_HEADER_FIELDS = [
        HeaderFieldInfo('magic',                  '4s'  ),
        HeaderFieldInfo('version',                'I'   ),
        HeaderFieldInfo('build_string',           '128s'),
        HeaderFieldInfo('records',                'I'   ),
        HeaderFieldInfo('fields',                 'I'   ),
        HeaderFieldInfo('record_size',            'I'   ),
        HeaderFieldInfo('string_block_size',      'I'   ),
        HeaderFieldInfo('table_hash',             'I'   ),
        HeaderFieldInfo('layout_hash',            'I'   ),
        HeaderFieldInfo('first_id',               'I'   ),
        HeaderFieldInfo('last_id',                'I'   ),
        HeaderFieldInfo('locale',                 'I'   ),
        HeaderFieldInfo('flags',                  'H'   ),
        HeaderFieldInfo('id_index',               'H'   ),
        HeaderFieldInfo('total_fields',           'I'   ),
        HeaderFieldInfo('packed_data_offset',     'I'   ),
        HeaderFieldInfo('wdc2_unk1',              'I'   ),
        HeaderFieldInfo('column_info_block_size', 'I'   ),
        HeaderFieldInfo('sparse_block_size',      'I'   ),
        HeaderFieldInfo('column_data_block_size', 'I'   ),
        HeaderFieldInfo('sections',               'I'   )
    ]

    SECTION_OBJECT = WDC4Section

    def __init__(self, options, fname):
        super().__init__(options, fname)

        # Set header format
        self.header_format = self.__WDC5_HEADER_FIELDS
        self.section_data = []

    def is_magic(self): return self.magic == b'WDC5'
