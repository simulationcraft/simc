import sys, os, re, types, html.parser, urllib, datetime, signal, json, pdb, pathlib, csv, logging, io, fnmatch, traceback

import dbc.db, dbc.data, dbc.constants, dbc.parser, dbc.file

# Special hotfix flags for spells to mark that the spell has hotfixed effects or powers
SPELL_EFFECT_HOTFIX_PRESENT = 0x8000000000000000
SPELL_POWER_HOTFIX_PRESENT  = 0x4000000000000000

# Special hotfix map values to indicate an entry is new (added completely through the hotfix entry)
SPELL_HOTFIX_MAP_NEW  = 0xFFFFFFFFFFFFFFFF
EFFECT_HOTFIX_MAP_NEW = 0xFFFFFFFF
POWER_HOTFIX_MAP_NEW  = 0xFFFFFFFF

# Client does not get certain hotfix data, so we need to blacklist specific
# fields in specific DB2 files to not hotfix themselves
HOTFIX_FIELD_BLACKLIST = {
}

def escape_string(tmpstr):
    tmpstr = tmpstr.replace("\\", "\\\\")
    tmpstr = tmpstr.replace("\"", "\\\"")
    tmpstr = tmpstr.replace("\n", "\\n")
    tmpstr = tmpstr.replace("\r", "\\r")

    return tmpstr

def hotfix_fields(orig, hotfix):
    if orig.id != hotfix.id:
        hotfix.add_hotfix(-1, orig)
        return -1

    hotfix_fields = 0
    for idx in range(0, len(orig._fo)):
        fmt = orig._fo[idx]
        name = orig._fi[idx]
        if 'S' in fmt and orig._dbcp.get_string(orig._d[idx]) != hotfix._dbcp.get_string(hotfix._d[idx]):
            # Blacklist check
            if name not in HOTFIX_FIELD_BLACKLIST.get(orig._dbcp.class_name(), []):
                hotfix_fields |= (1 << idx)
                hotfix.add_hotfix(orig._fi[idx], orig)
        elif 'S' not in fmt and orig._d[idx] != hotfix._d[idx]:
            # Blacklist check
            if name not in HOTFIX_FIELD_BLACKLIST.get(orig._dbcp.class_name(), []):
                hotfix_fields |= (1 << idx)
                hotfix.add_hotfix(orig._fi[idx], orig)

    return hotfix_fields

def apply_hotfixes(opts, file_name, dbc_file, database):
    if not opts.cache_dir:
        return

    pattern = '%s.*' % dbc_file.class_name()
    potential_files = []
    # Get all cache files
    for dirpath, dirnames, filenames in os.walk(opts.cache_dir):
        for filename in filenames:
            if fnmatch.fnmatch(filename, pattern):
                potential_files.append(os.path.join(dirpath, filename))

    cache_dbc_files = []
    for potential_file in potential_files:
        cache_dbc_file = dbc.file.DBCFile(opts, potential_file, dbc_file.parser)
        if not cache_dbc_file.open():
            logging.warn('Unable to open cache file %s', potential_file)
            continue

        # Sanity check table hash and layout hash against the actual client
        # data file. They must match for the cache to be used
        if cache_dbc_file.parser.table_hash != dbc_file.parser.table_hash or \
            cache_dbc_file.parser.layout_hash != dbc_file.parser.layout_hash:
            logging.debug('Table or Layout hashes do not match, table_hash: cache=%#.8x client_data=%#.8x, layout_hash: cache=%#.8x, client_data=%#.8x',
                cache_dbc_file.parser.table_hash, dbc_file.parser.table_hash,
                cache_dbc_file.parser.layout_hash, dbc_file.parser.layout_hash)
            continue

        # Empty cache files are kinda pointless
        if cache_dbc_file.parser.n_records() == 0:
            continue

        cache_dbc_files.append(cache_dbc_file)

    # We have a set of cache files parsed, sort them into ascending timestamp
    # order. The timestamp in the cache file header defines the last time the
    # cache file is written .. higher timestamps = more fresh caches.
    cache_dbc_files.sort(key = lambda v: v.parser.timestamp)

    # Then, overwrite any existing data in the parameter 'database' with the
    # cached data. This may also include adding new data.
    for cache_file in cache_dbc_files:
        logging.debug('Applying hotfixes from %s', cache_file.file_name)
        for record in cache_file:
            try:
                hotfix_data = hotfix_fields(database[record.id], record)
                # Add some additional information for debugging purposes
                if opts.debug and hotfix_data:
                    if database[record.id].id == record.id:
                        logging.debug('%s (%d) REPLACE OLD: %s',
                            cache_file.file_name, cache_file.parser.timestamp, database[record.id])
                        logging.debug('%s (%d) REPLACE NEW: %s',
                            cache_file.file_name, cache_file.parser.timestamp, record)
                    else:
                        logging.debug('%s (%d) ADD: %s',
                            cache_file.file_name, cache_file.parser.timestamp, record)

                if hotfix_data:
                    record._flags = hotfix_data
                    database[record.id] = record
            except Exception as e:
                logging.error('Error while parsing %s: record=%s, error=%s',
                    cache_file.class_name(), record, e)
                traceback.print_exc()
                sys.exit(1)

def output_hotfixes(generator, data_str, hotfix_data):
    generator._out.write('#define %s%s_HOTFIX%s_SIZE (%d)\n\n' % (
        (generator._options.prefix and ('%s_' % generator._options.prefix) or '').upper(),
        data_str.upper(),
        (generator._options.suffix and ('_%s' % generator._options.suffix) or '').upper(),
        len(hotfix_data.keys())
    ))

    generator._out.write('// %d %s hotfix entries, wow build level %d\n' % (
        len(hotfix_data.keys()), data_str, generator._options.build ))
    generator._out.write('static hotfix::client_hotfix_entry_t __%s%s_hotfix%s_data[] = {\n' % (
        generator._options.prefix and ('%s_' % generator._options.prefix) or '',
        data_str,
        generator._options.suffix and ('_%s' % generator._options.suffix) or ''
    ))

    for id in sorted(hotfix_data.keys()) + [0]:
        entry = None
        if id > 0:
            entry = hotfix_data[id]
        # Add a zero entry at the end so we can conveniently loop to
        # the end of the array on the C++ side
        else:
            entry = [(0, 'i', 0, 0)]

        for field_id, field_format, orig_field_value, new_field_value in entry:
            orig_data_str = ''
            cur_data_str = ''
            if field_format in ['I', 'H', 'B']:
                orig_data_str = '%uU' % orig_field_value
                cur_data_str = '%uU' % new_field_value
            elif field_format in ['i', 'h', 'b']:
                orig_data_str = '%d' % orig_field_value
                cur_data_str = '%d' % new_field_value
            elif field_format in ['f']:
                orig_data_str = '%f' % orig_field_value
                cur_data_str = '%f' % new_field_value
            else:
                if orig_field_value == 0:
                    orig_data_str = 'nullptr'
                else:
                    orig_data_str = '"%s"' % escape_string(orig_field_value)
                if new_field_value == 0:
                    cur_data_str = 'nullptr'
                else:
                    cur_data_str = '"%s"' % escape_string(new_field_value)
            generator._out.write('  { %6u, %2u, %s, %s },\n' % (
                id, field_id, orig_data_str, cur_data_str
            ))

    generator._out.write('};\n\n')

# Generic linker
def link(source_db, source_key, target_db, target_attr):
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

class CSVDataGenerator(object):
    def __init__(self, options, csvs):
        self._options = options
        self._csv_options = {}
        if type(csvs) == dict:
            self._csv_options[csvs['file']] = csvs
            self._dbc = [ csvs['file'], ]
        else:
            for v in csvs:
                self._csv_options[v['file']] = v
            self._dbc = sorted(self._csv_options.keys())

    def struct_name(self, dbc = None):
        d = (dbc and dbc or self._dbc[0]).split('.')[0]
        return re.sub(r'([A-Z]+)', r'_\1', d).lower()

    def generate_header(self, dbc):
        dimensions = ''
        if len(self.values(dbc)) > 1:
            dimensions = '[][%d]' % self.max_rows(dbc)
        else:
            dimensions = '[%d]' % self.max_rows(dbc)
        return 'static double _%s%s%s%s = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self.struct_name(dbc),
            self._options.suffix and ('_%s' % self._options.suffix) or '',
            dimensions)

    def generate(self):
        for dbc in self._dbc:
            data = list(getattr(self, self.dbname(dbc)))
            max_rows = self.max_rows(dbc)

            if self.comment(dbc):
                self._out.write(self.comment(dbc))
            self._out.write(self.generate_header(dbc))

            segments = self.values(dbc)
            segment_formats = self.value_formats(dbc)
            for segment_idx in range(0, len(segments)):
                # Last entry is the fixed data
                if len(segments) > 1 and segment_idx < len(segments) and segments[segment_idx] != None:
                    self._out.write('  // %s\n' % segments[segment_idx])

                if len(segments) > 1:
                    self._out.write('  {\n')
                self._out.write('    ')

                fmt = segment_formats[segment_idx]
                name = segments[segment_idx]

                for row in data:
                    level = int(row[self.key(dbc)])
                    self._out.write('%s,\t' % (fmt % row.get(name, 0)))
                    if level % 5 == 0:
                        self._out.write('// %4u\n' % level)
                        if level < max_rows - 1:
                            self._out.write('    ')

                    if level >= max_rows:
                        break

                if len(segments) > 1:
                    self._out.write('  },\n')

            self._out.write('};\n\n')

    def key(self, dbc = None):
        k = dbc and dbc or list(self._csv_options.keys())[0]
        return self._csv_options.get(k, {}).get('key', 'Level')

    def max_rows(self, dbc = None):
        k = dbc and dbc or list(self._csv_options.keys())[0]
        return self._csv_options.get(k, {}).get('max_rows', self._options.level)

    def comment(self, dbc = None):
        k = dbc and dbc or list(self._csv_options.keys())[0]
        return self._csv_options.get(k, {}).get('comment', '')

    def values(self, dbc = None):
        k = dbc and dbc or list(self._csv_options.keys())[0]
        return self._csv_options.get(k, {}).get('values', [])

    def value_keys(self, dbc = None):
        k = dbc and dbc or list(self._csv_options.keys())[0]
        values = []
        for v in self._csv_options.get(k, {}).get('values', []):
            if type(v) == tuple:
                values.append(v[0])
            else:
                values.append(v)

        return values

    def value_formats(self, dbc = None):
        k = dbc and dbc or list(self._csv_options.keys())[0]
        values = []
        for v in self._csv_options.get(k, {}).get('values', []):
            if type(v) == tuple:
                values.append(v[1])
            else:
                values.append('%s')

        return values

    def dbname(self, csvfile):
        return '_' + csvfile.split('.')[0].lower().replace('-', '_') + '_db'

    def initialize(self):
        if self._options.output:
            self._out = pathlib.Path(self._options.output).open('w')
            if not self._out.writable():
                print('Unable to write to file "%s"' % self._options.output)
                return False
        elif self._options.append:
            self._out = pathlib.Path(self._options.append).open('a')
            if not self._out.writable():
                print('Unable to write to file "%s"' % self._options.append)
                return False
        else:
            self._out = sys.stdout

        for i in self._dbc:
            v = os.path.abspath(os.path.join(self._options.path, i))
            dbcp = csv.DictReader(open(v, 'r'), delimiter = '\t')

            if self.dbname(i) not in dir(self):
                setattr(self, self.dbname(i), dbcp)

        return True

class DataGenerator(object):
    _class_names = [ None, 'Warrior', 'Paladin', 'Hunter', 'Rogue',     'Priest', 'Death Knight', 'Shaman', 'Mage',  'Warlock', 'Monk',       'Druid', 'Demon Hunter'  ]
    _class_masks = [ None, 0x1,       0x2,       0x4,      0x8,         0x10,     0x20, 0x40,     0x80,    0x100,     0x200,        0x400, 0x800   ]
    _race_names  = [ None, 'Human',   'Orc',     'Dwarf',  'Night Elf', 'Undead', 'Tauren',       'Gnome',  'Troll', 'Goblin',  'Blood Elf', 'Draenei' ] + [ None ] * 10 + [ 'Worgen', None, 'Pandaren', 'Pandaren', 'Pandaren', None ]
    _race_masks  = [ None, 0x1,       0x2,       0x4,      0x8,         0x10,     0x20,           0x40,     0x80,    0x100,     0x200,       0x400     ] + [ None ] * 10 + [ 0x200000, None, 0x800000, 0x1000000, 0x2000000, None ]
    _pet_names   = [ None, 'Ferocity', 'Tenacity', None, 'Cunning' ]
    _pet_masks   = [ None, 0x1,        0x2,        None, 0x4       ]

    def __init__(self, options, data_store = None):
        self._options = options
        self._data_store = data_store
        self._out = None

        self._class_map = { }
        # Build some maps to help us output things
        for i in range(0, len(DataGenerator._class_names)):
            if not DataGenerator._class_names[i]:
                continue

            self._class_map[DataGenerator._class_names[i]] = i
            self._class_map[1 << (i - 1)] = i
            #self._class_map[DataGenerator._class_masks[i]] = i

        self._race_map = { }
        for i in range(0, len(DataGenerator._race_names)):
            if not DataGenerator._race_names[i]:
                continue

            self._race_map[DataGenerator._race_names[i]] = i
            self._race_map[1 << (i - 1)] = i

        #print self._class_map, self._race_map
        if self._options.output:
            self._out = pathlib.Path(self._options.output).open('w')
            if not self._out.writable():
                print('Unable to write to file "%s"' % self._options.output)
                return False
        elif self._options.append:
            self._out = pathlib.Path(self._options.append).open('a')
            if not self._out.writable():
                print('Unable to write to file "%s"' % self._options.append)
                return False
        else:
            self._out = sys.stdout


    def format_str(self, string):
        return '%s%s%s' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            string,
            self._options.suffix and ('_%s' % self._options.suffix) or '' )

    def file_path(self, basename):
        return os.path.abspath(os.path.join(self._options.path, basename))

    def attrib_name(self, basename):
        return basename.lower().replace('-', '_')

    def set_output(self, obj, append = False):
        if self._out and self._out != sys.stdout:
            self._out.close()

        if isinstance(obj, io.IOBase):
            self._out = obj
        elif isinstance(obj, str):
            self._out = pathlib.Path(obj).open(append and 'a' or 'w')
            if not self._out.writable():
                logging.error('Output file %s is not writable', obj)
                return False
        elif obj == None:
            self._out = sys.stdout

        return True

    def close(self):
        if self._out and self._out != sys.stdout:
            self._out.close()

    def initialize(self):
        for i in self._dbc:
            dbcf = None
            if self._data_store:
                dbase = self._data_store.get(i)
                if '_%s_db' % self.attrib_name(i) not in dir(self):
                    setattr(self, '_%s_db' % self.attrib_name(i), dbase)
            else:
                dbcf = dbc.file.DBCFile(self._options, self.file_path(i))
                if not dbcf.open():
                    return False

                if '_%s_db' % dbcf.name() not in dir(self):
                    setattr(self, '_%s_db' % dbcf.name(), dbc.db.DBCDB(dbcf.record_class()))

                dbase = getattr(self, '_%s_db' % dbcf.name())

                for record in dbcf:
                    try:
                        dbase[record.id] = record
                    except:
                        print('breakage', record, dbcf)
                        print(dbcf, record, type(record), record._fi)
                        sys.exit(1)

                apply_hotfixes(self._options, self.file_path(i), dbcf, dbase)

        return True

    def filter(self):
        return None

    def generate(self, ids = None):
        return ''

class RealPPMModifierGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        super().__init__(options, data_store)

        self._dbc = [ 'ChrSpecialization', 'SpellProcsPerMinute', 'SpellProcsPerMinuteMod', 'SpellAuraOptions' ]
        self._specmap = { 0: 'SPEC_NONE' }

    def initialize(self):
        DataGenerator.initialize(self)

        for i, data in self._chrspecialization_db.items():
            if data.class_id > 0:
                self._specmap[i] = '%s_%s' % (
                    DataGenerator._class_names[data.class_id].upper().replace(" ", "_"),
                    data.name.upper().replace(" ", "_"),
                )

        return True

    def generate(self, ids = None):
        output_data = []

        for i, data in self._spellprocsperminutemod_db.items():
            #if data.id_chr_spec not in self._specmap.keys() or data.id_chr_spec == 0:
            #    continue

            spell_id = 0
            for aopts_id, aopts_data in self._spellauraoptions_db.items():
                if aopts_data.id_ppm != data.id_ppm:
                    continue

                spell_id = aopts_data.id_spell
                if spell_id == 0:
                    continue

                output_data.append((data.id_chr_spec, data.coefficient, spell_id, data.unk_1))

        output_data.sort(key = lambda v: v[2])

        self._out.write('#define %sRPPMMOD%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(output_data)))
        self._out.write('// %d RPPM Modifiers, wow build level %d\n' % (len(output_data), self._options.build))
        self._out.write('static struct rppm_modifier_t __%srppmmodifier%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or ''))

        for data in output_data + [(0, 0, 0, 0)]:
            self._out.write('  { %6u, %4u, %2u, %7.4f },\n' % (data[2], data[0], data[3], data[1]))

        self._out.write('};\n')

class SpecializationEnumGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'ChrSpecialization' ]

        super().__init__(options, data_store)

    def generate(self, ids = None):
        enum_ids = [
            [ None, None, None, None ], # pets come here
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
        ]

        spec_translations = [
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
        ]

        spec_to_idx_map = [ ]
        max_specialization = 0
        for spec_id, spec_data in self._chrspecialization_db.items():
            if spec_data.class_id > 0:
                spec_name = '%s_%s' % (
                    DataGenerator._class_names[spec_data.class_id].upper().replace(" ", "_"),
                    spec_data.name.upper().replace(" ", "_"),
                )

                if spec_data.index > max_specialization:
                    max_specialization = spec_data.index

                if len(spec_to_idx_map) < spec_id + 1:
                    spec_to_idx_map += [ -1 ] * ( ( spec_id - len(spec_to_idx_map) ) + 1 )

                spec_to_idx_map[ spec_id ] = spec_data.index
            # Ugly but works for us for now
            elif spec_data.name in [ 'Ferocity', 'Cunning', 'Tenacity' ]:
                spec_name = 'PET_%s' % (
                    spec_data.name.upper().replace(" ", "_")
                )
            else:
                continue

            for i in range(0, (max_specialization + 1) - len(enum_ids[ spec_data.class_id ] ) ):
                enum_ids[ spec_data.class_id ].append( None )

            enum_ids[spec_data.class_id][spec_data.index] = { 'id': spec_id, 'name': spec_name }

        spec_arr = []
        self._out.write('enum specialization_e {\n')
        self._out.write('  SPEC_NONE              = 0,\n')
        self._out.write('  SPEC_PET               = 1,\n')
        for cls in range(0, len(enum_ids)):
            if enum_ids[cls][0] == None:
                continue

            for spec in range(0, len(enum_ids[cls])):
                if enum_ids[cls][spec] == None:
                    continue

                enum_str = '  %s%s= %u,\n' % (
                    enum_ids[cls][spec]['name'],
                    ( 23 - len(enum_ids[cls][spec]['name']) ) * ' ',
                    enum_ids[cls][spec]['id'] )

                self._out.write(enum_str)
                spec_arr.append('%s' % enum_ids[cls][spec]['name'])
        self._out.write('};\n\n')

        spec_idx_str = ''
        for i in range(0, len(spec_to_idx_map)):
            if i % 25 == 0:
                spec_idx_str += '\n  '

            spec_idx_str += '%2d' % spec_to_idx_map[i]
            if i < len(spec_to_idx_map) - 1:
                spec_idx_str += ','
                if (i + 1) % 25 != 0:
                    spec_idx_str += ' '

        # Ugliness abound, but the easiest way to iterate over all specs is this ...
        self._out.write('namespace specdata {\n')
        # enums can be negative. using int avoids warnings when comparing.
        # src: http://stackoverflow.com/questions/159034/are-c-enums-signed-or-unsigned/159308#159308
        self._out.write('static const int n_specs = %u;\n\n' % len(spec_arr))
        self._out.write('static const int spec_to_idx_map_len = %u;\n\n' % len(spec_to_idx_map))
        self._out.write('static const specialization_e __specs[%u] = {\n  %s\n};\n\n' % (len(spec_arr), ', \n  '.join(spec_arr)))
        self._out.write('static const int __idx_specs[%u] = {%s\n};\n\n' % (len(spec_to_idx_map), spec_idx_str))
        self._out.write('} // namespace specdata\n')

class SpecializationListGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'ChrSpecialization' ]

        super().__init__(options, data_store)

    def generate(self, ids = None):
        enum_ids = [
            [ None, None, None, None ], # pets come here
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
        ]

        spec_translations = [
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
        ]

        max_specialization = 0
        for spec_id, spec_data in self._chrspecialization_db.items():
            if spec_data.class_id > 0:
                spec_name = '%s_%s' % (
                    DataGenerator._class_names[spec_data.class_id].upper().replace(" ", "_"),
                    spec_data.name.upper().replace(" ", "_"),
                )
            elif spec_data.name in [ 'Ferocity', 'Cunning', 'Tenacity' ]:
                spec_name = 'PET_%s' % (
                    spec_data.name.upper().replace(" ", "_")
                )
            else:
                continue

            if spec_data.index > max_specialization:
                max_specialization = spec_data.index

            for i in range(0, (max_specialization + 1) - len(enum_ids[spec_data.class_id] ) ):
                enum_ids[spec_data.class_id].append( None )

            enum_ids[spec_data.class_id][spec_data.index] = { 'id': spec_id, 'name': spec_name }

        self._out.write('#define MAX_SPECS_PER_CLASS (%u)\n' % (max_specialization + 1))
        self._out.write('#define MAX_SPEC_CLASS  (%u)\n\n' % len(enum_ids))

        self._out.write('static specialization_e __class_spec_id[MAX_SPEC_CLASS][MAX_SPECS_PER_CLASS] = \n{\n')
        for cls in range(0, len(enum_ids)):
            if enum_ids[cls][0] == None:
                self._out.write('  {\n')
                self._out.write('    SPEC_NONE,\n')
                self._out.write('  },\n')
                continue

            self._out.write('  {\n')
            for spec in range(0, len(enum_ids[cls])):
                if enum_ids[cls][spec] == None:
                    self._out.write('    SPEC_NONE,\n')
                    continue

                self._out.write('    %s,\n' % enum_ids[cls][spec]['name'])

            self._out.write('  },\n')

        self._out.write('};\n\n')

class TalentDataGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        super().__init__(options, data_store)

        self._dbc = [ 'Spell', 'Talent', 'ChrSpecialization' ]

    def filter(self):
        ids = [ ]

        for talent_id, talent_data in self._talent_db.items():
            # Make sure at least one spell id is defined
            if talent_data.id_spell == 0:
                continue

            # Make sure the "base spell" exists
            if not self._spell_db.get(talent_data.id_spell):
                continue

            ids.append(talent_id)

        return ids

    def generate(self, ids = None):
        # Sort keys
        ids.sort()

        self._out.write('#define %sTALENT%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(ids)))
        self._out.write('// %d talents, wow build %d\n' % ( len(ids), self._options.build ))
        self._out.write('static struct talent_data_t __%stalent%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '' ))

        index = 0
        for id in ids + [ 0 ]:
            talent     = self._talent_db[id]
            spell      = self._spell_db[talent.id_spell]
            if not spell.id and talent.id_spell > 0:
                continue

            if( index % 20 == 0 ):
                self._out.write('//{ Name                                ,    Id, Flgs,  Class, Spc, Col, Row, SpellID, ReplcID, S1 },\n')

            fields = spell.field('name')
            fields += talent.field('id')
            fields += [ '%#.2x' % 0 ]
            fields += [ '%#.04x' % (DataGenerator._class_masks[talent.class_id] or 0) ]
            fields += talent.field('spec_id')

            fields += talent.field('col','row', 'id_spell', 'id_replace' )
            # Pad struct with empty pointers for direct rank based spell data access
            fields += [ ' 0' ]

            try:
                self._out.write('  { %s },\n' % (', '.join(fields)))
            except:
                sys.stderr.write('%s\n' % fields)
                sys.exit(1)

            index += 1

        self._out.write('};')

class RulesetItemUpgradeGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'RulesetItemUpgrade' ]

        super().__init__(options, data_store)

    def filter(self):
        return sorted(self._rulesetitemupgrade_db.keys()) + [0]

    def generate(self, ids = None):
        self._out.write('// Item upgrade rules, wow build level %d\n' % self._options.build)

        self._out.write('static struct item_upgrade_rule_t __%sitem_upgrade_rule%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or ''
        ))

        for id_ in ids + [ 0 ]:
            rule = self._rulesetitemupgrade_db[id_]
            self._out.write('  { %s },\n' % (', '.join(rule.field('id', 'id_upgrade_base', 'id_item'))))

        self._out.write('};\n\n')

class ItemUpgradeDataGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'ItemUpgrade' ]

        super().__init__(options, data_store)

    def generate(self, ids = None):
        self._out.write('// Upgrade rule data, wow build level %d\n' % self._options.build)

        self._out.write('static struct item_upgrade_t __%supgrade_rule%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or ''
        ))

        for id_ in sorted(self._itemupgrade_db.keys()) + [ 0 ]:
            upgrade = self._itemupgrade_db[id_]
            self._out.write('  { %s },\n' % (', '.join(upgrade.field('id', 'upgrade_group', 'prev_id', 'upgrade_ilevel'))))

        self._out.write('};\n\n')

