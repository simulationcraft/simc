import os, logging, types, sys
from collections import defaultdict

import dbc

from dbc.constants import HotfixType

# Global data store for the extractor invocation
__DBSTORE = None

class DBCDB(dict):
    def __init__(self, obj = None, parser = None):
        super().__init__()

        self.__obj = obj
        self.__parser = parser

        self.__parent = defaultdict(dict)
        self.__references = defaultdict(lambda: defaultdict(dict))

        self.parent_info = dbc.data._FORMATDB.parent_dbs(parser.class_name())

        # Instantiate the default object for the database
        self.default()

    def parser(self):
        return self.__parser

    def default(self):
        return self.__obj.default(self.__parser)

    def records_for_parent(self, parent_id):
        records = self.__parent.get(parent_id, ())
        if not isinstance(records, tuple):
            records = tuple(sorted(records.values(), key = lambda e: e._id))
            self.__parent[parent_id] = records
        return records

    def records_for_reference(self, referencedb_name, reference_id):
        records = self.__references.get(referencedb_name, {}).get(reference_id, ())
        if not isinstance(records, tuple):
            records = tuple(sorted(records.values(), key = lambda e: e._id))
            self.__references[referencedb_name][reference_id] = records
        return records

    def __add_parent_entry(self, value):
        self.__parent[value.id_parent][value.id] = value

    def __add_reference_entry(self, parent_db, parent_id, value):
        self.__references[parent_db][parent_id][value.id] = value

    def __delitem__(self, key):
        super().__delitem__(key)

        # Delete entry from parent record mappings
        for parent_id, values in self.__parent.items():
            if key in values:
                del values[key]

        # Delete references to this record from other databases
        for parent_db, parent_ids in self.__references.items():
            for parent_id, values in parent_ids.items():
                if key in values:
                    del values[key]

    # Collects parent information if we can
    def __setitem__(self, key, value):
        super().__setitem__(key, value)

        for parent_info in self.parent_info:
            parent_db = parent_info['db']
            key = parent_info.get('key', None) or 'id_parent'
            n_elements = parent_info['elements']

            if key == 'id_parent':
                if not self.__parser.has_key_block():
                    raise ValueError(
                        'Key-block based parent db defined for "{}", but no key block'.format(
                            self.__parser.class_name()))
                else:
                    self.__add_parent_entry(value)

            elif n_elements == 1:
                parent_id = getattr(value, key, None)
                if parent_id == 0:
                    continue

                if parent_id == None:
                    raise ValueError('Unable to find field "{}" for parenting "{}"'.format(
                        key, self.__parser.class_name()))

                self.__add_reference_entry(parent_db, parent_id, value)

            else:
                for element_index in range(1, n_elements + 1):
                    key_str = '{}_{}'.format(key, element_index)

                    parent_id = getattr(value, key_str, None)
                    if parent_id == 0:
                        continue

                    if parent_id == None:
                        raise ValueError('Unable to find field "{}" for parenting "{}"'.format(
                            key_str, self.__parser.class_name()))

                    self.__add_reference_entry(parent_db, parent_id, value)

    def __missing__(self, key):
        if self.__obj:
            return self.__obj.default(self.__parser)
        else:
            raise KeyError

class DataStore:
    def __init__(self, options):
        self.options = options
        self.databases = {}
        self.initializers = {}
        self.cache = None

        if options.hotfix_file:
            self.cache = dbc.file.HotfixFile(options)
            if not self.cache.open():
                sys.exit(1)

    def __hotfix_fields(self, orig, hotfix):
        if orig.id != hotfix.id:
            hotfix.add_hotfix(-1, orig)
            return -1

        hotfix_fields = 0
        for idx in range(0, len(orig._fo)):
            fmt = orig._fo[idx]
            name = orig._fi[idx]
            if 'S' in fmt and orig._dbcp.get_string(orig._d[idx], orig._id, idx) != hotfix._dbcp.get_string(hotfix._d[idx], orig._id, idx):
                # Blacklist check
                hotfix_fields |= (1 << idx)
                hotfix.add_hotfix(orig._fi[idx], orig)
            elif 'S' not in fmt and orig._d[idx] != hotfix._d[idx]:
                # Blacklist check
                hotfix_fields |= (1 << idx)
                hotfix.add_hotfix(orig._fi[idx], orig)

        return hotfix_fields

    def __apply_hotfixes(self, dbc_parser, database):
        for record, hotfix_header in self.cache.entries(dbc_parser):
            if hotfix_header['state'] == HotfixType.REMOVED and database[hotfix_header['record_id']].id:
                if self.options.debug:
                    logging.debug('%s REMOVE: %s', dbc_parser.file_name(),
                            database[hotfix_header['record_id']])

                del database[hotfix_header['record_id']]

                continue

            try:
                hotfix_data = self.__hotfix_fields(database[record.id], record)
                # Add some additional information for debugging purposes
                if self.options.debug and hotfix_data:
                    if database[record.id].id == record.id:
                        logging.debug('%s REPLACE OLD: %s',
                            dbc_parser.file_name(), database[record.id])
                        logging.debug('%s REPLACE NEW: %s',
                            dbc_parser.file_name(), record)
                    else:
                        logging.debug('%s ADD: %s',
                            dbc_parser.file_name(), record)

                if hotfix_data:
                    record._flags = hotfix_data
                    database[record.id] = record
            except Exception as e:
                logging.error('Error while parsing %s: record=%s, error=%s',
                    dbc_parser.class_name(), record, e)
                traceback.print_exc()
                sys.exit(1)

    def path(self, fn):
        return os.path.join(self.options.path, fn)

    def get(self, fn):
        if fn in self.databases:
            return self.databases[fn]

        dbcf = dbc.file.DBCFile(self.options, self.path(fn))
        if not dbcf.open():
            logging.error("Failed to open %s, exiting", fn)
            sys.exit(1)

        dbase = DBCDB(dbcf.record_class(), dbcf.parser)

        for record in dbcf:
            dbase[record.id] = record

        self.databases[fn] = dbase

        # Hotfix entries if a cache file has been given
        if self.cache:
            self.__apply_hotfixes(dbcf.parser, dbase)

        return dbase

    def link(self, source, source_key, target, target_attr, validator = None):
        key = [source]
        if isinstance(source_key, str):
            key += [source_key]
        elif isinstance(source_key, types.FunctionType) or isinstance(source_key, types.MethodType):
            key += [source_key.__name__]
        else:
            logging.warn('Invalid source key %s, not linking %s::%s to %s::%s',
                source_key, source, source_key, target, target_attr)
            return

        key += [target, target_attr]

        initializer_key = '|'.join(key)
        if initializer_key in self.initializers:
            return

        logging.debug('Linking %s::%s to %s::%s', source, source_key, target, target_attr)

        source_db = self.get(source)
        target_db = self.get(target)

        for id_, data in source_db.items():
            v = 0
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

# Instantiate a data store with options. First invocation of the function with a
# non-null options param will instantiate the object
def datastore(options = None):
    global __DBSTORE

    if not __DBSTORE and not options:
        return None

    if not __DBSTORE:
        __DBSTORE = DataStore(options)

    return __DBSTORE

