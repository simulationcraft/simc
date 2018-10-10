import os, io, struct, sys, logging

import dbc.fmt

class DBCParserBase:
    def __init__(self, options, fname):
        self.options = options

        # Data stuff
        self.file = None
        self.data = None
        self.header_format = None

        if isinstance(fname, str):
            # See that file exists already
            normalized_path = os.path.abspath(fname)
            for i in ['', '.db2']:
                if os.access(normalized_path + i, os.R_OK):
                    self.file = open(normalized_path + i, 'rb')
                    logging.debug('WDB file found at %s', self.file.name)
                    break

            if not self.file:
                logging.error('No WDB file found based on "%s"', fname)
                sys.exit(1)
        else:
            self.file = fname

        # Data format storage
        self.fmt = dbc.fmt.DBFormat(options)

    def id_format(self):
        return '%u'

    def key_format(self):
        return '%u'

    def full_name(self):
        return os.path.basename(self.file.name)

    def file_name(self):
        return os.path.basename(self.file.name).split('.')[0]

    def class_name(self):
        return os.path.basename(self.file.name).split('.')[0]

    def name(self):
        return os.path.basename(self.file.name).split('.')[0].replace('-', '_').lower()

    def empty_file(self):
        return self.records == 0

    def parse_header(self):
        if not self.header_format:
            logging.error('%s: No header format defined', self.full_name())
            return False

        # Build a parser out of the given header format
        parser_str = '<'
        for info in self.header_format:
            parser_str += info.format

        parser = struct.Struct(parser_str)

        data = parser.unpack_from(self.data, self.parse_offset)
        if len(data) != len(self.header_format):
            logging.error('%s: Header field count mismatch', self.class_name())
            return False

        for index in range(0, len(data)):
            setattr(self, self.header_format[index].attr, data[index])

        self.parse_offset += parser.size

        return self.parse_extended_header()

    def parse_extended_header(self):
        return True

    def parse_column_info(self):
        return True

    def fields_str(self):
        return []

    def __str__(self):
        return '%s { %s }' % (self.full_name(), ', '.join(self.fields_str()))

    def open(self):
        if self.data:
            return True

        if not self.file:
            return True

        self.data = self.file.read()
        self.file.close()

        if not self.parse_header():
            return False

        if self.empty_file():
            return True

        if not self.is_magic():
            logging.error('%s: Invalid data file format %s', self.class_name(), self.magic)
            return False

        if not self.parse_column_info():
            return False

        # Compute offsets to various blocks and such, warn if all data is not consumed
        consumed = self.compute_block_offsets()
        if consumed != len(self.data):
            logging.warning("%s: Unparsed bytes (%d), in file, consumed=%d, file_size=%d",
                self.class_name(), len(self.data) - consumed, consumed, len(self.data))

        if not self.parse_blocks():
            return False

        return True

    # Methods that need to be implemented for parsing

    def create_raw_parser(self):
        raise NotImplementedError()

    def create_formatted_parser(self):
        raise NotImplementedError()

    def n_records(self):
        raise NotImplementedError()

    def parse_blocks(self):
        raise NotImplementedError()

    def compute_block_offsets(self):
        raise NotImplementedError()

    def is_magic(self):
        raise NotImplementedError()

    def get_string(self, offset):
        raise NotImplementedError()

    def get_record_info(self, record_id):
        raise NotImplementedError()

    def get_dbc_info(self, dbc_id):
        raise NotImplementedError()

    def get_record(self, dbc_id, offset, size):
        raise NotImplementedError()