class ItemDataGenerator(DataGenerator):
    _item_blacklist = [
        17,    138,   11671, 11672,                 # Various non-existing items
        27863, 27864, 37301, 38498,
        40948, 41605, 41606, 43336,
        43337, 43362, 43384, 55156,
        55159, 65015, 65104, 51951,
        51953, 52313, 52315,
        68711, 68713, 68710, 68709,                 # Tol Barad trinkets have three versions, only need two, blacklist the "non faction" one
        62343, 62345, 62346, 62333,                 # Therazane enchants that have faction requirements
        50338, 50337, 50336, 50335,                 # Sons of Hodir enchants that have faction requirements
        50370, 50368, 50367, 50369, 50373, 50372,   # Wotlk head enchants that have faction requirements
        62367, 68719, 62368, 68763, 68718, 62366,   # Obsolete Cataclysm head enchants
        68721, 62369, 62422, 68722,                 #
        43097,                                      #
    ]

    _item_name_blacklist = [
        "^(Lesser |)Arcanum of (Rum|Con|Ten|Vor|Rap|Foc|Pro)",
        "^Scroll of Enchant",
        "^Enchant ",
        "Deprecated",
        "DEPRECATED",
        "QA",
        "zzOLD",
        "NYI",
    ]

    _item_whitelist = [
        133578,     # Hearty Feast (7.0)
        133579,     # Lavish Suramar Feast (7.0)
    ]

    _type_flags = {
        "Raid Finder"   : 0x01,
        "Heroic"        : 0x02,
        "Flexible"      : 0x04,
        "Elite"         : 0x10, # Meta type
        "Timeless"      : 0x10,
        "Thunderforged" : 0x10,
        "Warforged"     : 0x10,

        # Combinations
        "Heroic Thunderforged" : 0x12,
        "Heroic Warforged"     : 0x12,
    }

    def __init__(self, options, data_store = None):
        self._dbc = [ 'Item-sparse', 'Item', 'ItemEffect', 'SpellEffect', 'Spell', 'JournalEncounterItem', 'ItemNameDescription' ]

        super().__init__(options, data_store)

    def initialize(self):
        if not DataGenerator.initialize(self):
            return False

        # Reverse map various things to Spell records so we can easily generate output
        link(self._spelleffect_db, 'id_spell', self._spell_db, 'add_effect')

        # Various Item-related data model linkages
        link(self._itemeffect_db, 'id_item', self._item_sparse_db, 'spells')
        link(self._journalencounteritem_db, 'id_item', self._item_sparse_db, 'journal')

        return True

    def filter(self):
        ids = []
        for item_id, data in self._item_sparse_db.items():
            blacklist_item = False

            classdata = self._item_db[item_id]
            if item_id in self._item_blacklist:
                continue

            for pat in self._item_name_blacklist:
                if data.name and re.search(pat, data.name):
                    blacklist_item = True

            if blacklist_item:
                continue

            filter_ilevel = True

            # Item no longer in game
            # LEGION: Apparently no longer true?
            #if data.flags_1 & 0x10:
            #    continue

            # Various things in armors/weapons
            if item_id in self._item_whitelist:
                filter_ilevel = False
            elif classdata.classs in [ 2, 4 ]:
                # All shirts
                if data.inv_type == 4:
                    filter_ilevel = False
                # All tabards
                elif data.inv_type == 19:
                    filter_ilevel = False
                # All heirlooms
                elif data.quality == 7:
                    filter_ilevel = False
                else:
                    # On-use item, with a valid spell (and cooldown)
                    for item_effect in data.get_links('spells'):
                        if item_effect.trigger_type == 0 and item_effect.id_spell > 0 and (item_effect.cooldown_group_duration > 0 or item_effect.cooldown_duration > 0):
                            filter_ilevel = False
                            break
            # Gems
            elif classdata.classs == 3:
                if data.gem_props == 0:
                    continue
                else:
                    filter_ilevel = False
            # Consumables
            elif classdata.classs == 0:
                # Potions, Elixirs, Flasks. Simple spells only.
                if classdata.has_value('subclass', [1, 2, 3]):
                    for item_effect in data.get_links('spells'):
                        spell = self._spell_db[item_effect.id_spell]
                        if not spell.has_effect('type', 6):
                            continue

                        # Grants armor, stats, rating or direct trigger of spells
                        if not spell.has_effect('sub_type', [13, 22, 29, 99, 189, 465, 43, 42]):
                            continue

                        filter_ilevel = False
                # Food
                elif classdata.has_value('subclass', 5):
                    for item_effect in data.get_links('spells'):
                        spell = self._spell_db[item_effect.id_spell]
                        for effect in spell._effects:
                            if not effect:
                                continue

                            if effect.sub_type == 23:
                                filter_ilevel = False
                elif classdata.subclass == 3:
                    filter_ilevel = False
                # Permanent Item Enchants (not strictly needed for simc, but
                # paperdoll will like them)
                elif classdata.subclass == 6:
                   filter_ilevel = False
                else:
                    continue
            # Hunter scopes and whatnot
            elif classdata.classs == 7:
                if classdata.has_value('subclass', 3):
                    for item_effect in data.get_links('spells'):
                        spell = self._spell_db[item_effect.id_spell]
                        for effect in spell._effects:
                            if not effect:
                                continue

                            if effect.type == 53:
                                filter_ilevel = False
            # Only very select quest-item permanent item enchantments
            elif classdata.classs == 12:
                valid = False
                for spell in data.get_links('spells'):
                    spell_id = spell.id_spell
                    if spell_id == 0:
                        continue

                    spell = self._spell_db[spell_id]
                    for effect in spell._effects:
                        if not effect or effect.type != 53:
                            continue

                        valid = True
                        break

                if valid:
                    filter_ilevel = False
                else:
                    continue
            # All glyphs
            elif classdata.classs == 16:
                filter_ilevel = False
            # All tabards
            elif data.inv_type == 19:
                filter_ilevel = False
            # All heirlooms
            elif data.quality == 7:
                filter_ilevel = False

            # Item-level based non-equippable items
            if filter_ilevel and data.inv_type == 0:
                continue
            # All else is filtered based on item level
            elif filter_ilevel and (data.ilevel < self._options.min_ilevel or data.ilevel > self._options.max_ilevel):
                continue

            ids.append(item_id)

        return ids

    def generate(self, ids = None):
        #if (self._options.output_type == 'cpp'):
        self.generate_cpp(ids)
        #elif (self._options.output_type == 'js'):
        #    return self.generate_json(ids)
        #else:
        #    return "Unknown output type"

    def generate_cpp(self, ids = None):
        ids.sort()

        self._out.write('#define %sITEM%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(ids)))
        self._out.write('// %d items, ilevel %d-%d, wow build level %d\n' % (
            len(ids), self._options.min_ilevel, self._options.max_ilevel, self._options.build))
        self._out.write('static struct item_data_t __%sitem%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or ''))

        index = 0
        for id in ids + [ 0 ]:
            item = self._item_sparse_db[id]
            item2 = self._item_db[id]

            if not item.id and id > 0:
                sys.stderr.write('Item id %d not found\n' % id)
                continue

            if(index % 20 == 0):
                self._out.write('//{    Id, Name                                                   ,     Flags1,     Flags2, Type,Level,ReqL,ReqSk, RSkL,Qua,Inv,Cla,SCl,Bnd, Delay, DmgRange, Modifier,  ClassMask,   RaceMask, { ST1, ST2, ST3, ST4, ST5, ST6, ST7, ST8, ST9, ST10}, {  SV1,  SV2,  SV3,  SV4,  SV5,  SV6,  SV7,  SV8,  SV9, SV10 }, {  SId1,  SId2,  SId3,  SId4,  SId5 }, {Soc1,Soc2,Soc3 }, GemP,IdSBon,IdSet,IdSuf },\n')

            fields = item.field('id', 'name')
            fields += item.field('flags_1', 'flags_2')

            flag_types = 0x00

            for entry in item.get_links('journal'):
                if entry.flags_1 == 0x10:
                    flag_types |= self._type_flags['Raid Finder']
                elif entry.flags_1 == 0xC:
                    flag_types |= self._type_flags['Heroic']

            desc = self._itemnamedescription_db[item.id_name_desc]
            flag_types |= self._type_flags.get(desc.desc, 0)

            fields += [ '%#.2x' % flag_types ]
            fields += item.field('ilevel', 'req_level', 'req_skill', 'req_skill_rank', 'quality', 'inv_type')
            fields += item2.field('classs', 'subclass')
            fields += item.field( 'bonding', 'delay', 'dmg_range', 'item_damage_modifier', 'race_mask', 'class_mask')
            fields += [ '{ %s }' % ', '.join(item.field('stat_type_1', 'stat_type_2', 'stat_type_3', 'stat_type_4', 'stat_type_5', 'stat_type_6', 'stat_type_7', 'stat_type_8', 'stat_type_9', 'stat_type_10')) ]
            fields += [ '{ %s }' % ', '.join(item.field('stat_val_1', 'stat_val_2', 'stat_val_3', 'stat_val_4', 'stat_val_5', 'stat_val_6', 'stat_val_7', 'stat_val_8', 'stat_val_9', 'stat_val_10')) ]
            fields += [ '{ %s }' % ', '.join(item.field('stat_alloc_1', 'stat_alloc_2', 'stat_alloc_3', 'stat_alloc_4', 'stat_alloc_5', 'stat_alloc_6', 'stat_alloc_7', 'stat_alloc_8', 'stat_alloc_9', 'stat_alloc_10')) ]
            fields += [ '{ %s }' % ', '.join(item.field('stat_socket_mul_1', 'stat_socket_mul_2', 'stat_socket_mul_3', 'stat_socket_mul_4', 'stat_socket_mul_5', 'stat_socket_mul_6', 'stat_socket_mul_7', 'stat_socket_mul_8', 'stat_socket_mul_9', 'stat_socket_mul_10')) ]

            spells = self._itemeffect_db[0].field('id_spell') * 5
            trigger_types = self._itemeffect_db[0].field('trigger_type') * 5
            cooldown_duration = self._itemeffect_db[0].field('cooldown_duration') * 5
            cooldown_group = self._itemeffect_db[0].field('cooldown_group') * 5
            cooldown_group_duration = self._itemeffect_db[0].field('cooldown_group_duration') * 5
            for spell in item.get_links('spells'):
                spells[ spell.index ] = spell.field('id_spell')[ 0 ]
                trigger_types[ spell.index ] = spell.field('trigger_type')[ 0 ]
                cooldown_duration[ spell.index ] = spell.field('cooldown_duration')[ 0 ]
                cooldown_group[ spell.index ] = spell.field('cooldown_group')[ 0 ]
                cooldown_group_duration[ spell.index ] = spell.field('cooldown_group_duration')[ 0 ]

            fields += [ '{ %s }' % ', '.join(trigger_types) ]
            fields += [ '{ %s }' % ', '.join(spells) ]
            fields += [ '{ %s }' % ', '.join(cooldown_duration) ]
            fields += [ '{ %s }' % ', '.join(cooldown_group) ]
            fields += [ '{ %s }' % ', '.join(cooldown_group_duration) ]

            fields += [ '{ %s }' % ', '.join(item.field('socket_color_1', 'socket_color_2', 'socket_color_3')) ]
            fields += item.field('gem_props', 'socket_bonus', 'item_set', 'rand_suffix', 'scale_stat_dist', 'id_artifact' )

            self._out.write('  { %s },\n' % (', '.join(fields)))

            index += 1
        self._out.write('};\n\n')

    def generate_json(self, ids = None):

        ids.sort()

        s2 = dict()
        s2['wow_build'] = self._options.build
        s2['min_ilevel'] = self._options.min_ilevel
        s2['max_ilevel'] = self._options.max_ilevel
        s2['num_items'] = len(ids)

        s2['items'] = list()

        for id in ids + [0]:
            item = self._item_sparse_db[id]
            item2 = self._item_db[id]

            if not item.id and id > 0:
                sys.stderr.write('Item id %d not found\n' % id)
                continue

            # Aand, hack classs 12 (quest item) to be 0, 6
            # so we get item enchants clumped in the same category, sigh ..
            if item2.classs == 12:
                item2.classs = 0
                item2.subclass = 6

            item_entry = dict()
            item_entry['id'] = int(item.field('id')[0])
            item_entry['name'] = item.field('name')[0].strip()

            # put the flags in as strings instead of converting them to ints.
            # this makes it more readable
            item_entry['flags'] = item.field('flags')[0].strip()
            item_entry['flags_2'] = item.field('flags_2')[0].strip()

            flag_types = 0x00

            if hasattr(item, 'journal'):
                if item.journal.flags_1 == 0x10:
                    flag_types |= self._type_flags['Raid Finder']
                elif item.journal.flags_1 == 0xC:
                    flag_types |= self._type_flags['Heroic']

            desc = self._itemnamedescription_db[item.id_name_desc]
            flag_types |= self._type_flags.get(desc.desc, 0)

            # put the flags in as strings instead of converting them to ints.
            # this makes it more readable
            item_entry['flag_types'] = '%#.2x' % flag_types
            item_entry['ilevel'] = int(item.field('ilevel')[0])
            item_entry['req_level'] = int(item.field('req_level')[0])
            item_entry['req_skill'] = int(item.field('req_skill')[0])
            item_entry['req_skill_rank'] = int(item.field('req_skill_rank')[0])
            item_entry['quality'] = int(item.field('quality')[0])
            item_entry['inv_type'] = int(item.field('inv_type')[0])

            item_entry['classs'] = int(item2.field('classs')[0])
            item_entry['subclass'] = int(item2.field('subclass')[0])

            item_entry['bonding'] = int(item.field('bonding')[0])
            item_entry['delay'] = int(item.field('delay')[0])
            item_entry['weapon_damage_range'] = float(item.field('weapon_damage_range')[0])
            item_entry['item_damage_modifier'] = float(item.field('item_damage_modifier')[0])

            # put the masks in as strings instead of converting them to ints.
            # this makes it more readable
            item_entry['race_mask'] = item.field("race_mask")[0].strip()
            item_entry['class_mask'] = item.field("class_mask")[0].strip()

            item_entry['stats'] = list()
            for i in range(1,10):
                type = int(item.field('stat_type_%d' % i)[0])
                if type != -1 and type != 0:
                    stats = dict()
                    stats['type'] = type
                    stats['val'] = int(item.field('stat_val_%d' % i)[0])
                    stats['alloc'] = int(item.field('stat_alloc_%d' % i)[0])
                    stats['socket_mul'] = float(item.field('stat_socket_mul_%d' % i)[0])
                    item_entry['stats'].append(stats)

            spells = self._itemeffect_db[0].field('id_spell') * 5
            trigger_types = self._itemeffect_db[0].field('trigger_type') * 5
            cooldown_category = self._itemeffect_db[0].field('cooldown_category') * 5
            cooldown_value = self._itemeffect_db[0].field('cooldown_category_duration') * 5
            cooldown_group = self._itemeffect_db[0].field('cooldown_group') * 5
            cooldown_shared = self._itemeffect_db[0].field('cooldown_group_duration') * 5
            if len(item.get_links('spells')) > 0:
                item_entry['spells'] = list()
                for spell in item.get_links('spells'):
                    spl = dict();
                    spl['id'] = int(spell.field('id_spell')[0])
                    spl['trigger_type'] = int(spell.field('trigger_type')[0])
                    spl['cooldown_category'] = int(spell.field('cooldown_category')[0])
                    spl['cooldown_category_duration'] = int(spell.field('cooldown_category_duration')[0])
                    spl['cooldown_group'] = int(spell.field('cooldown_group')[0])
                    spl['cooldown_group_duration'] = int(spell.field('cooldown_group_duration')[0])
                    item_entry['spells'].append(spl)

            item_entry['sockets'] = list()
            for i in range(1,3):
                item_entry['sockets'].append(int(item.field('socket_color_%d' % i)[0]))

            item_entry['gem_props'] = int(item.field('gem_props')[0])
            item_entry['socket_bonus'] = int(item.field('socket_bonus')[0])
            item_entry['item_set'] = int(item.field('item_set')[0])
            item_entry['rand_suffix'] = int(item.field('rand_suffix')[0])
            item_entry['scale_stat_dist'] = int(item.field('scale_stat_dist')[0])

            s2['items'].append(item_entry)

        s = json.dumps(s2)

        return s

class SpellDataGenerator(DataGenerator):
    _spell_ref_rx = r'(?:\??\(?[Saps]|@spell(?:name|desc|icon|tooltip)|\$|&)([0-9]{2,})(?:\[|(?<=[0-9a-zA-Z])|\&|\))'



    # Pattern based whitelist, these will always be added
    _spell_name_whitelist = [
        #re.compile(r'^Item\s+-\s+(.+)\s+T([0-9]+)\s+([A-z\s]*)\s*([0-9]+)P')
    ]

    # Explicitly included list of spells per class, that cannot be
    # found from talents, or based on a SkillLine category
    # The list contains a tuple ( spell_id, category[, activated ] ),
    # where category is an entry in the _class_categories class-specific
    # tuples, e.g. general = 0, specialization0..3 = 1..4 and pets as a whole
    # are 5. The optional activated parameter forces the spell to appear (if set
    # to True) or disappear (if set to False) from the class activated spell list,
    # regardless of the automated activated check.
    # Manually entered general spells ("tree" 0) do not appear in class activated lists, even if
    # they pass the "activated" check, but will appear there if the optional activated parameter
    # exists, and is set to True.
    # The first tuple in the list is for non-class related, generic spells that are whitelisted,
    # without a category
    _spell_id_list = [
        (
         109871, 109869,            # No'Kaled the Elements of Death - LFR
         107785, 107789,            # No'Kaled the Elements of Death - Normal
         109872, 109870,            # No'Kaled the Elements of Death - Heroic
         52586,  68043,  68044,     # Gurthalak, Voice of the Deeps - LFR, N, H
         109959, 109955, 109939,    # Rogue Legendary buffs for P1, 2, 3
         84745,  84746,             # Shallow Insight, Moderate Insight
         138537,                    # Death Knight Tier15 2PC melee pet special attack
         137597,                    # Legendary meta gem Lightning Strike
         137323, 137247,            # Healer legendary meta
         137331, 137326,
         146137,                    # Cleave
         146071,                    # Multistrike
         120032, 142530,            # Dancing Steel
         104993, 142535,            # Jade Spirit
         116631,                    # Colossus
         105617,                    # Alchemist's Flask
         137596,                    # Capacitance
         104510, 104423,            # Windsong Mastery / Haste buffs
         177172, 177175, 177176,    # WoD Legendary ring, phase 1(?)
         177161, 177159, 177160,    # WoD Legendary ring, phase 1(?)
         187619, 187616, 187624, 187625, # phase 2?
         143924,                    # Leech
         54861, 133022,             # Nitro boosts
         175457, 175456, 175439,    # Focus Augmentation / Hyper Augmentation / Stout Augmentation
         179154, 179155, 179156, 179157, # T17 LFR cloth dps set bonus nukes
         183950,                    # Darklight Ray (WoD 6.2 Int DPS Trinket 3 damage spell)
         184559,                    # Spirit Eruption (WoD 6.2 Agi DPS Trinket 3 damage spell)
         184279,                    # Felstorm (WoD 6.2 Agi DPS Trinket 2 damage spell)
         60235,                     # Darkmoon Card: Greatness proc
         71556, 71558, 71559, 71560,# Deathbringer's Will procs
         71484, 71485, 71492,
         45428, 45429, 45430,       # Shattered Sun Pendant procs
         45431, 45432, 45478,
         45479, 45480,
         184968,                    # Unholy coil heal (Blood dk class trinket)
         191091, 191118, 191119,    # Mark of the Loyal Druid spells
         191121, 191112, 191123,
         191124, 191146,
         214802, 214803,            # Momento of Angerboda trinket
         215476,                    # Obelisk of the Void trinket
         215695,                    # Figurehead of the Naglfar trinket 
         215407,                    # Caged Horror trinket
         191545, 191548, 191549, 191550, 191551, 191552, 191553, 191554, # Darkmoon Deck: Dominion
         191603, 191604, 191605, 191606, 191607, 191608, 191609, 191610, # Darkmoon Deck: Hellfire
         191624, 191625, 191626, 191627, 191628, 191629, 191630, 191631, # Darkmoon Deck: Immortality
         216099,                    # Elementium Bomb Squirrel Generator trinket
         211309, 213428, 219655, 226829, # Artifact passive boosts
         222517, 222518, 222519, 222520, # Nature's Call trinket
         222050,                   # Wriggling Sinew trinket
         221865,                   # Twisting Wind trinket
         221804, 221805,           # Ravaged Seed Pod trinket
         207694,                   # Symbiote Strike for Cinidaria, the Symbiote
         201639, 201640, 201641,   # 7.0 Lavish Suramar Feast food buffs
         201635, 201636, 201637,   # 7.0 Hearty Feast food buffs
         215047,                   # Terrorbound Nexus trinket
         # Eyasu's Mulligan stuff
         227394, 227397,
         227392, 227395,
         227396, 227393,
         215754, 215750,           # Magma Spit / summon spell for Spawn of Serpentrix
        ),

        # Warrior:
        (
            ( 58385,  0 ),          # Glyph of Hamstring
            ( 118779, 0, False ),   # Victory Rush heal is not directly activatable
            ( 144442, 0 ),          # T16 Melee 4 pc buff
            ( 119938, 0 ),          # Overpower
            ( 209700, 0 ),           # Void Cleave (arms artifact gold medal)
            ( 218877, 0 ),
            ( 218850, 0 ),
            ( 218836, 0 ),
            ( 218835, 0 ),
            ( 218834, 0 ),
            ( 218822, 0 ),
			( 209493, 0 ),
        ),

        # Paladin:
        (
            ( 86700, 5 ),           # Ancient Power
            ( 144581, 0 ),          # Blessing of the Guardians (prot T16 2-piece bonus)
            ( 122287, 0, True ),    # Symbiosis Wrath
            ( 42463, 0, False ),    # Seal of Truth damage id not directly activatable
            ( 114852, 0, False ),   # Holy Prism false positives for activated
            ( 114871, 0, False ),
            ( 65148, 0, False ),    # Sacred Shield absorb tick
            ( 136494, 0, False ),   # World of Glory false positive for activated
            ( 113075, 0, False ),   # Barkskin (from Symbiosis)
            ( 144569, 0, False ),   # Bastion of Power (prot T16 4-piece bonus)
            ( 130552, 0, True ),    # Harsh Word
            ( 186876, 0 ),          # echoed Divine Storm (speculative)
            ( 186805, 0 ),          # echoed Templar's Verdict (speculative)
            ( 193115, 0 ),          # Blade of Light (speculative)
            ( 180290, 0 ),          # ashen strike (speculative)
            ( 221883, 0 ),          # Divine Steed
            ( 228288, 0, True ),
			( 205729, 0 ),
        ),

        # Hunter:
        (
          ( 82928, 0 ), # Aimed Shot Master Marksman
          ( 131900, 0 ), # Murder of Crows damage spell
          ( 138374, 5 ), # T15 2pc nuke
          ( 168811, 0 ), # Sniper Training
          ( 171457, 0 ), # Chimaera Shot - Nature
          ( 90967, 0 ),  # Kill Command cooldown
          ( 157708, 2 ), # Marks Kill Shot
          ( 178875, 0 ), # BM T17 4P
          ( 188507, 0 ), # BM T18 4P
          ( 188402, 0 ), # SV T18 4P
          ( 185365, 0 ), # Hunter's Mark debuff
          ( 191043, 0 ), # MM Artifact proc
          ( 204740, 0 ), # Flanking Strike
          ( 191070, 0 ), # Call of the Hunter
          ( 217207, 0 ), # Dire Frenzy (pet)
          ( 201594, 0 ), # Stampede
        ),

        # Rogue:
        (
            ( 121474, 0 ),          # Shadow Blades off hand
            ( 113780, 0, False ),   # Deadly Poison damage is not directly activatable
            ( 89775, 0, False ),    # Hemorrhage damage is not directy activatable
            ( 86392, 0, False ),    # Main Gauche false positive for activatable
            ( 145211, 0 ),          # Subtlety Tier16 4PC proc
            ( 168908, 0 ),          # Sinister Calling: Hemorrhage
            ( 168952, 0 ),          # Sinister Calling: Crimson Tempest
            ( 168971, 0 ),          # Sinister Calling: Garrote
            ( 168963, 0 ),          # Sinister Calling: Rupture
            ( 115189, 0 ),          # Anticipation buff
            ( 157562, 0 ),          # Crimson Poison (Enhanced Crimson Tempest perk)
            ( 186183, 0 ),          # Assassination T18 2PC Nature Damage component
            ( 157957, 0 ),          # Shadow Reflection Dispatch
            ( 173458, 0 ),          # Shadow Reflection Backstab
            ( 195627, 0 ),          # Free Pistol Shot proc for Saber Slash
            # Roll the Bones buffs
            ( 199603, 0 ), ( 193358, 0 ), ( 193357, 0 ), ( 193359, 0 ), ( 199600, 0 ), ( 193356, 0 ),
            ( 193536, 0 ),          # Weaponmaster damage spell for ticks
            ( 197393, 0 ), ( 197395, 0 ), # Finality spells
            ( 192380, 0 ),          # Poisoned Knives damage component
            ( 202848, 0 ),          # Blunderbuss driver
            ( 197496, 0 ), ( 197498, 0 ), # New nightblade buffs
        ),

        # Priest:
        (   (  63619, 5 ), 			# Shadowfiend "Shadowcrawl"
            (  94472, 0 ), 			# Atonement Crit
            ( 114908, 0, False ), 	# Spirit Shell absorb
            ( 190714, 3, False ), 	# Shadow Word: Death - Insanity gain
            ( 193473, 5 ),			# Void Tendril "Mind Flay"
            ( 217676, 3 )			# Mind Spike Detonation
        ),

        # Death Knight:
        (
          ( 51963, 5 ), # gargoyle strike
          ( 66198, 0 ), # Obliterate off-hand
          ( 66196, 0 ), # Frost Strike off-hand
          ( 66216, 0 ), # Plague Strike off-hand
          ( 66188, 0 ), # Death Strike off-hand
          ( 113516, 0, True ), # Symbiosis Wild Mushroom: Plague
          ( 52212, 0, False ), # Death and Decay false positive for activatable
          ( 81277, 5 ), ( 81280, 5 ), ( 50453, 5 ), # Bloodworms heal / burst
          ( 77535, 0 ), # Blood Shield
          ( 116783, 0 ), # Death Siphon heal
          ( 96171, 0 ), # Will of the Necropolish Rune Tap buff
          ( 144948, 0 ), # T16 tank 4PC Bone Shield charge proc
          ( 144953, 0 ), # T16 tank 2PC Death Strike proc
          ( 144909, 0 ), # T16 dps 4PC frost driver spell
          ( 57330, 0, True ), # Horn of Winter needs to be explicitly put in the general tree, as our (over)zealous filtering thinks it's not an active ability
          ( 47568, 0, True ), # Same goes for Empower Rune Weapon
          ( 170205, 0 ), # Frost T17 4pc driver continued ...
          ( 187981, 0 ), ( 187970, 0 ), # T18 4pc unholy relevant spells
          ( 184982, 0 ),    # Frozen Obliteration
          ( 212333, 5 ),    # Cleaver for Sludge Belcher
          ( 212332, 5 ),    # Smash for Sludge Belcher
          ( 212338, 5 ),    # Vile Gas for Sludge Belcher
          ( 198715, 5 ),    # Val'kyr Strike for Dark Arbiter
          ( 211947, 0 ),    # Shadow Empowerment for Dark Arbiter
          ( 81141, 0 ),     # Crimson Scourge buff
          ( 212423, 5 ),    # Skulker Shot for All Will Serve
          ( 207260, 5 ),    # Arrow Spray for All Will Serve
          ( 45470, 0 ),     # Death Strike heal
          ( 196545, 0 ),    # Bonestorm heal
          ( 221847, 0 ),    # Blood Mirror damage
          ( 205164, 0 ),    # Crystalline Swords
          ( 205165, 0 ),    # More crystalline swords stuff
          ( 191730, 0 ), ( 191727, 0 ), ( 191728, 0 ), ( 191729, 0 ), # Armies of the Damned debuffs
        ),

        # Shaman:
        ( (  77451, 0 ), (  45284, 0 ), (  45297, 0 ),  #  Overloads
          ( 114074, 1 ), ( 114738, 0 ),                 # Ascendance: Lava Beam, Lava Beam overload
          ( 120687, 0 ), ( 120588, 0 ),                 # Stormlash, Elemental Blast overload
          ( 121617, 0 ),                                # Ancestral Swiftness 5% haste passive
          ( 25504, 0, False ), ( 33750, 0, False ),     # Windfury passives are not directly activatable
          ( 8034, 0, False ),                           # Frostbrand false positive for activatable
          ( 145002, 0, False ),                         # Lightning Elemental nuke
          ( 157348, 5 ), ( 157331, 5 ),                 # Storm elemental spells
          ( 159101, 0 ), ( 159105, 0 ), ( 159103, 0 ),  # Echo of the Elements spec buffs
          ( 173184, 0 ), ( 173185, 0 ), ( 173186, 0 ),  # Elemental Blast buffs
          ( 173183, 0 ),                                # Elemental Blast buffs
          ( 170512, 0 ), ( 170523, 0 ),                 # Feral Spirit windfury (t17 enhance 4pc set bonus)
          ( 189078, 0 ),                                # Gathering Vortex (Elemental t18 4pc charge)
          ( 201846, 0 ),                                # Stormfury buff
          ( 195256, 0 ), ( 195222, 0 ),                 # Stormlash stuff for legion
          ( 199054, 0 ), ( 199053, 0 ),                 # Unleash Doom, DOOM I SAY
          ( 198300, 0 ),                                # Gathering Storms buff
          ( 198830, 0 ), ( 199019, 0 ), ( 198933, 0 ),  # Doomhammer procs
          ( 197576, 0 ),                                # Magnitude ground? effect
          ( 191635, 0 ), ( 191634, 0 ),                 # Static Overload spells
          ( 202045, 0 ), ( 202044, 0 ),                 # Feral Swipe, Stomp
          ( 33750, 0 ),                                 # Offhand Windfury Attack
          ( 210801, 0 ), ( 210797, 0 ),                 # Crashing Storm
          ( 207835, 0 ), ( 213307, 0 ),                 # Stormlash auxilary spell(s)
          # Various feral spirit variation special attacks
          ( 198455, 0 ), ( 198483, 0 ), ( 198485, 0 ), ( 198480, 0 ),
          ( 191726, 0 ), ( 191732, 0 ),                 # Greater Lightning Elemental spells
          ( 214816, 0 ), ( 218559, 0 ), ( 218558, 0 ),  # Overload energize spells
          ( 219271, 0 ),                                # Icefury overload
          ( 77762, 0 ),                                 # Lava Surge cast time modifier
          ( 214134, 0 ),                                # The Deceiver's Blood Pact energize
          ( 224126, 0 ), ( 224125, 0 ), ( 224127, 0 ),  # Wolves of Doom Doom Effects, DOOM!
          ( 198506, 0 ),                                # Wolves of Doom, summon spell
          ( 197568, 0 ),                                # Lightning Rod damage spell
          ( 207998, 0 ), ( 207999, 0 ),                 # 7.0 legendary ring Eye of the Twisting Nether
        ),

        # Mage:
        (
          ( 48107, 0, False ), ( 48108, 0, False ), # Heating Up and Pyroblast! buffs
          ( 88084, 5 ), ( 88082, 5 ), ( 59638, 5 ), # Mirror Image spells.
          ( 80354, 0 ),                             # Temporal Displacement
          ( 131581, 0 ),                            # Waterbolt
          ( 7268, 0, False ),                       # Arcane missiles trigger
          ( 115757, 0, False ),                     # Frost Nova false positive for activatable
          ( 145264, 0 ),                            # T16 Frigid Blast
          ( 148022, 0 ),                            # Icicle
          ( 155152, 5 ),                            # Prismatic Crystal nuke
          ( 157978, 0 ), ( 157979, 0 ),             # Unstable magic aoe
          ( 187677, 0 ),                            # Aegwynn's Ascendance AOE
          ( 191764, 0 ), ( 191799, 0 ),             # Arcane T18 2P Pet
          ( 194432, 0 ),                            # Felo'melorn - Aftershocks
          ( 205235, 5 ),                            # Arcane Familiar attack, Arcane Assault
          ( 210833, 0 ),                            # Touch of the Magi
        ),

        # Warlock:
        (
          ( 157900, 0, True ), # grimoire: doomguard
          ( 157901, 0, True ), # grimoire: infernal
          ( 104025, 2, True ), # immolation aura
          ( 104232, 3, True ), # destruction rain of fire
          ( 111859, 0, True ), ( 111895, 0, True ), ( 111897, 0, True ), ( 111896, 0, True ), ( 111898, 2, True ), # Grimoire of Service summons
          ( 114654, 0 ),        # Fire and Brimstone nukes
          ( 108686, 0 ),
          ( 109468, 0 ),
          ( 104225, 0 ),
          ( 129476, 0 ),        # Immolation Aura
          ( 189297, 0 ),        # Demonology Warlock T18 4P Pet
          ( 189298, 0 ),        # Demonology Warlock T18 4P Pet
          ( 189296, 0 ),        # Demonology Warlock T18 4P Pet
          ( 17941, 0 ),         # Agony energize
          ( 104318, 5 ),        # Wild Imp Firebolt
          ( 205196, 5 ),        # Dreadstalker Dreadbite
          ( 226802, 3 ),        # Lord of Flames buff
          ( 196659, 3 ),        # Dimensional rift spells
          ( 196606, 2 ),        # Shadowy Inspiration buff
          ( 205231, 5 ),        # Darkglare spell
          ( 196639, 3 ),
          ( 215409, 3 ),
          ( 215276, 3 ),
          ( 187385, 3 ),
          ( 205260, 0 ),        # Soul effigy damage
		  ( 233496, 0 ),
		  ( 233497, 0 ),
		  ( 233498, 0 ),
		  ( 233499, 0 ),
        ),

        # Monk:
        (
          # General
          # Brewmaster
          ( 195630, 1 ), # Brewmaster Mastery Buff
          ( 124503, 0 ), # Gift of the Ox Orb Left
          ( 124506, 0 ), # Gift of the Ox Orb Right
          ( 178173, 0 ), # Gift of the Ox Explosion
          ( 124275, 1 ), # Light Stagger
          ( 124274, 1 ), # Medium Stagger
          ( 124273, 1 ), # Heavy Stagger
          ( 216521, 1 ), # Celestial Fortune Heal
          ( 227679, 1 ), # Face Palm
          # Mistweaver
          ( 167732, 0 ), # Tier 17 2-piece Healer Buff
          ( 228649, 2 ), # Teachings of the Monastery - Blackout Proc
          # Windwalker
          ( 116768, 3 ), # Combo Breaker: Blackout Kick
          ( 121283, 0 ), # Chi Sphere from Power Strikes
          ( 228287, 3 ), # Spinning Crane Kick's Mark of the Crane debuff
          ( 195651, 0 ), # Crosswinds Artifact trait trigger spell
          ( 196061, 0 ), # Crosswinds Artifact trait damage spell
          ( 211432, 3 ), # Tier 19 4-piece DPS Buff
          # Legendary
          ( 213114, 3 ), # Hidden Master's Forbidden Touch buff
        ),

        # Druid:
        ( (  93402, 1, True ), # Sunfire
          ( 106996, 1, True ), # Astral Storm
          ( 112071, 1, True ), # Celestial Alignment
          ( 122114, 1, True ), # Chosen of Elune
          ( 122283, 0, True ),
          ( 110807, 0, True ),
          ( 112997, 0, True ),
          ( 144770, 1, False ), ( 144772, 1, False ), # Balance Tier 16 2pc spells
          ( 150017, 5 ),       # Rake for Treants
          ( 146874, 2 ),       # Feral Rage (T16 4pc feral bonus)
          ( 124991, 0 ), ( 124988, 0 ), # Nature's Vigil
          ( 155627, 2 ),       # Lunar Inspiration
          ( 155625, 2 ),       # Lunar Inspiration Moonfire
          ( 145152, 2 ),       # Bloodtalons buff
          ( 135597, 3 ),       # Tooth and Claw absorb buff
          ( 155784, 3 ),       # Primal Tenacity buff
          ( 137542, 0 ),       # Displacer Beast buff
          ( 185321, 3 ),       # Stalwart Guardian buff (T18 trinket)
          ( 188046, 5 ),       # T18 2P Faerie casts this spell
          ( 202771, 1 ),       # Half Moon artifact power
          ( 202768, 1 ),       # Full Moon artifact power
          ( 203001, 1 ),       # Goldrinn's Fang, Spirit of Goldrinn artifact power
          ( 203958, 3 ),       # Brambles (talent) damage spell
          ( 210721, 2 ),       # Shredded Wounds (Fangs of Ashamane artifact passive)
          ( 210713, 2 ),       # Ashamane's Rake (Fangs of Ashamane artifact trait spell)
          ( 210705, 2 ),       # Ashamane's Rip (Fangs of Ashamane artifact trait spell)
          ( 210649, 2 ),       # Feral Instinct (Fangs of Ashamane artifact trait)
          ( 211140, 2 ),       # Feral tier19_2pc
          ( 211142, 2 ),       # Feral tier19_4pc
          ( 213557, 2 ),       # Protection of Ashamane ICD (Fangs of Ashamane artifact trait) 
          ( 211547, 1 ),       # Fury of Elune move spell
          ( 213771, 3 ),       # Swipe (Bear)
          ( 209406, 1 ),       # Oneth's Intuition buff
          ( 209407, 1 ),       # Oneth's Overconfidence buff
          ( 213666, 1 ),       # Echoing Stars artifact spell
          ( 69369,  2 ),       # Predatory Swiftness buff
          ( 227034, 3 ),       # Nature's Guardian heal
        ),
        # Demon Hunter:
        (
          # General
          ( 225102, 0 ), # Fel Eruption damage

          # Havoc
          ( 203557, 1 ), # Felblade proc rate
          ( 208605, 1 ), # Nemesis player buff
          ( 203796, 1 ), # Demon Blade proc
          ( 217070, 1 ), # Rage of the Illidari explosion
          ( 217060, 1 ), # Rage of the Illidari buff
          ( 202446, 1 ), # Anguish damage spell
          ( 222703, 1 ), # Fel Barrage proc rate
          ( 211796, 1 ), # Chaos Blades damage spell

          # Vengeance
          ( 209245, 2 ), # Fiery Brand damage reduction
          ( 213011, 2 ), # Charred Warblades heal
       ),
    ]

    _profession_enchant_categories = [
        165,  # Leatherworking
        171,  # Alchemy
        197,  # Tailoring
        202,  # Engineering
        333,  # Enchanting
        773,  # Inscription
    ]

    # General
    _skill_categories = [
          0,
        840,    # Warrior
        800,    # Paladin
        795,    # Hunter
        921,    # Rogue
        804,    # Priest
        796,    # Death Knight
        924,    # Shaman
        904,    # Mage
        849,    # Warlock
        829,    # Monk
        798,    # Druid
        1848,   # Demon Hunter
    ]

    _pet_skill_categories = [
        ( ),
        ( ),         # Warrior
        ( ),         # Paladin
        ( 203, 208, 209, 210, 211, 212, 213, 214, 215, 217, 218, 236, 251, 270, 653, 654, 655, 656, 763, 764, 765, 766, 767, 768, 775, 780, 781, 783, 784, 785, 786, 787, 788, 808, 811 ),       # Hunter
        ( ),         # Rogue
        ( ),         # Priest
        ( 782, ),    # Death Knight
        ( 962, 963 ),         # Shaman
        ( 805, ),    # Mage
        ( 188, 189, 204, 205, 206, 207, 761 ),  # Warlock
        ( ),         # Monk
        ( ),         # Druid
    ]

    # Specialization categories, Spec0 | Spec1 | Spec2
    # Note, these are reset for MoP
    _spec_skill_categories = [
        (),
        (  71,  72,  73,   0 ), # Warrior
        (  65,  66,  70,   0 ), # Paladin
        ( 254, 255, 256,   0 ), # Hunter
        ( 259, 260, 261,   0 ), # Rogue
        ( 256, 257, 258,   0 ), # Priest
        ( 250, 251, 252,   0 ), # Death Knight
        ( 262, 263, 264,   0 ), # Shaman
        (  62,  63,  64,   0 ), # Mage
        ( 265, 266, 267,   0 ), # Warlock
        ( 268, 270, 269,   0 ), # Monk
        ( 102, 103, 104, 105 ), # Druid
        ( 577, 581,   0,   0 ), # Demon Hunter
    ]

    _race_categories = [
        (),
        ( 754, ),                # Human     0x0001
        ( 125, ),                # Orc       0x0002
        ( 101, ),                # Dwarf     0x0004
        ( 126, ),                # Night-elf 0x0008
        ( 220, ),                # Undead    0x0010
        ( 124, ),                # Tauren    0x0020
        ( 753, ),                # Gnome     0x0040
        ( 733, ),                # Troll     0x0080
        ( 790, ),                # Goblin    0x0100? not defined yet
        ( 756, ),                # Blood elf 0x0200
        ( 760, ),                # Draenei   0x0400
        (),                      # Fel Orc
        (),                      # Naga
        (),                      # Broken
        (),                      # Skeleton
        (),                      # Vrykul
        (),                      # Tuskarr
        (),                      # Forest Troll
        (),                      # Taunka
        (),                      # Northrend Skeleton
        (),                      # Ice Troll
        ( 789, ),                # Worgen   0x200000
        (),                      # Gilnean
    ]

    _skill_category_blacklist = [
        148,                # Horse Riding
        762,                # Riding
        129,                # First aid
        183,                # Generic (DND)
    ]

    # Any spell with this effect type, will be automatically
    # blacklisted
    # http://github.com/mangos/mangos/blob/400/src/game/SharedDefines.h
    _effect_type_blacklist = [
        5,      # SPELL_EFFECT_TELEPORT_UNITS
        #10,     # SPELL_EFFECT_HEAL
        16,     # SPELL_EFFECT_QUEST_COMPLETE
        18,     # SPELL_EFFECT_RESURRECT
        25,     # SPELL_EFFECT_WEAPONS
        39,     # SPELL_EFFECT_LANGUAGE
        47,     # SPELL_EFFECT_TRADESKILL
        50,     # SPELL_EFFECT_TRANS_DOOR
        60,     # SPELL_EFFECT_PROFICIENCY
        71,     # SPELL_EFFECT_PICKPOCKET
        94,     # SPELL_EFFECT_SELF_RESURRECT
        97,     # SPELL_EFFECT_SUMMON_ALL_TOTEMS
        103,    # Grant reputation to faction
        109,    # SPELL_EFFECT_SUMMON_DEAD_PET
        110,    # SPELL_EFFECT_DESTROY_ALL_TOTEMS
        118,    # SPELL_EFFECT_SKILL
        126,    # SPELL_STEAL_BENEFICIAL_BUFF
        252,    # Some kind of teleport
        255,    # Some kind of transmog thing
    ]

    # http://github.com/mangos/mangos/blob/400/src/game/SpellAuraDefines.h
    _aura_type_blacklist = [
        1,      # SPELL_AURA_BIND_SIGHT
        2,      # SPELL_AURA_MOD_POSSESS
        5,      # SPELL_AURA_MOD_CONFUSE
        6,      # SPELL_AURA_MOD_CHARM
        7,      # SPELL_AURA_MOD_FEAR
        #8,      # SPELL_AURA_PERIODIC_HEAL
        17,     # SPELL_AURA_MOD_STEALTH_DETECT
        25,     # SPELL_AURA_MOD_PACIFY
        30,     # SPELL_AURA_MOD_SKILL (various skills?)
        #31,     # SPELL_AURA_MOD_INCREASE_SPEED
        44,     # SPELL_AURA_TRACK_CREATURES
        45,     # SPELL_AURA_TRACK_RESOURCES
        56,     # SPELL_AURA_TRANSFORM
        58,     # SPELL_AURA_MOD_INCREASE_SWIM_SPEED
        75,     # SPELL_AURA_MOD_LANGUAGE
        78,     # SPELL_AURA_MOUNTED
        82,     # SPELL_AURA_WATER_BREATHING
        91,     # SPELL_AURA_MOD_DETECT_RANGE
        98,     # SPELL_AURA_MOD_SKILL (trade skills?)
        104,    # SPELL_AURA_WATER_WALK,
        105,    # SPELL_AURA_FEATHER_FALL
        151,    # SPELL_AURA_TRACK_STEALTHED
        154,    # SPELL_AURA_MOD_STEALTH_LEVEL
        156,    # SPELL_AURA_MOD_REPUTATION_GAIN
        206,    # SPELL_AURA_MOD_FLIGHT_SPEED_xx begin
        207,
        208,
        209,
        210,
        211,
        212     # SPELL_AURA_MOD_FLIGHT_SPEED_xx ends
    ]

    _mechanic_blacklist = [
        21,     # MECHANIC_MOUNT
    ]

    _spell_blacklist = [
        3561,   # Teleports --
        3562,
        3563,
        3565,
        3566,
        3567,   # -- Teleports
        20585,  # Wisp spirit (night elf racial)
        42955,  # Conjure Refreshment
        43987,  # Ritual of Refreshment
        48018,  # Demonic Circle: Summon
        48020,  # Demonic Circle: Teleport
        69044,  # Best deals anywhere (goblin racial)
        69046,  # Pack hobgoblin (woe hogger)
        68978,  # Flayer (worgen racial)
        68996,  # Two forms (worgen racial)
    ]

    _spell_name_blacklist = [
        "^Acquire Treasure Map",
        "^Languages",
        "^Teleport:",
        "^Weapon Skills",
        "^Armor Skills",
        "^Tamed Pet Passive",
        "^Empowering$",
    ]

    _spell_families = {
        'mage': 3,
        'warrior': 4,
        'warlock': 5,
        'priest': 6,
        'druid': 7,
        'rogue': 8,
        'hunter': 9,
        'paladin': 10,
        'shaman': 11,
        'deathknight': 15,
        'monk': 53,
        'demonhunter': 107,
    }

    def __init__(self, options, data_store = None):
        super().__init__(options, data_store)

        self._dbc = [ 'Spell', 'SpellEffect', 'SpellScaling', 'SpellCooldowns', 'SpellRange',
                'SpellClassOptions', 'SpellDuration', 'SpellPower', 'SpellLevels',
                'SpellCategories', 'SpellCategory', 'Talent', 'SkillLineAbility',
                'SpellAuraOptions', 'SpellRadius', 'GlyphProperties', 'Item-sparse', 'Item',
                'SpellCastTimes', 'ItemSet', 'SpellDescriptionVariables', 'SpellItemEnchantment',
                'SpellEquippedItems', 'SpellIcon', 'SpecializationSpells', 'ChrSpecialization',
                'SpellEffectScaling', 'SpellMisc', 'SpellProcsPerMinute', 'ItemSetSpell',
                'ItemEffect', 'MinorTalent', 'ArtifactPowerRank', 'ArtifactPower', 'Artifact',
                'SpellShapeshift', 'SpellMechanic' ]

    def initialize(self):
        super().initialize()

        if self._data_store:
            self._data_store.link('SpellEffect',        'id_spell', 'Spell', 'add_effect'   )
            self._data_store.link('SpellPower',         'id_spell', 'Spell', 'power'        )
            self._data_store.link('SpellCategories',    'id_spell', 'Spell', 'categories'   )
            self._data_store.link('SpellScaling',       'id_spell', 'Spell', 'scaling'      )
            self._data_store.link('SpellLevels',        'id_spell', 'Spell', 'level'        )
            self._data_store.link('SpellCooldowns',     'id_spell', 'Spell', 'cooldown'     )
            self._data_store.link('SpellAuraOptions',   'id_spell', 'Spell', 'aura_option'  )
            self._data_store.link('SpellEquippedItems', 'id_spell', 'Spell', 'equipped_item')
            self._data_store.link('SpellClassOptions',  'id_spell', 'Spell', 'class_option' )
            self._data_store.link('SpellShapeshift',    'id_spell', 'Spell', 'shapeshift'   )
            self._data_store.link('ArtifactPowerRank',  'id_spell', 'Spell', 'artifact_power')

            self._data_store.link('SpellEffectScaling', 'id_effect', 'SpellEffect', 'scaling')

            self._data_store.link('ItemSetSpell', 'id_item_set', 'ItemSet', 'bonus')

            self._data_store.link('ItemEffect', 'id_item', 'Item-sparse', 'spells')
        else:
            # Reverse map various things to Spell records so we can easily generate output
            link(self._spelleffect_db, 'id_spell', self._spell_db, 'add_effect')
            link(self._spellpower_db, 'id_spell', self._spell_db, 'power')
            link(self._spellcategories_db, 'id_spell', self._spell_db, 'categories')
            link(self._spellscaling_db, 'id_spell', self._spell_db, 'scaling')
            link(self._spelllevels_db, 'id_spell', self._spell_db, 'level')
            link(self._spellcooldowns_db, 'id_spell', self._spell_db, 'cooldown')
            link(self._spellauraoptions_db, 'id_spell', self._spell_db, 'aura_option')
            link(self._spellequippeditems_db, 'id_spell', self._spell_db, 'equipped_item')
            link(self._spellclassoptions_db, 'id_spell', self._spell_db, 'class_option')
            link(self._spellshapeshift_db, 'id_spell', self._spell_db, 'shapeshift')
            link(self._artifactpowerrank_db, 'id_spell', self._spell_db, 'artifact_power')

            # Effect data model linkage
            link(self._spelleffectscaling_db, 'id_effect', self._spelleffect_db, 'scaling')

            # Various Item-related data model linkages
            link(self._itemeffect_db, 'id_item', self._item_sparse_db, 'spells')
            link(self._itemsetspell_db, 'id_item_set', self._itemset_db, 'bonus')

        return True

    def class_mask_by_skill(self, skill):
        for i in range(0, len(self._skill_categories)):
            if self._skill_categories[i] == skill:
                return DataGenerator._class_masks[i]

        return 0

    def class_mask_by_spec_skill(self, spec_skill):
        for i in range(0, len(self._spec_skill_categories)):
            if spec_skill in self._spec_skill_categories[i]:
                return DataGenerator._class_masks[i]

        return 0

    def class_mask_by_pet_skill(self, pet_skill):
        for i in range(0, len(self._pet_skill_categories)):
            if pet_skill in self._pet_skill_categories[i]:
                return DataGenerator._class_masks[i]

        return 0

    def race_mask_by_skill(self, skill):
        for i in range(0, len(self._race_categories)):
            if skill in self._race_categories[i]:
                return DataGenerator._race_masks[i]

        return 0

    def process_spell(self, spell_id, result_dict, mask_class = 0, mask_race = 0, state = True):
        filter_list = { }
        lst = self.generate_spell_filter_list(spell_id, mask_class, mask_race, filter_list, state)
        if not lst:
            return

        for k, v in lst.items():
            if result_dict.get(k):
                result_dict[k]['mask_class'] |= v['mask_class']
                result_dict[k]['mask_race'] |= v['mask_race']
            else:
                result_dict[k] = { 'mask_class': v['mask_class'], 'mask_race' : v['mask_race'], 'effect_list': v['effect_list'] }

    def spell_state(self, spell, enabled_effects = None):
        spell_name = spell.name

        if spell.id == 1:
            pdb.set_trace()
        # Check for blacklisted spells
        if spell.id in SpellDataGenerator._spell_blacklist:
            logging.debug("Spell id %u (%s) is blacklisted", spell.id, spell.name)
            return False

        # Check for spell name blacklist
        if spell_name:
            for p in SpellDataGenerator._spell_name_blacklist:
                if re.search(p, spell_name):
                    logging.debug("Spell id %u (%s) matches name blacklist pattern %s", spell.id, spell.name, p)
                    return False

        # Check for blacklisted spell category mechanism
        for c in spell.get_links('categories'):
            if c.mechanic in SpellDataGenerator._mechanic_blacklist:
                logging.debug("Spell id %u (%s) matches mechanic blacklist %u", spell.id, spell_name, c.mechanic)
                return False

        # Make sure we can filter based on effects even if there's no map of relevant effects
        if enabled_effects == None:
            enabled_effects = [ True ] * ( spell.max_effect_index + 1 )

        # Effect blacklist processing
        for effect_index in range(0, len(spell._effects)):
            if not spell._effects[effect_index]:
                enabled_effects[effect_index] = False
                continue

            effect = spell._effects[effect_index]

            # Blacklist by effect type
            effect_type = effect.type
            real_index = effect.index
            if effect_type in SpellDataGenerator._effect_type_blacklist:
                enabled_effects[real_index] = False

            # Blacklist by apply aura (party, raid)
            if effect_type in [ 6, 35, 65 ] and effect.sub_type in SpellDataGenerator._aura_type_blacklist:
                enabled_effects[real_index] = False

        # If we do not find a true value in enabled effects, this spell is completely
        # blacklisted, as it has no effects enabled that interest us
        if len(spell._effects) > 0 and True not in enabled_effects:
            logging.debug("Spell id %u (%s) has no enabled effects (n_effects=%u)", spell.id, spell_name, len(spell._effects))
            return False

        return True

    def generate_spell_filter_list(self, spell_id, mask_class, mask_race, filter_list = { }, state = True):
        spell = self._spell_db[spell_id]
        enabled_effects = [ True ] * ( spell.max_effect_index + 1 )

        if spell.id == 0:
            return None

        if state and not self.spell_state(spell, enabled_effects):
            return None

        filter_list[spell.id] = { 'mask_class': mask_class, 'mask_race': mask_race, 'effect_list': enabled_effects }

        spell_id = spell.id
        # Add spell triggers to the filter list recursively
        for effect in spell._effects:
            if not effect or spell_id == effect.trigger_spell:
                continue

            # Regardless of trigger_spell or not, if the effect is not enabled,
            # we do not process it
            if not enabled_effects[effect.index]:
                continue

            trigger_spell = effect.trigger_spell
            if trigger_spell > 0:
                if trigger_spell in filter_list.keys():
                    continue

                lst = self.generate_spell_filter_list(trigger_spell, mask_class, mask_race, filter_list)
                if not lst:
                    continue

                for k, v in lst.items():
                    if filter_list.get(k):
                        filter_list[k]['mask_class'] |= v['mask_class']
                        filter_list[k]['mask_race'] |= v['mask_race']
                    else:
                        filter_list[k] = { 'mask_class': v['mask_class'], 'mask_race' : v['mask_race'], 'effect_list': v['effect_list'] }

        spell_refs = re.findall(SpellDataGenerator._spell_ref_rx, spell.desc or '')
        spell_refs += re.findall(SpellDataGenerator._spell_ref_rx, spell.tt or '')
        desc_var = spell.id_desc_var
        if desc_var > 0:
            data = self._spelldescriptionvariables_db[desc_var]
            if data.id > 0:
                spell_refs += re.findall(SpellDataGenerator._spell_ref_rx, data.desc)
        spell_refs = list(set(spell_refs))

        for ref_spell_id in spell_refs:
            rsid = int(ref_spell_id)
            if rsid == spell_id:
                continue

            if rsid in filter_list.keys():
                continue

            lst = self.generate_spell_filter_list(rsid, mask_class, mask_race, filter_list)
            if not lst:
                continue

            for k, v in lst.items():
                if filter_list.get(k):
                    filter_list[k]['mask_class'] |= v['mask_class']
                    filter_list[k]['mask_race'] |= v['mask_race']
                else:
                    filter_list[k] = { 'mask_class': v['mask_class'], 'mask_race' : v['mask_race'], 'effect_list': v['effect_list'] }

        return filter_list

    def filter(self):
        ids = { }

        _start = datetime.datetime.now()
        # First, get spells from talents. Pet and character class alike
        for talent_id, talent_data in self._talent_db.items():
            mask_class = 0
            class_id = talent_data.class_id

            # These may now be pet talents
            if class_id > 0:
                mask_class = DataGenerator._class_masks[class_id]
                self.process_spell(talent_data.id_spell, ids, mask_class, 0, False)

        # Get all perks
        for perk_id, perk_data in self._minortalent_db.items():
            spell_id = perk_data.id_spell
            if spell_id == 0:
                continue

            spec_data = self._chrspecialization_db[perk_data.id_spec]
            if spec_data.id == 0:
                continue

            self.process_spell(spell_id, ids, DataGenerator._class_masks[spec_data.class_id], 0, False)

        # Get base skills from SkillLineAbility
        for ability_id, ability_data in self._skilllineability_db.items():
            mask_class_category = 0
            mask_race_category  = 0

            skill_id = ability_data.id_skill

            if skill_id in SpellDataGenerator._skill_category_blacklist:
                continue

            # Guess class based on skill category identifier
            mask_class_category = self.class_mask_by_skill(skill_id)
            if mask_class_category == 0:
                mask_class_category = self.class_mask_by_spec_skill(skill_id)

            if mask_class_category == 0:
                mask_class_category = self.class_mask_by_pet_skill(skill_id)

            # Guess race based on skill category identifier
            mask_race_category = self.race_mask_by_skill(skill_id)

            # Make sure there's a class or a race for an ability we are using
            if not ability_data.mask_class and not ability_data.mask_race and not mask_class_category and not mask_race_category:
                continue

            spell_id = ability_data.id_spell
            spell = self._spell_db[spell_id]
            if not spell.id:
                continue

            self.process_spell(spell_id, ids, ability_data.mask_class or mask_class_category, ability_data.mask_race or mask_race_category)

        # Get specialization skills from SpecializationSpells and masteries from ChrSpecializations
        for spec_id, spec_spell_data in self._specializationspells_db.items():
            # Guess class based on specialization category identifier
            spec_data = self._chrspecialization_db[spec_spell_data.spec_id]
            if spec_data.id == 0:
                continue

            spell_id = spec_spell_data.spell_id
            spell = self._spell_db[spell_id]
            if not spell.id:
                continue

            mask_class = 0
            if spec_data.class_id > 0:
                mask_class = DataGenerator._class_masks[spec_data.class_id]
            # Hunter pet classes have a class id of 0, tag them as "hunter spells" like before
            else:
                mask_class = DataGenerator._class_masks[3]

            self.process_spell(spell_id, ids, mask_class, 0, False)
            if spell_id in ids:
                ids[spell_id]['replace_spell_id'] = spec_spell_data.replace_spell_id

        for spec_id, spec_data in self._chrspecialization_db.items():
            s = self._spell_db[spec_data.id_mastery]
            if s.id == 0:
                continue

            self.process_spell(s.id, ids, DataGenerator._class_masks[spec_data.class_id], 0, False)


        # Get spells relating to item enchants, so we can populate a (nice?) list
        for enchant_id, enchant_data in self._spellitemenchantment_db.items():
            for i in range(1, 4):
                type_field_str = 'type_%d' % i
                id_field_str = 'id_property_%d' % i

                # "combat spell", "equip spell", "use spell"
                if getattr(enchant_data, type_field_str) not in [ 1, 3, 7 ]:
                    continue

                spell_id = getattr(enchant_data, id_field_str)
                if not spell_id:
                    continue

                self.process_spell(spell_id, ids, 0, 0)

        # Get spells that create item enchants
        for ability_id, ability_data in self._skilllineability_db.items():
            if ability_data.id_skill not in self._profession_enchant_categories:
                continue;

            spell = self._spell_db[ability_data.id_spell]
            if not spell.id:
                continue

            enchant_spell_id = 0
            for effect in spell._effects:
                # Grab Enchant Items and Create Items (create item will be filtered further)
                if not effect or (effect.type != 53 and effect.type != 24):
                    continue

                # Create Item, see if the created item has a spell that enchants an item, if so
                # add the enchant spell. Also grab all gem spells
                if effect.type == 24:
                    item = self._item_sparse_db[effect.item_type]
                    if not item.id or item.gem_props == 0:
                        continue

                    for spell in item.get_links('spells'):
                        id_spell = spell.id_spell
                        enchant_spell = self._spell_db[id_spell]
                        for enchant_effect in enchant_spell._effects:
                            if not enchant_effect or (enchant_effect.type != 53 and enchant_effect.type != 6):
                                continue

                            enchant_spell_id = id_spell
                            break

                        if enchant_spell_id > 0:
                            break
                elif effect.type == 53:
                    spell_item_ench = self._spellitemenchantment_db[effect.misc_value]
                    #if (spell_item_ench.req_skill == 0 and self._spelllevels_db[spell.id_levels].base_level < 60) or \
                    #   (spell_item_ench.req_skill > 0 and spell_item_ench.req_skill_value <= 375):
                    #    continue
                    enchant_spell_id = spell.id

                if enchant_spell_id > 0:
                    break

            # Valid enchant, process it
            if enchant_spell_id > 0:
                self.process_spell(enchant_spell_id, ids, 0, 0)

        # Rest of the Item enchants relevant to us, such as Shoulder / Head enchants
        for item_id, data in self._item_sparse_db.items():
            blacklist_item = False

            classdata = self._item_db[item_id]
            class_ = classdata.classs
            subclass = classdata.subclass
            if item_id in ItemDataGenerator._item_blacklist:
                continue

            name = data.name
            for pat in ItemDataGenerator._item_name_blacklist:
                if name and re.search(pat, name):
                    blacklist_item = True

            if blacklist_item:
                continue

            # Consumables, Flasks, Elixirs, Potions, Food & Drink, Permanent Enchants
            if class_ != 12 and \
               (class_ != 7 or subclass not in [3]) and \
               (class_ != 0 or subclass not in [1, 2, 3, 5, 6]):
                continue

            # Grab relevant spells from quest items, this in essence only
            # includes certain permanent enchants
            if class_ == 12:
                for spell in data.get_links('spells'):
                    spell_id = spell.id_spell
                    if spell_id == 0:
                        continue

                    spell = self._spell_db[spell_id]
                    for effect in spell._effects:
                        if not effect or effect.type != 53:
                            continue

                        self.process_spell(spell_id, ids, 0, 0)
            # Grab relevant spells from consumables as well
            elif class_ == 0:
                for item_effect in data.get_links('spells'):
                    spell_id = item_effect.id_spell
                    spell = self._spell_db[spell_id]
                    if not spell.id:
                        continue

                    # Potions and Elixirs need to apply attributes, rating or
                    # armor
                    if classdata.has_value('subclass', [1, 2, 3]) and spell.has_effect('sub_type', [13, 22, 29, 99, 189, 465, 43]):
                        self.process_spell(spell_id, ids, 0, 0)
                    # Food needs to have a periodically triggering effect
                    # (presumed to be always a stat giving effect)
                    elif classdata.has_value('subclass', 5) and spell.has_effect('sub_type', 23):
                        self.process_spell(spell_id, ids, 0, 0)
                    # Permanent enchants
                    elif classdata.has_value('subclass', 6):
                        self.process_spell(spell_id, ids, 0, 0)
            # Hunter scopes and whatnot
            elif class_ == 7:
                if classdata.has_value('subclass', 3):
                    for item_effect in data.get_links('spells'):
                        spell_id = item_effect.id_spell
                        spell = self._spell_db[spell_id]
                        for effect in spell._effects:
                            if not effect:
                                continue

                            if effect.type == 53:
                                self.process_spell(spell_id, ids, 0, 0)

        # Relevant set bonuses
        for id, set_spell_data in self._itemsetspell_db.items():
            if not SetBonusListGenerator.is_extract_set_bonus(set_spell_data.id_item_set)[0]:
                continue

            self.process_spell(set_spell_data.id_spell, ids, 0, 0)

        # Glyph effects, need to do trickery here to get actual effect from spellbook data
        for ability_id, ability_data in self._skilllineability_db.items():
            if ability_data.id_skill != 810 or not ability_data.mask_class:
                continue

            use_glyph_spell = self._spell_db[ability_data.id_spell]
            if not use_glyph_spell.id:
                continue

            # Find the on-use for glyph then, misc value will contain the correct GlyphProperties.dbc id
            for effect in use_glyph_spell._effects:
                if not effect or effect.type != 74: # Use glyph
                    continue

                # Filter some erroneous glyph data out
                glyph_data = self._glyphproperties_db[effect.misc_value]
                spell_id = glyph_data.id_spell
                if not glyph_data.id or not spell_id:
                    continue

                self.process_spell(spell_id, ids, ability_data.mask_class, 0)

        # Item enchantments that use a spell
        for eid, data in self._spellitemenchantment_db.items():
            for attr_id in range(1, 4):
                attr_type = getattr(data, 'type_%d' % attr_id)
                if attr_type == 1 or attr_type == 3 or attr_type == 7:
                    sid = getattr(data, 'id_property_%d' % attr_id)
                    self.process_spell(sid, ids, 0, 0)

        # Items with a spell identifier as "stats"
        for iid, data in self._item_sparse_db.items():
            # Allow neck, finger, trinkets, weapons, 2hweapons to bypass ilevel checking
            ilevel = data.ilevel
            if data.inv_type not in [ 2, 11, 12, 13, 15, 17, 21, 22, 26 ] and \
               (ilevel < self._options.min_ilevel or ilevel > self._options.max_ilevel):
                continue

            for spell in data.get_links('spells'):
                spell_id = spell.id_spell
                if spell_id == 0:
                    continue

                self.process_spell(spell_id, ids, 0, 0)

        # Artifact spells
        for _, data in self._artifactpowerrank_db.items():
            spell_id = data.id_spell
            spell = self._spell_db[spell_id]
            if spell.id != spell_id:
                continue

            self.process_spell(spell_id, ids, 0, 0)

        # Last, get the explicitly defined spells in _spell_id_list on a class basis and the
        # generic spells from SpellDataGenerator._spell_id_list[0]
        for generic_spell_id in SpellDataGenerator._spell_id_list[0]:
            if generic_spell_id in ids.keys():
                logging.debug('Whitelisted spell id %u (%s) already in the list of spells to be extracted',
                    generic_spell_id, self._spell_db[generic_spell_id].name)
            self.process_spell(generic_spell_id, ids, 0, 0)

        for cls in range(1, len(SpellDataGenerator._spell_id_list)):
            for spell_tuple in SpellDataGenerator._spell_id_list[cls]:
                if len(spell_tuple) == 2 and spell_tuple[0] in ids.keys():
                    logging.debug('Whitelisted spell id %u (%s) already in the list of spells to be extracted',
                        spell_tuple[0], self._spell_db[spell_tuple[0]].name)
                self.process_spell(spell_tuple[0], ids, self._class_masks[cls], 0)

        for spell_id, spell_data in self._spell_db.items():
            for pattern in SpellDataGenerator._spell_name_whitelist:
                if pattern.match(spell_data.name):
                    self.process_spell(spell_id, ids, 0, 0)


        # After normal spells have been fetched, go through all spell ids,
        # and get all the relevant aura_ids for selected spells
        more_ids = { }
        for spell_id, spell_data in ids.items():
            spell = self._spell_db[spell_id]
            for power in spell.get_links('power'):
                aura_id = power.aura_id
                if aura_id == 0:
                    continue

                self.process_spell(aura_id, more_ids, spell_data['mask_class'], spell_data['mask_race'])

        for id, data in more_ids.items():
            if id  not  in ids:
                ids[ id ] = data
            else:
                ids[id]['mask_class'] |= data['mask_class']
                ids[id]['mask_race'] |= data['mask_race']

        #print('filter done', datetime.datetime.now() - _start)
        return ids

    def generate(self, ids = None):
        _start = datetime.datetime.now()
        # Sort keys
        id_keys = sorted(ids.keys())
        effects = set()
        powers = set()

        # Hotfix data for spells, effects, powers
        spell_hotfix_data = {}
        effect_hotfix_data = {}
        power_hotfix_data = {}

        self._out.write('#define %sSPELL%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(ids)
        ))
        self._out.write('// %d spells, wow build level %d\n' % ( len(ids), self._options.build ))
        self._out.write('static struct spell_data_t __%sspell%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or ''
        ))

        index = 0
        for id in id_keys + [0]:
            spell = self._spell_db[id]
            hotfix_flags = 0
            hotfix_data = []

            if not spell.id and id > 0:
                sys.stderr.write('Spell id %d not found\n') % id
                continue

            for power in spell.get_links('power'):
                powers.add( power )
                if power.is_hotfixed():
                    hotfix_flags |= SPELL_POWER_HOTFIX_PRESENT

            misc = self._spellmisc_db[spell.id_misc]

            if index % 20 == 0:
              self._out.write('//{ Name                                ,     Id,             Hotfix,PrjSp,  Sch, Class,  Race,Sca,MSL,SpLv,MxL,MinRange,MaxRange,Cooldown,  GCD,Chg, ChrgCd, Cat,  Duration,  RCost, RPG,Stac, PCh,PCr, ProcFlags,EqpCl, EqpInvType,EqpSubclass,CastMn,CastMx,Div,       Scaling,SLv, RplcId, {      Attr1,      Attr2,      Attr3,      Attr4,      Attr5,      Attr6,      Attr7,      Attr8,      Attr9,     Attr10,     Attr11,     Attr12 }, {     Flags1,     Flags2,     Flags3,     Flags4 }, Family, Description, Tooltip, Description Variable, Icon, ActiveIcon, Effect1, Effect2, Effect3 },\n')

            # 1, 2
            fields = spell.field('name', 'id')
            f, hfd = spell.get_hotfix_info(('name', 0))
            hotfix_flags |= f
            hotfix_data += hfd

            assert len(fields) == 2
            # 3
            fields += [ u'%#.16x' % 0 ]
            assert len(fields) == 3
            # 4, 5
            fields += misc.field('proj_speed', 'school')
            f, hfd = misc.get_hotfix_info(('proj_speed', 3), ('school', 4))
            hotfix_flags |= f
            hotfix_data += hfd
            assert len(fields) == 5

            # Hack in the combined class from the id_tuples dict
            # 6, 7
            fields += [ u'%#.3x' % ids.get(id, { 'mask_class' : 0, 'mask_race': 0 })['mask_class'] ]
            fields += [ u'%#.3x' % ids.get(id, { 'mask_class' : 0, 'mask_race': 0 })['mask_race'] ]
            assert len(fields) == 7

            # Set the scaling index for the spell
            # 8, 9
            fields += spell.get_link('scaling').field('id_class', 'max_scaling_level')
            f, hfd = spell.get_link('scaling').get_hotfix_info(('id_class', 7), ('max_scaling_level', 8))
            hotfix_flags |= f
            hotfix_data += hfd
            assert len(fields) == 9

            #fields += spell.field('extra_coeff')

            # 10, 11
            fields += spell.get_link('level').field('base_level', 'max_level')
            f, hfd = spell.get_link('level').get_hotfix_info(('base_level', 9), ('max_level', 10))
            hotfix_flags |= f
            hotfix_data += hfd
            assert len(fields) == 11
            # 12, 13
            range_entry = self._spellrange_db[misc.id_range]
            fields += range_entry.field('min_range', 'max_range')
            f, hfd = range_entry.get_hotfix_info(('min_range', 11), ('max_range', 12))
            hotfix_flags |= f
            hotfix_data += hfd
            assert len(fields) == 13
            # 14, 15, 16
            fields += spell.get_link('cooldown').field('cooldown_duration', 'gcd_cooldown', 'category_cooldown')
            f, hfd = spell.get_link('cooldown').get_hotfix_info(('cooldown_duration', 13), ('gcd_cooldown', 14), ('category_cooldown', 15))
            hotfix_flags |= f
            hotfix_data += hfd
            assert len(fields) == 16
            # 17, 18
            category = spell.get_link('categories')
            category_data = self._spellcategory_db[category.charge_category]

            fields += category_data.field('charges', 'charge_cooldown')
            f, hfd = category_data.get_hotfix_info(('charges', 16), ('charge_cooldown', 17))
            hotfix_flags |= f
            hotfix_data += hfd
            # 19
            if category.charge_category > 0: # Note, some spells have both cooldown and charge categories
                fields += category.field('charge_category')
                f, hfd = category.get_hotfix_info(('charge_category', 18))
                hotfix_flags |= f
                hotfix_data += hfd
            else:
                fields += category.field('cooldown_category')
                f, hfd = category.get_hotfix_info(('cooldown_category', 18))
                hotfix_flags |= f
                hotfix_data += hfd
            assert len(fields) == 19

            # 20
            duration_entry = self._spellduration_db[misc.id_duration]
            fields += duration_entry.field('duration_1')
            f, hfd = duration_entry.get_hotfix_info(('duration_1', 19))
            hotfix_flags |= f
            hotfix_data += hfd
            assert len(fields) == 20
            # 21, 22, 23, 24, 25
            fields += spell.get_link('aura_option').field('stack_amount', 'proc_chance', 'proc_charges', 'proc_flags', 'internal_cooldown')
            f, hfd = spell.get_link('aura_option').get_hotfix_info(
                    ('stack_amount', 20), ('proc_chance', 21), ('proc_charges', 22),
                    ('proc_flags', 23), ('internal_cooldown', 24))
            hotfix_flags |= f
            hotfix_data += hfd
            assert len(fields) == 25
            # 26
            ppm_entry = self._spellprocsperminute_db[spell.get_link('aura_option').id_ppm]
            fields += ppm_entry.field('ppm')
            f, hfd = ppm_entry.get_hotfix_info(('ppm', 25))
            hotfix_flags |= f
            hotfix_data += hfd
            assert len(fields) == 26

            # 27, 28, 29
            fields += spell.get_link('equipped_item').field('item_class', 'mask_inv_type', 'mask_sub_class')
            f, hfd = spell.get_link('equipped_item').get_hotfix_info(
                ('item_class', 26), ('mask_inv_type', 27), ('mask_sub_class', 28))
            hotfix_flags |= f
            hotfix_data += hfd

            cast_times = self._spellcasttimes_db[misc.id_cast_time]
            # 30, 31
            fields += cast_times.field('min_cast_time', 'cast_time')
            f, hfd = cast_times.get_hotfix_info(('min_cast_time', 29), ('cast_time', 30))
            hotfix_flags |= f
            hotfix_data += hfd
            # 32, 33, 34
            fields += [u'0', u'0', u'0'] # cast_div, c_scaling, c_scaling_threshold

            # 35
            if id in ids and 'replace_spell_id' in ids[id]:
                fields += [ '%6u' % ids[id]['replace_spell_id'] ]
            else:
                fields += [ '%6u' % 0 ]

            s_effect = []
            effect_ids = []
            # Check if effects have hotfixed data, later on we use a special
            # bitflag (0x8000 0000 0000 0000) to mark effect hotfix presence for spells.
            for effect in spell._effects:
                if effect and ids.get(id, { 'effect_list': [ False ] })['effect_list'][effect.index]:
                    effects.add( ( effect.id, spell.get_link('scaling').id ) )
                    effect_ids.append( '%u' % effect.id )
                    if effect.is_hotfixed():
                        hotfix_flags |= SPELL_EFFECT_HOTFIX_PRESENT

            # Add spell flags
            # 36
            fields += [ '{ %s }' % ', '.join(misc.field('flags_1', 'flags_2', 'flags_3', 'flags_4',
                'flags_5', 'flags_6', 'flags_7', 'flags_8', 'flags_9', 'flags_10', 'flags_11',
                'flags_12')) ]
            # Note, bunch up the flags checking into one field,
            f, hfd = misc.get_hotfix_info(('flags_1', 35), ('flags_2', 35), ('flags_3', 35),
                ('flags_4', 35), ('flags_5', 35), ('flags_6', 35), ('flags_7', 35), ('flags_8', 35),
                ('flags_9', 35), ('flags_10', 35), ('flags_11', 35), ('flags_12', 35))
            hotfix_flags |= f
            #hotfix_data += hfd
            # 37, 38
            fields += [ '{ %s }' % ', '.join(spell.get_link('class_option').field('flags_1', 'flags_2', 'flags_3', 'flags_4')) ]
            fields += spell.get_link('class_option').field('family')
            f, hfd = spell.get_link('class_option').get_hotfix_info(('flags_1', 36), ('flags_2', 36),
                ('flags_3', 36), ('flags_4', 36))
            hotfix_flags |= f
            f, hfd = spell.get_link('class_option').get_hotfix_info(('family', 37))
            hotfix_flags |= f
            hotfix_data += hfd
            # 39
            fields += spell.get_link('shapeshift').field('flags')
            f, hfd= spell.get_link('shapeshift').get_hotfix_info(('flags', 38))
            hotfix_flags |= f
            hotfix_data += hfd
            # 40
            mechanic = self._spellmechanic_db[spell.get_link('categories').mechanic]
            fields += mechanic.field('mechanic')
            f, hfd = mechanic.get_hotfix_info(('mechanic', 39))
            hotfix_flags |= f
            hotfix_data += hfd

            # Note, no hotfix data for this for now. Also, only apply power id
            # to the first rank, and only to spec specific artifacts. Fishing
            # be gone!
            # 41
            power_rank = spell.get_link('artifact_power')
            power = self._artifactpower_db[power_rank.id_power]
            artifact = self._artifact_db[power.id_artifact]
            if power_rank.index == 0 and artifact.id_spec != 0:
                fields += spell.get_link('artifact_power').field('id_power')
            else:
                fields += self._artifactpowerrank_db[0].field('id_power')

            # 42, 43
            fields += spell.field('desc', 'tt')
            f, hfd = spell.get_hotfix_info(('desc', 41), ('tt', 42))
            hotfix_flags |= f
            hotfix_data += hfd
            # 43
            desc_var = self._spelldescriptionvariables_db.get(spell.id_desc_var)
            if desc_var:
                fields += desc_var.field('desc')
                f, hfd = desc_var.get_hotfix_info(('desc', 43))
                hotfix_flags |= f
                hotfix_data += hfd
            else:
                f, hfd = spell.get_hotfix_info(('id_desc_var', 43))
                hotfix_flags |= f
                hotfix_data += hfd
                fields += [ u'0' ]
            # 44
            fields += spell.field('rank')
            f, hfd = spell.get_hotfix_info(('rank', 44))
            hotfix_flags |= f
            hotfix_data += hfd
            # Pad struct with empty pointers for direct access to spell effect data
            # 44, 45, 46, 47
            fields += [ u'0', u'0', u'0', u'0' ]

            # Finally, update hotfix flags, they are located in the array of fields at position 2
            if spell._flags == -1:
                hotfix_flags = SPELL_HOTFIX_MAP_NEW
            fields[2] = '%#.16x' % hotfix_flags
            # And finally, collect spell hotfix data if it exists
            if len(hotfix_data) > 0:
                logging.debug('Hotfixed spell %s, original values: %s', spell.name, hotfix_data)
                spell_hotfix_data[spell.id] = hotfix_data

            try:
                self._out.write('  { %s }, /* %s */\n' % (', '.join(fields), ', '.join(effect_ids)))
            except Exception as e:
                sys.stderr.write('Error: %s\n%s\n' % (e, fields))
                sys.exit(1)

            index += 1

        self._out.write('};\n\n')

        self._out.write('#define __%sSPELLEFFECT%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(effects)))
        self._out.write('// %d effects, wow build level %d\n' % ( len(effects), self._options.build ))
        self._out.write('static struct spelleffect_data_t __%sspelleffect%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or ''))

        index = 0
        for effect_data in sorted(effects) + [ ( 0, 0 ) ]:
            effect = self._spelleffect_db[effect_data[0]]
            if not effect.id and effect_data[ 0 ] > 0:
                sys.stderr.write('Spell Effect id %d not found\n') % effect_data[0]
                continue

            if index % 20 == 0:
                self._out.write('//{     Id,Flags,   SpId,Idx, EffectType                  , EffectSubType                              ,       Average,         Delta,       Unknown,   Coefficient, APCoefficient,  Ampl,  Radius,  RadMax,   BaseV,   MiscV,  MiscV2, {     Flags1,     Flags2,     Flags3,     Flags4 }, Trigg,   DmgMul,  CboP, RealP,Die,Mech,ChTrg,0, 0 },\n')

            hotfix_flags = 0
            hotfix_data = []

            # 1
            fields = effect.field('id')
            # 2
            fields += [ '%#.8x' % 0 ]
            # 3, 4
            fields += effect.field('id_spell', 'index')
            f, hfd = effect.get_hotfix_info(('id_spell', 2), ('index', 3))
            hotfix_flags |= f
            hotfix_data += hfd

            tmp_fields = []
            if dbc.constants.effect_type.get(effect.type):
                tmp_fields += [ '%-*s' % ( dbc.constants.effect_type_maxlen, dbc.constants.effect_type.get(effect.type) ) ]
            else:
                #print "Type %d missing" % effect.type
                tmp_fields += [ '%-*s' % ( dbc.constants.effect_type_maxlen, 'E_%d' % effect.type ) ]

            if dbc.constants.effect_subtype.get(effect.sub_type):
                tmp_fields += [ '%-*s' % ( dbc.constants.effect_subtype_maxlen, dbc.constants.effect_subtype.get(effect.sub_type) ) ]
            else:
                #stm.add(effect.sub_type)
                tmp_fields += [ '%-*s' % ( dbc.constants.effect_subtype_maxlen, 'A_%d' % effect.sub_type ) ]

            # 5, 6
            fields += tmp_fields
            f, hfd = effect.get_hotfix_info(('type', 4), ('sub_type', 5))
            hotfix_flags |= f
            hotfix_data += hfd
            # 7, 8, 9
            fields += effect.get_link('scaling').field('average', 'delta', 'bonus')
            f, hfd = effect.get_link('scaling').get_hotfix_info(('average', 6), ('delta', 7), ('bonus', 8))
            hotfix_flags |= f
            hotfix_data += hfd
            # 10, 11, 12
            fields += effect.field('sp_coefficient', 'ap_coefficient', 'amplitude')
            f, hfd = effect.get_hotfix_info(('sp_coefficient', 9), ('ap_coefficient', 10), ('amplitude', 11))
            hotfix_flags |= f
            hotfix_data += hfd

            # 13
            radius_entry = self._spellradius_db[effect.id_radius]
            fields += radius_entry.field('radius_1')
            f, hfd = radius_entry.get_hotfix_info(('radius_1', 12))
            hotfix_flags |= f
            hotfix_data += hfd

            # 14
            radius_max_entry = self._spellradius_db[effect.id_radius_max]
            fields += radius_max_entry.field('radius_1')
            f, hfd = radius_max_entry.get_hotfix_info(('radius_1', 13))
            hotfix_flags |= f
            hotfix_data += hfd

            # 15, 16, 17
            fields += effect.field('base_value', 'misc_value', 'misc_value_2')
            f, hfd = effect.get_hotfix_info(('base_value', 14), ('misc_value', 15), ('misc_value_2', 16))
            hotfix_flags |= f
            hotfix_data += hfd

            # 18, note hotfix flags bunched together into one bit field
            fields += [ '{ %s }' % ', '.join( effect.field('class_mask_1', 'class_mask_2', 'class_mask_3', 'class_mask_4' ) ) ]
            f, hfd = effect.get_hotfix_info(('class_mask_1', 17), ('class_mask_2', 17), ('class_mask_3', 17), ('class_mask_4', 17))
            hotfix_flags |= f
            #hotfix_data += hfd

            # 19, 20, 21, 22, 23
            fields += effect.field('trigger_spell', 'dmg_multiplier', 'points_per_combo_points', 'real_ppl', 'die_sides')
            f, hfd = effect.get_hotfix_info(('trigger_spell', 18), ('dmg_multiplier', 19),
                ('points_per_combo_points', 20), ('real_ppl', 21), ('die_sides', 22))
            hotfix_flags |= f
            hotfix_data += hfd

            # 24
            mechanic = self._spellmechanic_db[effect.id_mechanic]
            fields += mechanic.field('mechanic')
            f, hfd = mechanic.get_hotfix_info(('mechanic', 23))
            hotfix_flags |= f
            hotfix_data += hfd

            # 25, 26, 27, 28
            fields += effect.field('chain_target', 'implicit_target_1', 'implicit_target_2', 'val_mul')
            f, hfd = effect.get_hotfix_info(('chain_target', 24), ('implicit_target_1', 25),
                ('implicit_target_2', 26), ('val_mul', 27))
            hotfix_flags |= f
            hotfix_data += hfd

            # Pad struct with empty pointers for direct spell data access
            fields += [ u'0', u'0', u'0' ]

            # Update hotfix flags, they are located at position 1. If the
            # effect's hotfix_flags is -1, it's a new entry
            if effect._flags == -1:
                hotfix_flags = EFFECT_HOTFIX_MAP_NEW
            fields[1] = '%#.8x' % hotfix_flags
            # And finally, collect effect hotfix data if it exists
            if len(hotfix_data) > 0:
                effect_hotfix_data[effect.id] = hotfix_data
                logging.debug('Hotfixed effect %d, original values: %s', effect.id, hotfix_data)

            try:
                self._out.write('  { %s },\n' % (', '.join(fields)))
            except:
                sys.stderr.write('%s\n' % fields)
                sys.exit(1)

            index += 1

        self._out.write('};\n\n')

        index = 0
        def sortf( a, b ):
            if a.id > b.id:
                return 1
            elif a.id < b.id:
                return -1

            return 0

        powers = list(powers)
        powers.sort(key = lambda k: k.id)

        self._out.write('#define __%s_SIZE (%d)\n\n' % ( self.format_str( "spellpower" ).upper(), len(powers) ))
        self._out.write('// %d effects, wow build level %d\n' % ( len(powers), self._options.build ))
        self._out.write('static struct spellpower_data_t __%s_data[] = {\n' % ( self.format_str( "spellpower" ) ))

        for power in powers + [ self._spellpower_db[0] ]:
            hotfix_flags = 0
            hotfix_data = []
            # 1 2 3
            fields = power.field('id', 'id_spell', 'aura_id')
            f, hfd = power.get_hotfix_info(('id_spell', 1), ('aura_id', 2))
            hotfix_flags |= f
            hotfix_data += hfd
            # 4
            fields += [ '%#.8x' % power._flags ]
            # 5 6 7 8 9 10 11
            fields += power.field('type_power', 'cost', 'cost_max', 'cost_per_second', 'pct_cost', 'pct_cost_max', 'pct_cost_per_second' )
            f, hfd = power.get_hotfix_info(('type_power', 4), ('cost', 5), ('cost_max', 6),
                ('cost_per_second', 7), ('pct_cost', 8), ('pct_cost_max', 9), ('pct_cost_per_second', 10))
            hotfix_flags |= f
            hotfix_data += hfd

            # Append a field to include hotfix entry pointer
            fields += [ u'0' ]

            # And update hotfix flags at position 3
            if power._flags == -1:
                hotfix_flags = POWER_HOTFIX_MAP_NEW
            fields[3] = '%#.8x' % hotfix_flags
            # And finally, collect powet hotfix data if it exists
            if len(hotfix_data) > 0:
                logging.debug('Hotfixed power %d, original values: %s', power.id, hotfix_data)
                power_hotfix_data[power.id] = hotfix_data

            try:
                self._out.write('  { %s },\n' % (', '.join(fields)))
            except:
                sys.stderr.write('%s\n' % fields)
                sys.exit(1)

        self._out.write('};\n\n')

        # Then, write out hotfix data
        output_data = [('spell', spell_hotfix_data), ('effect', effect_hotfix_data), ('power', power_hotfix_data)]
        for type_str, hotfix_data in output_data:
            output_hotfixes(self, type_str, hotfix_data)

        return ''

class MasteryAbilityGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        super().__init__(options, data_store)

        self._dbc = [ 'Spell', 'ChrSpecialization', 'SpellMisc' ]

    def filter(self):
        ids = {}
        for k, v in self._chrspecialization_db.items():
            if v.class_id == 0:
                continue

            s = self._spell_db[v.id_mastery]
            if s.id == 0:
                continue

            ids[v.id_mastery] = { 'mask_class' : v.class_id, 'category' : v.index, 'spec_name' : v.name }

        return ids

    def generate(self, ids = None):
        max_ids = 0
        mastery_class = 0
        keys    = [
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
        ]

        for k, v in ids.items():
            keys[v['mask_class']][v['category']].append( ( self._spell_db[k].name, k, v['spec_name'] ) )


        # Find out the maximum size of a key array
        for cls in keys:
            for spec in cls:
                if len(spec) > max_ids:
                    max_ids = len(spec)

        data_str = "%sclass_mastery_ability%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        self._out.write('#define %s_SIZE (%d)\n\n' % (data_str.upper(), max_ids))
        self._out.write('// Class mastery abilities, wow build %d\n' % self._options.build)
        self._out.write('static unsigned __%s_data[MAX_CLASS][MAX_SPECS_PER_CLASS][%s_SIZE] = {\n' % (
            data_str,
            data_str.upper(),
        ))

        for cls in range(0, len(keys)):
            if SpellDataGenerator._class_names[cls]:
                self._out.write('  // Class mastery abilities for %s\n' % ( SpellDataGenerator._class_names[cls] ))

            self._out.write('  {\n')
            for spec in range(0, len(keys[cls])):
                if len(keys[cls][spec]) > 0:
                    self._out.write('    // Masteries for %s specialization\n' % keys[cls][spec][0][2])
                self._out.write('    {\n')
                for ability in sorted(keys[cls][spec], key = lambda i: i[0]):
                    self._out.write('     %6u, // %s\n' % ( ability[1], ability[0] ))

                if len(keys[cls][spec]) < max_ids:
                    self._out.write('     %6u,\n' % 0)

                self._out.write('    },\n')
            self._out.write('  },\n')

        self._out.write('};\n')

class RacialSpellGenerator(SpellDataGenerator):
    def __init__(self, options, data_store = None):
        super().__init__(options, data_store)

        SpellDataGenerator._class_categories = []

    def filter(self):
        ids = { }

        for ability_id, ability_data in self._skilllineability_db.items():
            racial_spell = 0

            # Take only racial spells to this
            for j in range(0, len(SpellDataGenerator._race_categories)):
                if ability_data.id_skill in SpellDataGenerator._race_categories[j]:
                    racial_spell = j
                    break

            if not racial_spell:
                continue

            spell = self._spell_db[ability_data.id_spell]
            if not self.spell_state(spell):
                continue

            if ids.get(ability_data.id_spell):
                ids[ability_data.id_spell]['mask_class'] |= ability_data.mask_class
                ids[ability_data.id_spell]['mask_race'] |= (ability_data.mask_race or (1 << (racial_spell - 1)))
            else:
                ids[ability_data.id_spell] = { 'mask_class': ability_data.mask_class, 'mask_race' : ability_data.mask_race or (1 << (racial_spell - 1)) }

        return ids

    def generate(self, ids = None):
        keys = [ ]
        max_ids = 0

        for i in range(0, len(DataGenerator._race_names)):
            keys.insert(i, [])
            for j in range(0, len(DataGenerator._class_names)):
                keys[i].insert(j, [])

        for k, v in ids.items():
            # Add this for all races and classes that have a mask in v['mask_race']
            for race_bit in range(0, len(DataGenerator._race_names)):
                if not DataGenerator._race_names[race_bit]:
                    continue

                spell = self._spell_db[k]
                if spell.id != k:
                    continue

                if v['mask_race'] & (1 << (race_bit - 1)):
                    if v['mask_class']:
                        for class_bit in range(0, len(DataGenerator._class_names)):
                            if not DataGenerator._class_names[class_bit]:
                                continue

                            if v['mask_class'] & (1 << (class_bit - 1)):
                                keys[race_bit][class_bit].append( ( spell.name, k ) )
                    # Generic racial spell, goes to "class 0"
                    else:
                        keys[race_bit][0].append( ( spell.name, k ) )

        # Figure out tree with most abilities
        for race in range(0, len(keys)):
            for cls in range(0, len(keys[race])):
                if len(keys[race][cls]) > max_ids:
                    max_ids = len(keys[race][cls])

        data_str = "%srace_ability%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        # Then, output the stuffs
        self._out.write('#define %s_SIZE (%d)\n\n' % (
            data_str.upper(),
            max_ids
        ))

        self._out.write("#ifndef %s\n#define %s (%d)\n#endif\n\n" % (
            self.format_str( 'MAX_RACE' ),
            self.format_str( 'MAX_RACE' ),
            len(DataGenerator._race_names) ))
        self._out.write('// Racial abilities, wow build %d\n' % self._options.build)
        self._out.write('static unsigned __%s_data[%s][%s][%s_SIZE] = {\n' % (
            self.format_str( 'race_ability' ),
            self.format_str( 'MAX_RACE' ),
            self.format_str( 'MAX_CLASS' ),
            data_str.upper()
        ))

        for race in range(0, len(keys)):
            if DataGenerator._race_names[race]:
                self._out.write('  // Racial abilities for %s\n' % DataGenerator._race_names[race])

            self._out.write('  {\n')

            for cls in range(0, len(keys[race])):
                if len(keys[race][cls]) > 0:
                    if cls == 0:
                        self._out.write('    // Generic racial abilities\n')
                    else:
                        self._out.write('    // Racial abilities for %s class\n' % DataGenerator._class_names[cls])
                    self._out.write('    {\n')
                else:
                    self._out.write('    { %5d, },\n' % 0)
                    continue

                for ability in sorted(keys[race][cls], key = lambda i: i[0]):
                    self._out.write('      %5d, // %s\n' % ( ability[1], ability[0] ))

                if len(keys[race][cls]) < max_ids:
                    self._out.write('      %5d,\n' % 0)

                self._out.write('    },\n')
            self._out.write('  },\n')
        self._out.write('};\n')

class SpecializationSpellGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'Spell', 'SpecializationSpells', 'ChrSpecialization' ]

        super().__init__(options, data_store)

    def generate(self, ids = None):
        max_ids = 0
        keys = [
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
            [ [], [], [], [] ],
        ]
        spells = []

        for ssid, data in self._specializationspells_db.items():
            chrspec = self._chrspecialization_db[data.spec_id]
            if chrspec.id == 0:
                continue

            spell = self._spell_db[data.spell_id]
            if spell.id == 0:
                continue

            spec_tuple = (chrspec.class_id, chrspec.index, spell.id)
            if spec_tuple in spells:
                continue

            keys[chrspec.class_id][chrspec.index].append( ( spell, chrspec.name, data.unk_2 ) )
            spells.append(spec_tuple)

        # Figure out tree with most abilities
        for cls in range(0, len(keys)):
            for tree in range(0, len(keys[cls])):
                if len(keys[cls][tree]) > max_ids:
                    max_ids = len(keys[cls][tree])

        data_str = "%stree_specialization%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        self._out.write('#define %s_SIZE (%d)\n\n' % (data_str.upper(), max_ids))

        self._out.write('// Talent tree specialization abilities, wow build %d\n' % self._options.build)
        self._out.write('static unsigned __%s_data[][MAX_SPECS_PER_CLASS][%s_SIZE] = {\n' % (
            data_str,
            data_str.upper(),
        ))

        for cls in range(0, len(keys)):
            self._out.write('  // Specialization abilities for %s\n' % (cls > 0 and DataGenerator._class_names[cls] or 'Hunter pets'))
            self._out.write('  {\n')

            for tree in range(0, len(keys[cls])):
                if len(keys[cls][tree]) > 0:
                    self._out.write('    // Specialization abilities for %s\n' % keys[cls][tree][0][1])
                self._out.write('    {\n')
                for ability in sorted(keys[cls][tree], key = lambda i: i[2]):
                    self._out.write('      %6u, // %s%s\n'% ( ability[0].id, ability[0].name, ability[0].rank and (' (%s)' % ability[0].rank) or ''))

                if len(keys[cls][tree]) < max_ids:
                    self._out.write('      %6u,\n' % 0)

                self._out.write('    },\n')
            self._out.write('  },\n')
        self._out.write('};\n')

class SpellListGenerator(SpellDataGenerator):
    def spell_state(self, spell, enabled_effects = None):
        if not SpellDataGenerator.spell_state(self, spell, None):
            return False

        misc = self._spellmisc_db[spell.id_misc]
        # Skip passive spells
        if misc.flags_1 & 0x40:
            logging.debug("Spell id %u (%s) marked as passive", spell.id, spell.name)
            return False

        if misc.flags_1 & 0x80:
            logging.debug("Spell id %u (%s) marked as hidden", spell.id, spell.name)
            return False

        # Skip by possible indicator for spellbook visibility
        if misc.flags_5 & 0x8000:
            logging.debug("Spell id %u (%s) marked as hidden in spellbook", spell.id, spell.name)
            return False

        # Skip spells without any resource cost and category
        found_power = False
        for power in spell.get_links('power'):
            if not power:
                continue

            if power.cost > 0 or power.cost_max > 0 or power.cost_per_second > 0 or \
               power.pct_cost > 0 or power.pct_cost_max > 0 or power.pct_cost_per_second > 0:
                found_power = True
                break

        # Filter out any "Rank x" string, as there should no longer be such things. This should filter out
        # some silly left over? things, or things not shown to player anyhow, so should be all good.
        if type(spell.rank) == str:
            if 'Rank ' in spell.rank:
                logging.debug("Spell id %u (%s) has a rank defined", spell.id, spell.name)
                return False

            if 'Passive' in spell.rank:
                logging.debug("Spell id %u (%s) labeled passive", spell.id, spell.name)
                return False

        # Let's not accept spells that have over 100y range, as they cannot really be base abilities then
        if misc.id_range > 0:
            range_ = self._spellrange_db[misc.id_range]
            if range_.max_range > 100.0 or range_.max_range_2 > 100.0:
                logging.debug("Spell id %u (%s) has a high range (%f, %f)", spell.id, spell.name, range_.max_range, range_.max_range_2)
                return False

        # And finally, spells that are forcibly activated/disabled in whitelisting for
        for cls_ in range(1, len(SpellDataGenerator._spell_id_list)):
            for spell_tuple in SpellDataGenerator._spell_id_list[cls_]:
                if  spell_tuple[0] == spell.id and len(spell_tuple) == 2 and spell_tuple[1] == 0:
                    return False
                elif spell_tuple[0] == spell.id and len(spell_tuple) == 3:
                    return spell_tuple[2]

        return True

    def filter(self):
        triggered_spell_ids = []
        ids = { }
        spell_tree = -1
        spell_tree_name = ''
        for ability_id, ability_data in self._skilllineability_db.items():
            if ability_data.id_skill in SpellDataGenerator._skill_category_blacklist:
                continue

            mask_class_skill = self.class_mask_by_skill(ability_data.id_skill)
            mask_class_pet_skill = self.class_mask_by_pet_skill(ability_data.id_skill)
            mask_class = 0

            # Generic Class Ability
            if mask_class_skill > 0:
                spell_tree_name = "General"
                spell_tree = 0
                mask_class = mask_class_skill
            elif mask_class_pet_skill > 0:
                spell_tree_name = "Pet"
                spell_tree = 5
                mask_class = mask_class_pet_skill

            # We only want abilities that belong to a class
            if mask_class == 0:
                continue

            spell = self._spell_db[ability_data.id_spell]
            if not spell.id:
                continue

            # Blacklist all triggered spells for this
            for effect in spell._effects:
                if not effect:
                    continue

                if effect.trigger_spell > 0:
                    triggered_spell_ids.append(effect.trigger_spell)

            # Check generic SpellDataGenerator spell state filtering before anything else
            if not self.spell_state(spell):
                continue

            if ids.get(ability_data.id_spell):
                ids[ability_data.id_spell]['mask_class'] |= ability_data.mask_class or mask_class
            else:
                ids[ability_data.id_spell] = {
                    'mask_class': ability_data.mask_class or mask_class,
                    'tree'      : [ spell_tree ]
                }

        # Specialization spells
        for ss_id, ss_data in self._specializationspells_db.items():
            chrspec = self._chrspecialization_db[ss_data.spec_id]

            if chrspec.class_id == 0:
                continue

            spell = self._spell_db[ss_data.spell_id]
            if not spell.id:
                continue

            # Check generic SpellDataGenerator spell state filtering before anything else
            if not self.spell_state(spell):
                continue

            if ids.get(ss_data.spell_id):
                ids[ss_data.spell_id]['mask_class'] |= DataGenerator._class_masks[chrspec.class_id]
                if chrspec.index + 1 not in ids[ss_data.spell_id]['tree']:
                    ids[ss_data.spell_id]['tree'].append( chrspec.index + 1 )
            else:
                ids[ss_data.spell_id] = {
                    'mask_class': DataGenerator._class_masks[chrspec.class_id],
                    'tree'      : [ chrspec.index + 1 ]
                }

        for cls in range(1, len(SpellDataGenerator._spell_id_list)):
            for spell_tuple in SpellDataGenerator._spell_id_list[cls]:
                # Skip spells with zero tree, as they dont exist
                #if spell_tuple[1] == 0:
                #    continue

                spell = self._spell_db[spell_tuple[0]]
                if not spell.id:
                    continue

                if len(spell_tuple) == 2 and (spell_tuple[1] == 0 or not self.spell_state(spell)):
                    continue
                elif len(spell_tuple) == 3 and spell_tuple[2] == False:
                    continue

                if ids.get(spell_tuple[0]):
                    ids[spell_tuple[0]]['mask_class'] |= self._class_masks[cls]
                    if spell_tuple[1] not in ids[spell_tuple[0]]['tree']:
                        ids[spell_tuple[0]]['tree'].append( spell_tuple[1] )
                else:
                    ids[spell_tuple[0]] = {
                        'mask_class': self._class_masks[cls],
                        'tree'      : [ spell_tuple[1] ],
                    }

        # Finally, go through the spells and remove any triggered spell
        # as the order in dbcs is arbitrary
        final_list = {}
        for id, data in ids.items():
            if id in triggered_spell_ids:
                logging.debug("Spell id %u (%s) is a triggered spell", id, self._spell_db[id].name)
            else:
                final_list[id] = data

        return final_list

    def generate(self, ids = None):
        keys = [
            # General | Spec0 | Spec1 | Spec2 | Spec3 | Pet
            [ [], [], [], [], [], [] ],
            [ [], [], [], [], [], [] ],
            [ [], [], [], [], [], [] ],
            [ [], [], [], [], [], [] ],
            [ [], [], [], [], [], [] ],
            [ [], [], [], [], [], [] ],
            [ [], [], [], [], [], [] ],
            [ [], [], [], [], [], [] ],
            [ [], [], [], [], [], [] ],
            [ [], [], [], [], [], [] ],
            [ [], [], [], [], [], [] ],
            [ [], [], [], [], [], [] ],
            [ [], [], [], [], [], [] ],
        ]

        # Sort a suitable list for us
        for k, v in ids.items():
            if v['mask_class'] not in DataGenerator._class_masks:
                continue

            spell = self._spell_db[k]
            for tree in v['tree']:
                keys[self._class_map[v['mask_class']]][tree].append(( spell.name, spell.id ))

        # Find out the maximum size of a key array
        max_ids = 0
        for cls_list in keys:
            for tree_list in cls_list:
                if len(tree_list) > max_ids:
                    max_ids = len(tree_list)

        data_str = "%sclass_ability%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        self._out.write('#define %s_SIZE (%d)\n' % (
            data_str.upper(),
            max_ids
        ))
        self._out.write('#define %s_TREE_SIZE (%d)\n\n' % ( data_str.upper(), len( keys[0] ) ))
        self._out.write("#ifndef %s\n#define %s (%d)\n#endif\n" % (
            self.format_str( 'MAX_CLASS' ),
            self.format_str( 'MAX_CLASS' ),
            len(DataGenerator._class_names)))
        self._out.write('// Class based active abilities, wow build %d\n' % self._options.build)
        self._out.write('static unsigned __%s_data[][%s_TREE_SIZE][%s_SIZE] = {\n' % (
            data_str,
            data_str.upper(),
            data_str.upper()))

        for i in range(0, len(keys)):
            if SpellDataGenerator._class_names[i]:
                self._out.write('  // Class active abilities for %s\n' % ( SpellDataGenerator._class_names[i] ))

            self._out.write('  {\n')

            for j in range(0, len(keys[i])):
                # See if we can describe the tree
                for t in keys[i][j]:
                    tree_name = ''
                    if j == 0:
                        tree_name = 'General'
                    elif j == 5:
                        tree_name = 'Pet'
                    else:
                        for chrspec_id, chrspec_data in self._chrspecialization_db.items():
                            if chrspec_data.class_id == i and chrspec_data.index == j - 1:
                                tree_name = chrspec_data.name
                                break

                    self._out.write('    // %s tree, %d abilities\n' % ( tree_name, len(keys[i][j]) ))
                    break
                self._out.write('    {\n')
                for spell_id in sorted(keys[i][j], key = lambda k_: k_[0]):
                    r = ''
                    if self._spell_db[spell_id[1]].rank:
                        r = ' (%s)' % self._spell_db[spell_id[1]].rank
                    self._out.write('      %6u, // %s%s\n' % ( spell_id[1], spell_id[0], r ))

                # Append zero if a short struct
                if max_ids - len(keys[i][j]) > 0:
                    self._out.write('      %6u,\n' % 0)

                self._out.write('    },\n')

            self._out.write('  },\n')

        self._out.write('};\n')

