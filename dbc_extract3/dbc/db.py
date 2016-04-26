import os, logging

import dbc

class DBCDB(dict):
    def __init__(self, obj = None):
        dict.__init__(self)
        self.__obj = obj

    def __missing__(self, key):
        if self.__obj:
            return self.__obj.default()
        else:
            raise KeyError

class DataStore:
    def __init__(self, options):
        self.options = options
        self.databases = {}
        self.initializers = {}

    def path(self, fn):
        return os.path.join(self.options.path, fn)

    def get(self, fn):
        if fn in self.databases:
            return self.databases[fn]

        dbcf = dbc.file.DBCFile(self.options, self.path(fn))
        if not dbcf.open():
            logging.error("Failed to open %s, exiting", fn)
            sys.exit(1)

        dbase = DBCDB(dbcf.record_class())

        for record in dbcf:
            dbase[record.id] = record

        self.databases[fn] = dbase
        return dbase

    def link(self, source, source_key, target, target_attr, validator = None):
        initializer_key = '|'.join([source, source_key, target, target_attr])
        if initializer_key in self.initializers:
            return

        logging.debug('Linking %s::%s to %s::%s', source, source_key, target, target_attr)

        source_db = self.get(source)
        target_db = self.get(target)

        for id_, data in source_db.items():
            v = getattr(data, source_key, 0)
            if isinstance(source_key, str):
                v = getattr(data, source_key, 0)
            elif isinstance(source_key, types.FunctionType) or isinstance(source_key, types.MethodType):
                v = source_key(source_db, data, target_db, target_attr)
            if v == 0:
                continue

            target = target_db[v]
            if target.id != v:
                continue

            target.add_link(target_attr, data)

        self.initializers[initializer_key] = True

