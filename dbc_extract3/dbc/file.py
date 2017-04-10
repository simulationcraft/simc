import os, logging, sys

import dbc

_PARSERS = {
    b'WDBC': None,
    b'WDB2': None,
    b'WDB4': dbc.parser.WDB4Parser,
    b'WDB5': dbc.parser.WDB5Parser,
    b'WDB6': dbc.parser.WDB6Parser,
    b'WCH5': dbc.parser.LegionWCHParser,
    b'WCH6': dbc.parser.LegionWCHParser,
    b'WCH7': dbc.parser.WCH7Parser,
    b'WCH8': dbc.parser.WCH7Parser
}

class DBCacheIterator:
    def __init__(self, f, wdb_parser):
        self._data_class = getattr(dbc.data, wdb_parser.class_name().replace('-', '_'))
        self._parser = f.parser
        self._wdb_parser = wdb_parser
        self._records = f.parser.n_entries(wdb_parser)

        self._record = 0

    def __iter__(self):
        return self

    def __next__(self):
        if self._record == self._records:
            raise StopIteration

        dbc_id, offset, size = self._parser.get_record_info(self._wdb_parser, self._record)
        data = self._parser.get_record(self._wdb_parser, offset, size)
        self._record += 1

        return self._data_class(self._parser, dbc_id, data)

class DBCache:
    def __init__(self, options):
        self.options = options
        self.parser = dbc.parser.DBCacheParser(options)

    def open(self):
        if not self.parser.open():
            return False

        return True

    def entries(self, wdb_parser):
        return DBCacheIterator(self, wdb_parser)

class DBCFileIterator:
    def __init__(self, f):
        self._file = f
        self._parser = f.parser
        self._decorator = f.record_class()

        self._record = 0
        self._n_records = self._parser.n_records()

    def __iter__(self):
        return self

    def __next__(self):
        if self._record == self._n_records:
            raise StopIteration

        dbc_id, offset, size = self._parser.get_record_info(self._record)
        data = self._parser.get_record(offset, size)
        self._record += 1

        return self._decorator(self._parser, dbc_id, data)

class DBCFile:
    def __init__(self, options, filename, wdb_file = None):

        self.data_class = None
        self.magic = None

        self.fmt = dbc.fmt.DBFormat(options)

        self.options = options
        self.file_name = filename

        self.parser = self.__parser(filename, wdb_file)

    def __parser(self, file_name, wdb_file = None):
        f = None
        # See that file exists already
        normalized_path = os.path.abspath(file_name)
        for i in ['', '.db2', '.dbc', '.adb']:
            if os.access(normalized_path + i, os.R_OK):
                f = open(normalized_path + i, 'rb')
                break

        if not f:
            logging.error('Unable to find DBC file through %s', file_name)
            sys.exit(1)

        self.magic = f.read(4)
        f.close()
        parser = _PARSERS.get(self.magic, None)
        if not parser:
            return None

        parser_obj = None

        # WCH files need special handling. If we are viewing one, we beed to
        # find a "template wdb file" for the wch file to be able to properly
        # parse the entries
        if b'WCH' in self.magic:
            if wdb_file:
                parser_obj = parser(self.options, wdb_file, file_name)
            else:
                if self.options.type == 'view' and not self.options.wdb_file:
                    logging.error('Unable to parse WCH file %s without --wdbfile parameter',
                            self.file_name)
                    sys.exit(1)

                logging.debug('Opening template wdb file %s', self.options.wdb_file)

                wdb_parser = self.__parser(self.options.wdb_file)
                if not wdb_parser.open():
                    return None

                parser_obj = parser(self.options, wdb_parser, file_name)
        else:
            parser_obj = parser(self.options, file_name)

        return parser_obj

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
                self.data_class = getattr(dbc.data, self.class_name().replace('-', '_'))

            else:
                if self.parser.raw_outputtable():
                    self.data_class = dbc.data.RawDBCRecord
        except KeyError:
            # WDB5 we can always display something
            if self.wdb5():
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

