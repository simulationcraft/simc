import os, struct, logging, io

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

        # See that file exists already
    def get_string(self, offset):
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
            return DBCRecordInfo(-1, record_id, 0, 0, 0)

        if record_id >= len(self.entries[sig]):
            return DBCRecordInfo(-1, record_id, 0, 0, 0)

        dbc_id = self.entries[sig][record_id]['record_id']
        key_id = 0
        if wdb_parser.has_key_block() and wdb_parser.has_id_block() and dbc_id <= wdb_parser.last_id:
            real_record_info = wdb_parser.get_dbc_info(dbc_id)
            if real_record_info:
                key_id = real_record_info.parent_id

        return DBCRecordInfo(dbc_id, record_id, self.entries[sig][record_id]['offset'], \
                self.entries[sig][record_id]['length'], key_id)

    def get_record(self, dbc_id, offset, size, wdb_parser):
        sig = wdb_parser.table_hash

        if sig not in self.entries:
            return None

        if sig not in self.parsers:
            self.parsers[sig] = wdb_parser.create_formatted_parser(
                    hotfix_parser   = True,
                    expanded_parser = wdb_parser.class_name() in dbc.EXPANDED_HOTFIX_RECORDS)

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
        entry_unpacker = struct.Struct('<4sIiIIII')

        n_entries = 0
        while self.parse_offset < len(self.data):
            magic, unk_1, unk_2, length, sig, record_id, unk_3 = entry_unpacker.unpack_from(self.data, self.parse_offset)
            if magic != b'XFTH':
                logging.error('Invalid hotfix magic %s', magic.decode('utf-8'))
                return False

            self.parse_offset += entry_unpacker.size

            logging.debug('header: { magic=%s, unk_1=%u, unk_2=%u, length=%u, table_hash=%#.8x, dbc_id=%u, unk_3=%#.8x }',
                magic, unk_1, unk_2, length, sig, record_id, unk_3)

            n_entries += 1

            # TODO: Does length 0 indicate removal?
            if length == 0:
                continue

            entry = {
                'record_id': record_id,
                'unk_1': unk_1,
                'unk_2': unk_2,
                'unk_3': unk_3,
                'length': length,
                'offset': self.parse_offset
            }

            if sig not in self.entries:
                self.entries[sig] = []

            self.entries[sig].append(entry)

            # Skip data
            self.parse_offset += length

        logging.debug('Parsed %d hotfix entries', n_entries)

        return True