class ClassFlagGenerator(SpellDataGenerator):
    _masks = {
            'mage': 3,
            'warrior': 4,
            'warlock': 5,
            'priest': 6,
            'druid': 7,
            'rogue': 8,
            'hunter': 9,
            'paladin': 10,
            'shaman': 11,
            'deathknight': 15
    }
    def __init__(self, options):
        SpellDataGenerator.__init__(self, options)

    def filter(self, class_name):
        ids = { }
        mask = ClassFlagGenerator._masks.get(class_name.lower(), -1)
        if mask == -1:
            return ids

        for id, data in self._spell_db.items():
            if data.id_class_opts == 0:
                continue

            opts = self._spellclassoptions_db[data.id_class_opts]

            if opts.spell_family_name != mask:
                continue

            ids[id] = { }
        return ids

    def generate(self, ids = None):
        spell_data = []
        effect_data = { }
        for i in range(0, 128):
            spell_data.append({ 'spells' : [ ], 'effects': [ ] })

        for spell_id, data in ids.items():
            spell = self._spell_db[spell_id]
            if not spell.id_class_opts:
                continue

            copts = self._spellclassoptions_db[spell.id_class_opts]

            # Assign this spell to bitfield entries
            for i in range(1, 5):
                f = getattr(copts, 'spell_family_flags_%u' % i)

                for bit in range(0, 32):
                    if not (f & (1 << bit)):
                        continue

                    bfield = ((i - 1) * 32) + bit
                    spell_data[bfield]['spells'].append( spell )

            # Loop through spell effects, assigning them to effects
            for effect in spell._effects:
                if not effect:
                    continue

                for i in range(1, 5):
                    f = getattr(effect, 'class_mask_%u' % i)
                    for bit in range(0, 32):
                        if not (f & (1 << bit)):
                            continue

                        bfield = ((i - 1) * 32) + bit
                        spell_data[bfield]['effects'].append( effect )

        # Build effect data

        for bit_data in spell_data:
            for effect in bit_data['effects']:
                if effect.id_spell not in effect_data:
                    effect_data[effect.id_spell] = {
                        'effects': { },
                        'spell': self._spell_db[effect.id_spell]
                    }

                if effect.index not in effect_data[effect.id_spell]['effects']:
                    effect_data[effect.id_spell]['effects'][effect.index] = []

                effect_data[effect.id_spell]['effects'][effect.index] += bit_data['spells']

        field = 0
        for bit_field in spell_data:
            field += 1
            if not len(bit_field['spells']):
                continue

            if not len(bit_field['effects']):
                continue

            self._out.write('  [%-3d] ===================================================\n' % field)
            for spell in sorted(bit_field['spells'], key = lambda s: s.name):
                self._out.write('       %s (%u)\n' % ( spell.name, spell.id ))

            for effect in sorted(bit_field['effects'], key = lambda e: e.id_spell):
                rstr = ''
                if self._spell_db[effect.id_spell].rank:
                    rstr = ' (%s)' % self._spell_db[effect.id_spell].rank
                self._out.write('         [%u] {%u} %s%s\n' % ( effect.index, effect.id_spell, self._spell_db[effect.id_spell].name, rstr))

            self._out.write('\n')

        for spell_id in sorted(effect_data.keys()):
            spell = effect_data[spell_id]['spell']
            self._out.write('Spell: %s (%u)' % (spell.name, spell.id))
            if spell.rank:
                self._out.write(' %s' % spell.rank)
            self._out.write('\n')

            effects = effect_data[spell_id]['effects']
            for effect_index in sorted(effects.keys()):
                self._out.write('  Effect#%u:\n' % effect_index)
                for spell in sorted(effects[effect_index], key = lambda s: s.id):
                    self._out.write('    %s (%u)' % (spell.name, spell.id))
                    if spell.rank:
                        self._out.write(' %s' % spell.rank)
                    self._out.write('\n')
                self._out.write('\n')
            self._out.write('\n')

