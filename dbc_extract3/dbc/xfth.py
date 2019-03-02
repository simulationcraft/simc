import os, struct, logging, io, codecs

import dbc

from dbc import DBCRecordInfo, HeaderFieldInfo

from dbc.parser import DBCParserBase

class XFTHParser(DBCParserBase):
    __XFTH_HEADER_FIELDS = [
        HeaderFieldInfo('magic', '4s' ),
        HeaderFieldInfo('unk_1', 'I'  ),
        HeaderFieldInfo('build', 'I'  ),
        HeaderFieldInfo('unk_4', '32s')
    ]

    def is_magic(self): return self.magic == b'XFTH'

    def __init__(self, options):
        super().__init__(options, options.hotfix_file)

        self.data = None
        self.parse_offset = 0
        self.header_format = self.__XFTH_HEADER_FIELDS

        self.entries = {}
        self.parsers = {}

    # Never consider hotfix cache "empty"
    def empty_file(self):
        return False

    def get_string(self, offset, record_id, field_index):
        if offset == 0:
            return None

        end_offset = self.data.find(b'\x00', offset)

        if end_offset == -1:
            return None

        return self.data[offset:end_offset].decode('utf-8')

    # Returns dbc_id (always 0 for base), record offset into file
    def get_record_info(self, record_id, wdb_parser):
        sig = wdb_parser.table_hash

        if sig not in self.entries:
            return dbc.EMPTY_RECORD

        if record_id >= len(self.entries[sig]):
            return dbc.EMPTY_RECORD

        dbc_id = self.entries[sig][record_id]['record_id']
        key_id = 0
        if wdb_parser.has_key_block() and wdb_parser.has_id_block() and dbc_id <= wdb_parser.last_id:
            real_record_info = wdb_parser.get_dbc_info(dbc_id)
            if real_record_info:
                key_id = real_record_info.parent_id

        return DBCRecordInfo(dbc_id, self.entries[sig][record_id]['offset'],
                self.entries[sig][record_id]['length'], key_id, 0)

    def get_record(self, dbc_id, offset, size, wdb_parser):
        sig = wdb_parser.table_hash

        if sig not in self.entries:
            return None

        if sig not in self.parsers:
            if self.options.raw:
                self.parsers[sig] = wdb_parser.create_raw_parser(
                        hotfix_parser   = True)
            else:
                self.parsers[sig] = wdb_parser.create_formatted_parser(
                        hotfix_parser   = True)

        return self.parsers[sig](dbc_id, self.data, offset, size)

    def n_entries(self, wdb_parser):
        sig = wdb_parser.table_hash

        if sig not in self.entries:
            return 0

        return len(self.entries[sig])

    # Nothing really to compute block-wise
    def compute_block_offsets(self):
        return len(self.data)

    def parse_blocks(self):
        if self.options.build < dbc.WowVersion(8, 1, 5, 0):
            entry_unpacker = struct.Struct('<4sIiIIIB3s')
        else:
            entry_unpacker = struct.Struct('<4sIIIIB3s')

        n_entries = 0
        all_entries = []
        while self.parse_offset < len(self.data):
            if self.options.build < dbc.WowVersion(8, 1, 5, 0):
                magic, game_type, unk_2, length, sig, record_id, enabled, pad = \
                        entry_unpacker.unpack_from(self.data, self.parse_offset)
            else:
                magic, unk_2, sig, record_id, length, enabled, pad = \
                        entry_unpacker.unpack_from(self.data, self.parse_offset)

            if magic != b'XFTH':
                logging.error('Invalid hotfix magic %s', magic.decode('utf-8'))
                return False

            self.parse_offset += entry_unpacker.size

            entry = {
                'record_id': record_id,
                'unk_2': unk_2,
                'enabled': enabled,
                'length': length,
                'offset': self.parse_offset,
                'sig': sig,
                'pad': codecs.encode(pad, 'hex').decode('utf-8')
            }

            if self.options.build < dbc.WowVersion(8, 1, 5, 0):
                entry['game_type'] = game_type

            if sig not in self.entries:
                self.entries[sig] = []

            if enabled:
                self.entries[sig].append(entry)
                all_entries.append(entry)

            # Skip data
            self.parse_offset += length
            n_entries += 1

        if self.options.debug:
            if self.options.build < dbc.WowVersion(8, 1, 5, 0):
                for entry in sorted(all_entries, key = lambda e: (e['unk_2'], e['sig'], e['record_id'])):
                    logging.debug('entry: { %s }',
                        ('record_id=%(record_id)-6u game_type=%(game_type)u table_hash=%(sig)#.8x ' +
                         'unk_2=%(unk_2)-5u enabled=%(enabled)u, unk_4=%(unk_4)-3u unk_5=%(unk_5)-3u ' +
                         'unk_6=%(unk_6)-3u length=%(length)-3u offset=%(offset)-7u') % entry)
            else:
                for entry in sorted(all_entries, key = lambda e: (e['sig'], e['record_id'])):
                    logging.debug('entry: { %s }',
                            ('record_id=%(record_id)-6u table_hash=%(sig)#.8x ' +
                             'unk_2=%(unk_2)#.8x enabled=%(enabled)u ' +
                             'length=%(length)-3u pad=%(pad)-6s offset=%(offset)-7u') % entry)

        logging.debug('Parsed %d hotfix entries', n_entries)

        return True

