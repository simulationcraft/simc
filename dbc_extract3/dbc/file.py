import os, logging, sys

import dbc

_PARSERS = {
    b'WDBC': None,
    b'WDB2': None,
    b'WDB4': dbc.parser.WDB4Parser,
    b'WDB5': dbc.parser.WDB5Parser
}

class DBCFileIterator:
    def __init__(self, f):
        self.file = f

    def __iter__(self):
        return self

    def __next__(self):
        next_item = self.file.parser.next_record()
        if not next_item:
            raise StopIteration

        return self.file.decorate(next_item)

class DBCFile:
    def __init__(self, options, filename):

        self.data_parser = None
        self.data_class = None
        self.magic = None

        self.fmt = dbc.fmt.DBFormat(options)

        self.options = options
        self.file_name = filename

        self.parser = self.__parser()

    def __parser(self):
        f = None
        # See that file exists already
        normalized_path = os.path.abspath(self.file_name)
        for i in ['', '.db2', '.dbc']:
            if os.access(normalized_path + i, os.R_OK):
                f = open(normalized_path + i, 'rb')
                break

        if not f:
            logging.error('Unable to find DBC file through %s', self.file_name)
            sys.exit(1)

        self.magic = f.read(4)
        f.close()
        parser = _PARSERS.get(self.magic, None)
        if parser:
            return parser(self.options, self.file_name)
        return None

    def wdb5(self):
        return self.magic == b'WDB5'

    def name(self):
        return os.path.basename(self.file_name).split('.')[0].replace('-', '_').lower()

    def class_name(self):
        return os.path.basename(self.file_name).split('.')[0]

    def record_class(self):
        return self.data_class

    def searchable(self):
        return self.parser.searchable()

    def open(self):
        if not self.parser.open():
            return False

        try:
            if not self.options.raw:
                self.data_parser = self.fmt.parser(self.class_name())
                self.data_class = getattr(dbc.data, self.class_name().replace('-', '_'))

            else:
                if self.wdb5():
                    self.data_class = dbc.data.RawDBCRecord
        except KeyError:
            # WDB5 we can always display something
            if self.wdb5():
                self.data_parser = self.parser.build_decoder()
                self.data_class = dbc.data.RawDBCRecord
            # Others, not so much
            else:
                logging.error("Unable to determine data format for %s, aborting ...", self.class_name())
                return False

        return True

    def decorate(self, data):
        # Output data based on data parser + class, we are sure we have those things at this point
        return self.data_class(self.parser, *data)

    def find(self, id_):
        record_data = self.parser.find(id_)
        if len(record_data[1]) > 0:
            return self.decorate(record_data)
        else:
            return 'Record with DBC id %u not found' % id_

    def __iter__(self):
        return DBCFileIterator(self)

    def __str__(self):
        return str(self.parser)