class SetBonusListGenerator(DataGenerator):
    # These set bonuses map directly to set bonuses in ItemSet/ItemSetSpell
    # (the bonuses array is the id in ItemSet, and
    # ItemSetSpell::id_item_set).
    #
    # NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
    # ====================================================================
    # The ordering of this array _MUST_ match the ordering of
    # "set_bonus_type_e" enumeration in simulationcraft.hpp or very bad
    # things will happen.
    # ====================================================================
    # NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
    set_bonus_map = [
        # Warlords of Draenor PVP set bonuses
        {
            'name'   : 'pvp',
            'bonuses': [ 1230, 1225, 1222, 1227, 1226, 1220, 1228, 1223, 1229, 1224, 1221 ],
            'tier'   : 0,
        },
        # T17 LFR set bonuses
        {
            'name'   : 'tier17lfr',
            'bonuses': [ 1245, 1248, 1246, 1247 ],
            'tier'   : 17,
        },
        # T18 LFR set bonuses
        {
            'name'   : 'tier18lfr',
            'bonuses': [ 1260, 1261, 1262, 1263 ],
            'tier'   : 18,
        },
        # T19 Order Hall set bonuses
        {
            'name'   : 'tier19oh',
            'bonuses': [ 1269, 1270, 1271, 1272, 1273, 1274, 1275, 1276, 1277, 1278, 1279, 1280 ],
            'tier'   : 19
        },
        # Legion Dungeon, March of the Legion
        {
            'name'   : 'march_of_the_legion',
            'bonuses': [ 1293 ],
            'tier'   : 19
        },
        # Legion Dungeon, Journey Through Time
        {
            'name'   : 'journey_through_time',
            'bonuses': [ 1294 ],
            'tier'   : 19
        },
        # Legion Dungeon, Cloth
        {
            'name'   : 'tier19p_cloth',
            'bonuses': [ 1295 ],
            'tier'   : 19
        },
        # Legion Dungeon, Leather
        {
            'name'   : 'tier19p_leather',
            'bonuses': [ 1296 ],
            'tier'   : 19
        },
        # Legion Dungeon, Mail
        {
            'name'   : 'tier19p_mail',
            'bonuses': [ 1297 ],
            'tier'   : 19
        },
        # Legion Dungeon, Plate
        {
            'name'   : 'tier19p_plate',
            'bonuses': [ 1298 ],
            'tier'   : 19
        },
        {
            'name'   : 'tier17',
            'bonuses': [ 1242, 1238, 1236, 1240, 1239, 1234, 1241, 1235, 1243, 1237, 1233 ],
            'tier'   : 17
        },
        {
            'name'   : 'tier18',
            'bonuses': [ 1249, 1250, 1251, 1252, 1253, 1254, 1255, 1256, 1257, 1258, 1259 ],
            'tier'   : 18
        },
        {
            'name'   : 'tier19',
            'bonuses': [ 1281, 1282, 1283, 1284, 1285, 1286, 1287, 1288, 1289, 1290, 1291, 1292 ],
            'tier'   : 19
        },
    ]

    def __init__(self, options, data_store = None):
        self._dbc = [ 'ItemSet', 'ItemSetSpell', 'Spell', 'ChrSpecialization', 'Item-sparse' ]

        super().__init__(options, data_store)

    @staticmethod
    def is_extract_set_bonus(bonus):
        for idx in range(0, len(SetBonusListGenerator.set_bonus_map)):
            if bonus in SetBonusListGenerator.set_bonus_map[idx]['bonuses']:
                return True, idx

        return False, -1

    def filter(self):
        data = []
        for id, set_spell_data in self._itemsetspell_db.items():
            is_set_bonus, set_index = SetBonusListGenerator.is_extract_set_bonus(set_spell_data.id_item_set)
            if not is_set_bonus:
                continue

            item_set = self._itemset_db[set_spell_data.id_item_set]
            if not item_set.id:
                continue

            spell_data = self._spell_db[set_spell_data.id_spell]
            if not spell_data.id:
                continue

            base_entry = {
                'index'       : set_index,
                'set_bonus_id': id,
                'bonus'       : set_spell_data.n_req_items
            }

            spec_ = set_spell_data.id_spec
            class_ = []

            if spec_ > 0:
                spec_data = self._chrspecialization_db[spec_]
                class_.append(spec_data.class_id)
            # No spec id, set spec to "-1" (all specs), and try to use set
            # items to figure out the class (or many classes)
            else:
                spec_ = -1
                # TODO: Fetch from first available item if blizzard for some
                # reason decides to not use _1 as the first one?
                item_data = self._item_sparse_db[item_set.id_item_1]
                for idx in range(0, len(self._class_masks)):
                    mask = self._class_masks[idx]
                    if mask == None:
                        continue

                    if item_data.class_mask & mask:
                        class_.append(idx)

            if len(class_) == 0:
                logging.warn('Could not determine class information for required item set "%s" (id=%d)',
                    item_set.name, item_set.id)
                continue

            for cls in class_:
                new_entry = dict(base_entry)
                new_entry['class'] = cls
                new_entry['spec'] = spec_
                data.append(new_entry)

        return data

    def generate(self, ids = None):
        ids.sort(key = lambda v: (v['index'], v['class'], v['bonus'], v['set_bonus_id']))

        data_str = "%sset_bonus_data%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        self._out.write('#define %s_SIZE (%d)\n\n' % (
            data_str.upper(),
            len(ids)
        ))

        self._out.write('// Set bonus data, wow build %d\n' % self._options.build)
        self._out.write('static item_set_bonus_t __%s[%s_SIZE] = {\n' % (
            data_str,
            data_str.upper(),
        ))

        for data_idx in range(0, len(ids)):
            entry = ids[data_idx]

            if data_idx % 25 == 0:
                self._out.write('  // %-44s,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items\n' % 'Set bonus name')

            item_set_spell = self._itemsetspell_db[entry['set_bonus_id']]
            item_set = self._itemset_db[item_set_spell.id_item_set]
            map_entry = self.set_bonus_map[entry['index']]

            item_set_str = ""
            items = []

            for item_n in range(1, 17):
                item_id = getattr(item_set, 'id_item_%d' % item_n)
                if item_id > 0:
                    items.append('%6u' % item_id)

            if len(items) < 17:
                items.append(' 0')

            self._out.write('  { %-45s, %12s, %6d, %5d, %4u, %3u, %3u, %4u, %6u, %s },\n' % (
                '"%s"' % item_set.name.replace('"', '\\"'),
                '"%s"' % map_entry['name'].replace('"', '\\"'),
                entry['index'],
                item_set_spell.id_item_set,
                map_entry['tier'],
                entry['bonus'],
                entry['class'],
                entry['spec'],
                item_set_spell.id_spell,
                '{ %s }' % (', '.join(items))
            ))

        self._out.write('};\n')

class RandomSuffixGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'ItemRandomSuffix', 'SpellItemEnchantment' ]

        super().__init__(options, data_store)

    def filter(self):
        ids = set()
        # Let's do some modest filtering here, take only "stat" enchants,
        # and take out the test items as well
        for id, data in self._itemrandomsuffix_db.items():
            # of the Test, of the Paladin Testing
            if id == 46 or id == 48:
                continue

            has_non_stat_enchant = False
            # For now, naively presume type_1 of SpellItemEnchantment will tell us
            # if it's a relevant enchantment for us (ie. a stat one )
            for i in range(1,5):
                item_ench = self._spellitemenchantment_db.get( getattr(data, 'id_property_%d' % i) )
                if not item_ench:
                    logging.debug("No item enchantment found for %s", data.name)
                    continue

                if item_ench.type_1 not in [4, 5]:
                    has_non_stat_enchant = True
                    break

            if has_non_stat_enchant:
                continue

            ids.add( id )

        return list(ids)

    def generate(self, ids = None):
        # Sort keys
        ids.sort()

        self._out.write('#define %sRAND_SUFFIX%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(ids)
        ))
        self._out.write('// Random new-style item suffixes, wow build %d\n' % self._options.build)
        self._out.write('static struct random_suffix_data_t __%srand_suffix%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '' ))

        for id in ids + [ 0 ]:
            rs = self._itemrandomsuffix_db[id]

            fields  = rs.field('id', 'name')
            fields += [ '{ %s }' % ', '.join(rs.field('id_property_1', 'id_property_2', 'id_property_3', 'id_property_4', 'id_property_5')) ]
            fields += [ '{ %s }' % ', '.join(rs.field('property_pct_1', 'property_pct_2', 'property_pct_3', 'property_pct_4', 'property_pct_5')) ]
            try:
                self._out.write('  { %s },' % (', '.join(fields)))
            except:
                print(fields)
                sys.exit(1)
            self._out.write('\n')

        self._out.write('};\n')

class SpellItemEnchantmentGenerator(RandomSuffixGenerator):
    def __init__(self, options, data_store = None):
        RandomSuffixGenerator.__init__(self, options, data_store)

        self._dbc += ['Spell', 'SpellEffect', 'Item-sparse', 'GemProperties']

    def filter_linked_spells(self, source_db, data, target_db, target_attr):
        for effect in data._effects:
            if not effect or effect.type != 53:
                continue

            return effect.misc_value

        return 0

    def initialize(self):
        if not RandomSuffixGenerator.initialize(self):
            return False

        link(self._spelleffect_db, 'id_spell', self._spell_db, 'add_effect')
        # Map spell ids to spellitemenchantments, as there's no direct
        # link between them, and 5.4+, we need/want to scale enchants properly
        link(self._spell_db, self.filter_linked_spells, self._spellitemenchantment_db, 'spells')

        # Map items to gem properties
        link(self._item_sparse_db, 'gem_props', self._gemproperties_db, 'item')

        # Map gem properties to enchants
        link(self._gemproperties_db, 'id_enchant', self._spellitemenchantment_db, 'gem_property')

        return True

    def filter(self):
        return self._spellitemenchantment_db.keys()

    def generate(self, ids = None):
        self._out.write('#define %sSPELL_ITEM_ENCH%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(ids)
        ))
        self._out.write('// Item enchantment data, wow build %d\n' % self._options.build)
        self._out.write('static struct item_enchantment_data_t __%sspell_item_ench%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '' ))

        for i in sorted(ids) + [ 0 ]:
            ench_data = self._spellitemenchantment_db[i]

            fields = ench_data.field('id', 'slot')
            fields += ench_data.get_link('gem_property').get_link('item').field('id')
            fields += ench_data.field('scaling_type', 'min_scaling_level', 'max_scaling_level', 'req_skill', 'req_skill_value')
            fields += [ '{ %s }' % ', '.join(ench_data.field('type_1', 'type_2', 'type_3')) ]
            fields += [ '{ %s }' % ', '.join(ench_data.field('amount_1', 'amount_2', 'amount_3')) ]
            fields += [ '{ %s }' % ', '.join(ench_data.field('id_property_1', 'id_property_2', 'id_property_3')) ]
            fields += [ '{ %s }' % ', '.join(ench_data.field('coeff_1', 'coeff_2', 'coeff_3')) ]
            fields += ench_data.get_link('spells').field('id')
            fields += ench_data.field('desc')
            self._out.write('  { %s },\n' % (', '.join(fields)))

        self._out.write('};\n')

class RandomPropertyPointsGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'RandPropPoints' ]

        super().__init__(options, data_store)

    def filter(self):
        ids = [ ]

        for ilevel, data in self._randproppoints_db.items():
            if ilevel >= 1 and ilevel <= self._options.scale_ilevel:
                ids.append(ilevel)

        return ids

    def generate(self, ids = None):
        # Sort keys
        ids.sort()
        self._out.write('#define %sRAND_PROP_POINTS%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(ids)
        ))
        self._out.write('// Random property points for item levels 1-%d, wow build %d\n' % (
            self._options.scale_ilevel, self._options.build ))
        self._out.write('static struct random_prop_data_t __%srand_prop_points%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '' ))

        for id in ids + [ 0 ]:
            rpp = self._randproppoints_db[id]

            fields = rpp.field('id')
            fields += [ '{ %s }' % ', '.join(rpp.field('epic_points_1', 'epic_points_2', 'epic_points_3', 'epic_points_4', 'epic_points_5')) ]
            fields += [ '{ %s }' % ', '.join(rpp.field('rare_points_1', 'rare_points_2', 'rare_points_3', 'rare_points_4', 'rare_points_5')) ]
            fields += [ '{ %s }' % ', '.join(rpp.field('uncm_points_1', 'uncm_points_2', 'uncm_points_3', 'uncm_points_4', 'uncm_points_5')) ]

            self._out.write('  { %s },\n' % (', '.join(fields)))

        self._out.write('};\n')

class WeaponDamageDataGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'ItemDamageOneHand', 'ItemDamageOneHandCaster',
                      'ItemDamageTwoHand', 'ItemDamageTwoHandCaster',  ]

        super().__init__(options, data_store)

    def filter(self):

        return None

    def generate(self, ids = None):
        for dbname in self._dbc:
            db = getattr(self, '_%s_db' % dbname.lower() )

            self._out.write('// Item damage data from %s.dbc, ilevels 1-%d, wow build %d\n' % (
                dbname, self._options.scale_ilevel, self._options.build ))
            self._out.write('static struct item_scale_data_t __%s%s%s_data[] = {\n' % (
                self._options.prefix and ('%s_' % self._options.prefix) or '',
                dbname.lower(),
                self._options.suffix and ('_%s' % self._options.suffix) or '' ))

            for ilevel, data in db.items():
                if ilevel > self._options.scale_ilevel:
                    continue

                fields = data.field('ilevel')
                fields += [ '{ %s }' % ', '.join(data.field('v_1', 'v_2', 'v_3', 'v_4', 'v_5', 'v_6', 'v_7')) ]

                self._out.write('  { %s },\n' % (', '.join(fields)))

            self._out.write('  { %s }\n' % ( ', '.join([ '0' ] + [ '{ 0, 0, 0, 0, 0, 0, 0 }' ]) ))
            self._out.write('};\n\n')

class ArmorValueDataGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'ItemArmorQuality', 'ItemArmorShield', 'ItemArmorTotal' ]
        super().__init__(options, data_store)

    def filter(self):

        return None

    def generate(self, ids = None):
        for dbname in self._dbc:
            db = getattr(self, '_%s_db' % dbname.lower() )

            self._out.write('// Item armor values data from %s.dbc, ilevels 1-%d, wow build %d\n' % (
                dbname, self._options.scale_ilevel, self._options.build ))

            self._out.write('static struct %s __%s%s%s_data[] = {\n' % (
                (dbname != 'ItemArmorTotal') and 'item_scale_data_t' or 'item_armor_type_data_t',
                self._options.prefix and ('%s_' % self._options.prefix) or '',
                dbname.lower(),
                self._options.suffix and ('_%s' % self._options.suffix) or '' ))

            for ilevel, data in db.items():
                if ilevel > self._options.scale_ilevel:
                    continue

                fields = data.field('ilevel')
                if dbname != 'ItemArmorTotal':
                    fields += [ '{ %s }' % ', '.join(data.field('v_1', 'v_2', 'v_3', 'v_4', 'v_5', 'v_6', 'v_7')) ]
                else:
                    fields += [ '{ %s }' % ', '.join(data.field('v_1', 'v_2', 'v_3', 'v_4')) ]
                self._out.write('  { %s },\n' % (', '.join(fields)))

            if dbname != 'ItemArmorTotal':
                self._out.write('  { %s }\n' % ( ', '.join([ '0' ] + [ '{ 0, 0, 0, 0, 0, 0, 0 }' ]) ))
            else:
                self._out.write('  { %s }\n' % ( ', '.join([ '0' ] + [ '{ 0, 0, 0, 0 }' ]) ))
            self._out.write('};\n\n')

class ArmorSlotDataGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'ArmorLocation' ]
        super().__init__(options, data_store)

    def filter(self):
        return None

    def generate(self, ids = None):
        s = '// Inventory type based armor multipliers, wow build %d\n' % ( self._options.build )

        self._out.write('static struct item_armor_type_data_t __%sarmor_slot%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '' ))

        for inv_type in sorted(self._armorlocation_db.keys()):
            data = self._armorlocation_db[inv_type]
            fields = data.field('id')
            fields += [ '{ %s }' % ', '.join(data.field('v_1', 'v_2', 'v_3', 'v_4')) ]
            self._out.write('  { %s },\n' % (', '.join(fields)))

        self._out.write('  { %s }\n' % ( ', '.join([ '0' ] + [ '{ 0, 0, 0, 0 }' ]) ))
        self._out.write('};\n\n')

class GemPropertyDataGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'GemProperties' ]
        super().__init__(options, data_store)

    def filter(self):
        ids = []
        for id, data in self._gemproperties_db.items():
            if not data.color or not data.id_enchant:
                continue

            ids.append(id)
        return ids

    def generate(self, ids = None):
        ids.sort()
        self._out.write('// Gem properties, wow build %d\n' % ( self._options.build ))

        self._out.write('static struct gem_property_data_t __%sgem_property%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '' ))

        for id in ids + [ 0 ]:
            data = self._gemproperties_db[id]

            fields = data.field('id', 'id_enchant', 'color', 'min_ilevel')
            self._out.write('  { %s },\n' % (', '.join(fields)))

        self._out.write('};\n\n')

class ItemBonusDataGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'ItemBonus', 'ItemBonusTreeNode', 'ItemXBonusTree' ]
        super().__init__(options, data_store)

    def generate(self, ids):
        # Bonus trees

        data_str = "%sitem_bonus_tree%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        self._out.write('#define %s_SIZE (%d)\n\n' % (data_str.upper(), len(self._itembonustreenode_db.keys()) + 1))

        self._out.write('// Item bonus trees, wow build %d\n' % ( self._options.build ))

        self._out.write('static struct item_bonus_tree_entry_t __%s_data[%s_SIZE] = {\n' % (data_str, data_str.upper()))

        for key in sorted(self._itembonustreenode_db.keys()) + [0,]:
            data = self._itembonustreenode_db[key]

            fields = data.field('id', 'id_tree', 'index', 'id_child', 'id_node')
            self._out.write('  { %s },\n' % (', '.join(fields)))

        self._out.write('};\n\n')

        # Bonus definitions

        data_str = "%sitem_bonus%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        self._out.write('#define %s_SIZE (%d)\n\n' % (data_str.upper(), len(self._itembonus_db.keys()) + 1))
        self._out.write('// Item bonuses, wow build %d\n' % ( self._options.build ))

        self._out.write('static struct item_bonus_entry_t __%s_data[%s_SIZE] = {\n' % (data_str, data_str.upper()))

        for key in sorted(self._itembonus_db.keys()) + [0,]:
            data = self._itembonus_db[key]
            fields = data.field('id', 'id_node', 'type', 'val_1', 'val_2', 'index')
            self._out.write('  { %s },\n' % (', '.join(fields)))

        self._out.write('};\n\n')

        # Item bonuses (unsure as of yet if we need this, depends on how
        # Blizzard exports the bonus id to third parties)
        data_str = "%sitem_bonus_map%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        self._out.write('#define %s_SIZE (%d)\n\n' % (data_str.upper(), len(self._itemxbonustree_db.keys()) + 1))
        self._out.write('// Item bonus map, wow build %d\n' % ( self._options.build ))

        self._out.write('static struct item_bonus_node_entry_t __%s_data[%s_SIZE] = {\n' % (data_str, data_str.upper()))

        for key in sorted(self._itemxbonustree_db.keys()) + [0,]:
            data = self._itemxbonustree_db[key]
            fields = data.field('id', 'id_item', 'id_tree')
            self._out.write('  { %s },\n' % (', '.join(fields)))

        self._out.write('};\n\n')

def curve_point_sort(a, b):
    if a.id_distribution < b.id_distribution:
        return -1
    elif a.id_distribution > b.id_distribution:
        return 1
    else:
        if a.curve_index < b.curve_index:
            return -1
        elif a.curve_index > b.curve_index:
            return 1
        else:
            return 0

class ScalingStatDataGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'ScalingStatDistribution', 'CurvePoint' ]
        super().__init__(options, data_store)

    def generate(self, ids = None):
        # Bonus trees

        data_str = "%sscaling_stat_distribution%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        self._out.write('#define %s_SIZE (%d)\n\n' % (data_str.upper(), len(self._scalingstatdistribution_db.keys()) + 1))

        self._out.write('// Scaling stat distributions, wow build %d\n' % ( self._options.build ))

        self._out.write('static struct scaling_stat_distribution_t __%s_data[%s_SIZE] = {\n' % (data_str, data_str.upper()))

        for key in sorted(self._scalingstatdistribution_db.keys()) + [0,]:
            data = self._scalingstatdistribution_db[key]

            fields = data.field('id', 'min_level', 'max_level', 'id_curve' )
            self._out.write('  { %s },\n' % (', '.join(fields)))

        self._out.write('};\n\n')

        data_str = "%scurve_point%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        self._out.write('#define %s_SIZE (%d)\n\n' % (data_str.upper(), len(self._curvepoint_db.keys()) + 1))

        self._out.write('// Curve points data, wow build %d\n' % ( self._options.build ))

        self._out.write('static struct curve_point_t __%s_data[%s_SIZE] = {\n' % (data_str, data_str.upper()))

        vals = self._curvepoint_db.values()
        for data in sorted(vals, key = lambda k: (k.id_distribution, k.curve_index)):
            fields = data.field('id_distribution', 'curve_index', 'val_1', 'val_2' )
            self._out.write('  { %s },\n' % (', '.join(fields)))

        self._out.write('  { %s },\n' % (', '.join(self._curvepoint_db[0].field('id_distribution', 'curve_index', 'val_1', 'val_2' ))))

        self._out.write('};\n\n')

class ItemNameDescriptionDataGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'ItemNameDescription' ]
        super().__init__(options, data_store)

    def generate(self, ids = None):
        data_str = "%sitem_name_description%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        self._out.write('#define %s_SIZE (%d)\n\n' % (data_str.upper(), len(self._itemnamedescription_db.keys()) + 1))

        self._out.write('// Item name descriptions, wow build %d\n' % ( self._options.build ))

        self._out.write('static struct item_name_description_t __%s_data[%s_SIZE] = {\n' % (data_str, data_str.upper()))

        for key in sorted(self._itemnamedescription_db.keys()) + [0,]:
            data = self._itemnamedescription_db[key]

            fields = data.field( 'id', 'desc' )
            self._out.write('  { %s },\n' % (', '.join(fields)))

        self._out.write('};\n\n')

class ItemChildEquipmentGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'ItemChildEquipment' ]
        super().__init__(options, data_store)

    def generate(self, ids = None):
        data_str = "%sitem_child_equipment%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        self._out.write('#define %s_SIZE (%d)\n\n' % (data_str.upper(), len(self._itemchildequipment_db.keys()) + 1))

        self._out.write('// Item child equipment, wow build %d\n' % ( self._options.build ))

        self._out.write('static struct item_child_equipment_t __%s_data[%s_SIZE] = {\n' % (data_str, data_str.upper()))

        for key in sorted(self._itemchildequipment_db.keys()) + [0,]:
            data = self._itemchildequipment_db[key]

            fields = data.field( 'id', 'id_item', 'id_child' )
            self._out.write('  { %s },\n' % (', '.join(fields)))

        self._out.write('};\n\n')

class ArtifactDataGenerator(DataGenerator):
    def __init__(self, options, data_store = None):
        self._dbc = [ 'Artifact', 'ArtifactPower', 'ArtifactPowerRank', 'Spell' ]

        super().__init__(options, data_store)

    def filter(self):
        ids = {}

        for id, data in self._artifact_db.items():
            if data.id_spec == 0:
                continue

            ids[id] = { }

        for id, data in self._artifactpower_db.items():
            artifact_id = data.id_artifact

            if artifact_id not in ids:
                continue

            ids[artifact_id][id] = { 'data': data, 'ranks': [] }

        for id, data in self._artifactpowerrank_db.items():
            power_id = data.id_power
            power = self._artifactpower_db[power_id]
            if power.id == 0:
                continue

            if power.id_artifact not in ids:
                continue

            ids[power.id_artifact][power_id]['ranks'].append(data)

        return ids

    def generate(self, ids = None):
        data_str = "%sartifact%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        self._out.write('#define %s_SIZE (%d)\n\n' % (data_str.upper(), len(ids.keys()) + 1))

        self._out.write('// Artifact base data, wow build %d\n' % ( self._options.build ))

        self._out.write('static struct artifact_t __%s_data[%s_SIZE] = {\n' % (data_str, data_str.upper()))

        artifact_keys = sorted(ids.keys())
        powers = []
        for key in artifact_keys + [0]:
            data = self._artifact_db[key]
            fields = data.field( 'id', 'id_spec' )
            self._out.write('  { %s },\n' % (', '.join(fields)))
            if key in ids:
                for _, power_data in ids[key].items():
                    powers.append(power_data)

        self._out.write('};\n')

        data_str = "%sartifact_power%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        self._out.write('#define %s_SIZE (%d)\n\n' % (data_str.upper(), len(powers) + 1))

        self._out.write('// Artifact power data, wow build %d\n' % ( self._options.build ))

        self._out.write('static struct artifact_power_data_t __%s_data[%s_SIZE] = {\n' % (data_str, data_str.upper()))

        ranks = []
        for power in sorted(powers, key = lambda v: (v['data'].id_artifact, v['data'].id)) + [{ 'data': dbc.data.ArtifactPower.default(), 'ranks': [] }]:
            fields = power['data'].field('id', 'id_artifact', 'type', 'index', 'max_rank')
            if len(power['ranks']) > 0:
                spell = self._spell_db[power['ranks'][0].id_spell]
                fields += spell.field('id')
                fields += spell.field('name')
                self._out.write('  { %s }, // %s (id=%u, n_ranks=%u)\n' % (', '.join(fields), spell.name, power['ranks'][0].id_spell, len(power['ranks'])))
            else:
                fields += spell.field('id')
                fields += self._spell_db[0].field('name')
                self._out.write('  { %s },\n' % (', '.join(fields)))
            ranks += power['ranks']

        self._out.write('};\n')

        data_str = "%sartifact_power_rank%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        self._out.write('#define %s_SIZE (%d)\n\n' % (data_str.upper(), len(ranks) + 1))

        self._out.write('// Artifact power rank data, wow build %d\n' % ( self._options.build ))

        self._out.write('static struct artifact_power_rank_t __%s_data[%s_SIZE] = {\n' % (data_str, data_str.upper()))

        hotfix_data = {}
        for rank in sorted(ranks, key = lambda v: (v.id_power, v.index)) + [dbc.data.ArtifactPowerRank.default()]:
            fields = rank.field('id', 'id_power', 'index', 'id_spell', 'value')
            f, hfd = rank.get_hotfix_info(('id_power', 1), ('index', 2), ('id_spell', 3), ('value', 4))
            fields += [ '%#.8x' % f, '0' ]
            if f > 0:
                hotfix_data[rank.id] = hfd

            self._out.write('  { %s },\n' % (', '.join(fields)))

        self._out.write('};\n')

        output_hotfixes(self, data_str, hotfix_data);

