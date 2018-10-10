import logging, struct

from dbc import HeaderFieldInfo

from dbc import wdc1
from dbc.wdc2 import WDC2Parser

class WDC3Parser(WDC2Parser):
    EXTENDED_FIELDS_1 = [
        HeaderFieldInfo('wdc2_unk6',              'I' ),
        HeaderFieldInfo('wdc2_unk7',              'I' ),
        HeaderFieldInfo('offset_records',         'I' ),
        HeaderFieldInfo('total_records',          'I' ),
        HeaderFieldInfo('unk_string_block_size',  'I' ),
        HeaderFieldInfo('offset_map_offset',      'I' ),
        HeaderFieldInfo('id_block_size',          'I' ),
        HeaderFieldInfo('key_block_size',         'I' ),
        HeaderFieldInfo('unk_wdc3_1',             'I' ),
        HeaderFieldInfo('clone_block_entries',    'I' )
    ]

    def is_magic(self): return self.magic == b'WDC3'

    def __init__(self, options, fname):
        super().__init__(options, fname)

    def parse_extended_header(self):
        if self.extended_fields == 0:
            return True

        elif self.extended_fields == 1:
            # WDC2 has some conditional header information that depends on the "extended_fields" value
            parser_str = '<'
            for info in self.EXTENDED_FIELDS_1:
                parser_str += info.format

            parser = struct.Struct(parser_str)

            data = parser.unpack_from(self.data, self.parse_offset)
            if len(data) != len(self.EXTENDED_FIELDS_1):
                logging.error('%s: Header field count mismatch', self.class_name())
                return False

            for index in range(0, len(data)):
                setattr(self, self.EXTENDED_FIELDS_1[index].attr, data[index])

            self.parse_offset += parser.size

            # Retain support for old stuff, set clone_block_size too
            self.clone_block_size = self.clone_block_entries * wdc1._CLONE_INFO.size
        # No support for extended_fields == 2
        else:
            return False

        return True

    def fields_str(self):
        fields = super().fields_str()

        fields.append('unk_wdc3_1={}'.format(self.unk_wdc3_1))
        fields.append('clone_block_entries={}'.format(self.clone_block_entries))

        return fields
