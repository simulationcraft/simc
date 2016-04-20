import configparser, sys, importlib, logging, types

import dbc

def UniqueValidator(source_db, source_data, target_db, target_data, target_attr):
    if isinstance(target_attr, list) and len(target_attr) > 1:
        return False
    elif isinstance(target_attr, dbc.data.RawDBCRecord):
        return target_attr.id == 0
    else:
        return target_attr == None or target_attr == 0

class DataStore:
    def __init__(self, options):
        self.options = options
        self.databases = {}
        self.initializers = {}

    def get(self, fn):
        if fn in self.databases:
            return self.databases[fn]

        dbcf = dbc.file.DBCFile(self.options, fn)
        if not dbcf.open():
            logging.error("Failed to open %s, exiting", fn)
            sys.exit(1)

        dbase = dbc.db.DBCDB(dbcf.record_class())

        for record in dbcf:
            dbase[record.id] = record

        self.databases[fn] = dbase
        return dbase

    def link(self, source, source_key, target, target_attr, validator = None):
        initializer_key = '|'.join([source, source_key, target, target_attr])
        if initializer_key in self.initializers:
            return

        source_db = self.get(source)
        target_db = self.get(target)

        for id_, data in source_db.items():
            v = getattr(data, source_key, 0)
            if v == 0:
                continue

            target = target_db[v]
            if target.id != v:
                continue

            attr = getattr(target, target_attr, None)
            if validator and not validator(source_db, data, target_db, target, attr):
                logging.warning("Data %s fails uniqueness check for %s (field already has %s)",
                    target, data, attr)
                continue

            if isinstance(attr, types.FunctionType) or \
               isinstance(attr, types.LambdaType) or \
               isinstance(attr, types.MethodType):
                attr(data)
            elif isinstance(attr, list):
                attr.append(data)
            else:
                setattr(target, target_attr, data)

        self.initializers[initializer_key] = True

class Config:
    def __init__(self, options):
        self.options = options
        self.config = configparser.ConfigParser()
        self.data_store = DataStore(self.options)

        self.generators = {}
        self.generator_objects = {}

    def open(self):
        if len(self.options.args) < 1:
            return False

        self.config.read(self.options.args[0])

        default_module_path = self.config.get('general', 'module_base')
        for section in self.config.sections():
            if section == 'general':
                continue

            for i in self.config.get(section, 'generators').split():
                self.generators[i] = getattr(importlib.import_module(default_module_path), i)

        for generator, obj in self.generators.items():
            self.generator_objects[generator] = obj(self.options, self.data_store)

        for generator, obj in self.generator_objects.items():
            obj.initialize()


