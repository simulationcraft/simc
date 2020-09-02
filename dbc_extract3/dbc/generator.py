import sys, os, re, types, html.parser, urllib, datetime, signal, json, pathlib, csv, logging, io, fnmatch, traceback, binascii, time

from collections import defaultdict

import dbc.db, dbc.data, dbc.parser, dbc.file

from dbc import constants, util
from dbc.filter import ActiveClassSpellSet, PetActiveSpellSet, RacialSpellSet, MasterySpellSet, RankSpellSet, ConduitSet, SoulbindAbilitySet, CovenantAbilitySet

# Special hotfix field_id value to indicate an entry is new (added completely through the hotfix entry)
HOTFIX_MAP_NEW_ENTRY  = 0xFFFFFFFF

def escape_string(tmpstr):
    tmpstr = tmpstr.replace("\\", "\\\\")
    tmpstr = tmpstr.replace("\"", "\\\"")
    tmpstr = tmpstr.replace("\n", "\\n")
    tmpstr = tmpstr.replace("\r", "\\r")

    return tmpstr

class HotfixDataRecord(object):
    def __init__(self):
        self._new_entry = False
        self._data = []
        self._map_indices = set()

    def add(self, record, *args):
        if self._new_entry:
            return

        for field_name, map_index in args:
            if map_index in self._map_indices:
                raise ValueError('Duplicate hotfix data map_index: {}'.format(map_index))
            self._map_indices.add(map_index)

            if isinstance(field_name, tuple):
                fields = ((n, map_index) for n in field_name)
                flags = record.get_hotfix_info(*fields)[0]
                if flags != 0:
                    self._data.append((map_index, 'X', 0, 0))
            else:
                self._data += record.get_hotfix_info((field_name, map_index))[1]

    @property
    def new_entry(self):
        return self._new_entry

    @new_entry.setter
    def new_entry(self, value):
        if value == True and self._new_entry != value:
            self._new_entry = value
            self._data = [(HOTFIX_MAP_NEW_ENTRY, 'i', 0, 0)]

    @property
    def data(self):
        return self._data

class HotfixDataGenerator(object):
    def __init__(self, name):
        assert isinstance(name, str)
        self._name = name
        self._data = {}

    def add(self, id, record):
        if len(record.data) > 0:
            logging.debug('Hotfixed {} {}, original values: {}'.format(self._name, id, record.data))
            self._data[id] = record.data

    def add_single(self, id, entry, *args):
        record = HotfixDataRecord()
        record.add(entry, *args)
        self.add(id, record)

    def output(self, generator):
        generator._out.write('// {} hotfix entries, wow build {}\n'.format(
            self._name, generator._options.build ))
        generator._out.write('static constexpr std::array<hotfix::client_hotfix_entry_t, {}> __{}_data {{ {{\n'.format(
            sum( len(entries) for entries in self._data.values() ),
            generator.format_str(self._name + '_hotfix') ))

        for id, entry in sorted(self._data.items()):
            for field_id, field_format, orig_field_value, new_field_value in sorted(entry, key=lambda e: e[0]):
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
                elif field_format in ['X']:
                    orig_data_str = 'hotfix::client_hotfix_entry_t::flags_value_t{}'
                    cur_data_str = orig_data_str
                else:
                    if orig_field_value == 0:
                        orig_data_str = 'nullptr'
                    else:
                        orig_data_str = '"%s"' % escape_string(orig_field_value)
                    if new_field_value == 0:
                        cur_data_str = 'nullptr'
                    else:
                        cur_data_str = '"%s"' % escape_string(new_field_value)
                generator._out.write('  { %7u, %2u, %s, %s },\n' % (
                    id, field_id, orig_data_str, cur_data_str
                ))

        generator._out.write('} };\n\n')


class CSVDataGenerator(object):
    def __init__(self, options, csvs, base_type = None):
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
        return 'static %s _%s%s%s%s = {\n' % (
            self.base_type(dbc),
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
            simple_reader = self._csv_options.get(dbc, {}).get('simple_reader', False)

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
                    # Simple csv reader skips the first row header
                    if simple_reader and row == data[0]:
                        continue

                    level = int(row[self.key(dbc)])
                    # Simple reader rows come as tuples
                    if simple_reader:
                        self._out.write('%s,\t' % (fmt % row[name]))
                    else:
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

    def base_type(self, file_):
        return self._csv_options.get(file_, {}).get('base_type', 'double')

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
            if self._csv_options.get(i, {}).get('simple_reader', False):
                dbcp = csv.reader(open(v, 'r'), delimiter = '\t')
            else:
                dbcp = csv.DictReader(open(v, 'r'), delimiter = '\t')

            if self.dbname(i) not in dir(self):
                setattr(self, self.dbname(i), dbcp)

        return True

class DataGenerator(object):
    _class_names = [ None, 'Warrior', 'Paladin', 'Hunter', 'Rogue',     'Priest', 'Death Knight', 'Shaman', 'Mage',  'Warlock', 'Monk',       'Druid', 'Demon Hunter'  ]
    _class_masks = [ None, 0x1,       0x2,       0x4,      0x8,         0x10,     0x20, 0x40,     0x80,    0x100,     0x200,        0x400, 0x800   ]
    _race_names  = [ None, 'Human',   'Orc',     'Dwarf',  'Night Elf', 'Undead', 'Tauren',       'Gnome',  'Troll', 'Goblin',  'Blood Elf', 'Draenei', 'Dark Iron Dwarf', 'Vulpera', 'Mag\'har Orc', 'Mechagnome' ] + [ None ] * 6 + [ 'Worgen', None, None, 'Pandaren', None, 'Nightborne', 'Highmountain Tauren', 'Void Elf', 'Lightforged Draenei', 'Zandalari Troll', 'Kul Tiran' ]
    _race_masks  = [ None, 0x1,       0x2,       0x4,      0x8,         0x10,     0x20,           0x40,     0x80,    0x100,     0x200,       0x400,     0x800,             0x1000, 0x2000, 0x4000       ] + [ None ] * 6 + [ 0x200000, None, None, 0x1000000,  None, 0x4000000,    0x8000000,             0x10000000, 0x20000000,            0x40000000,        0x80000000 ]
    _pet_names   = [ None, 'Ferocity', 'Tenacity', None, 'Cunning' ]
    _pet_masks   = [ None, 0x1,        0x2,        None, 0x4       ]

    def __init__(self, options, data_store):
        self._options = options
        self._data_store = data_store
        self._out = None
        if not hasattr(self, '_dbc'):
            self._dbc = []

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

    def db(self, dbname):
        return dbc.db.datastore(self._options).get(dbname)

    def output_record(self, *args, fmt = None, comment = None):
        if fmt == None:
            fmt = '  {{ {} }},{}\n'

        a = list(*args)
        if comment:
            comment_str = ' // {}'.format(comment)
        else:
            comment_str = ''

        self._out.write(fmt.format(', '.join(a), comment_str))

    def output_header(self, *args, **kwargs):
        header_str = kwargs.get('header', None)
        typename = kwargs.get('type', None)
        l = kwargs.get('length', None)
        arrayname = kwargs.get('array', None)

        if not l or not isinstance(l, int):
            logging.error('Array length for {} must be a postive integer, not {}'.format(
                self.__class__.__name__, l))
            return

        if not typename or not isinstance(typename, str):
            logging.error('Structure name for {} must be a string'.format(
                self.__class__.__name__))
            return

        if not header_str and not arrayname:
            logging.error('Name or array for {} must be given'.format(
                self.__class__.__name__))
            return

        if not arrayname:
            arrayname = header_str.lower().replace(' ', '_')

        if header_str:
            self._out.write('// {}, wow build {}\n'.format(header_str, self._options.build))

        self._out.write('static const std::array<{}, {}> __{}_data {{ {{\n'.format(
            typename, l, self.format_str(arrayname)))

    def output_footer(self):
        self._out.write('} };\n\n')

    def output_id_index(self, *args, **kwargs):
        index = kwargs.get('index', None)
        arrayname = kwargs.get('array', None)

        if not index and not isinstance(index, list):
            logging.error('Index for {} must be a list, not {}'.format(
                self.__class__.__name__, index))
            return

        if not arrayname:
            logging.error('Name of the index for {} must be given'.format(
                self.__class__.__name__))
            return

        typename = 'uint16_t'
        if len(index) >= 1 << 16:
            typename = 'uint32_t'

        self._out.write('static const std::array<{}, {}> __{}_id_index {{ {{\n'.format(
            typename, len(index), self.format_str(arrayname)))
        for i in range(0, len(index), 8):
            self._out.write('  {},\n'.format(', '.join('{:>5}'.format(idx) for idx in index[i: i + 8])))
        self._out.write('} };\n\n')

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
            self._out = pathlib.Path(obj).open(append and 'a' or 'w', encoding="utf-8")
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
        if self._options.output:
            self._out = pathlib.Path(self._options.output).open('w', encoding = "utf-8")
        elif self._options.append:
            self._out = pathlib.Path(self._options.append).open('a', encoding = "utf-8")
        else:
            self._out = sys.stdout

        if not self._out.writable():
            logging.error('Unable to write to file "%s"', self._options.append)
            return False

        for i in self._dbc:
            dbase = self._data_store.get(i)
            if '_%s_db' % self.attrib_name(i) not in dir(self):
                setattr(self, '_%s_db' % self.attrib_name(i), dbase)

        return True

    def filter(self):
        return None

    def generate(self, ids = None):
        return ''

class RealPPMModifierGenerator(DataGenerator):
    def __init__(self, options, data_store):
        super().__init__(options, data_store)

        self._dbc = [ 'ChrSpecialization', 'SpellProcsPerMinute', 'SpellProcsPerMinuteMod', 'SpellAuraOptions' ]

    def generate(self, ids = None):
        output_data = []

        for i, data in self._spellprocsperminutemod_db.items():
            ppm_id = data.id_parent
            spell_id = 0
            for aopts_id, aopts_data in self._spellauraoptions_db.items():
                if aopts_data.id_ppm != ppm_id:
                    continue

                spell_id = aopts_data.id_parent
                if spell_id == 0:
                    continue

                output_data.append((data.id_chr_spec, data.coefficient, spell_id, data.unk_1))

        self._out.write('// RPPM Modifiers, wow build level %s\n' % self._options.build)
        self._out.write('static const std::array<rppm_modifier_t, %d> __%s_data { {\n' % (
            len(output_data), self.format_str('rppm_modifier')))

        for data in sorted(output_data, key = lambda v: v[2]):
            self._out.write('  { %6u, %4u, %2u, %7.4f },\n' % (
                data[2], data[0], data[3], data[1]))

        self._out.write('} };\n')

class SpecializationEnumGenerator(DataGenerator):
    def __init__(self, options, data_store):
        super().__init__(options, data_store)

        self._dbc = [ 'ChrSpecialization' ]

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
            # Ignore "Initial" specializations for now
            # TODO: Revisit
            if spec_data.name == 'Initial':
                continue

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
    def __init__(self, options, data_store):
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
            if spec_data.name == 'Initial':
                continue

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
    def __init__(self, options, data_store):
        super().__init__(options, data_store)

        self._dbc = [ 'Talent', 'ChrSpecialization', 'SpellName' ]

    def filter(self):
        ids = [ ]

        for talent_id, talent_data in self._talent_db.items():
            # Make sure at least one spell id is defined
            if talent_data.id_spell == 0:
                continue

            # Make sure the "base spell" exists
            if self._spellname_db[talent_data.id_spell].id != talent_data.id_spell:
                continue

            ids.append(talent_id)

        return ids

    def generate(self, ids = None):
        # Sort keys
        ids.sort()

        self._out.write('// %d talents, wow build %s\n' % ( len(ids), self._options.build ))
        self._out.write('static std::array<talent_data_t, %d> __%s_data { {\n' % (
            len(ids), self.format_str( 'talent' ) ))

        index = 0
        for id in ids:
            talent = self._talent_db[id]
            spell  = self._spellname_db[talent.id_spell]

            if not spell.id and talent.id_spell > 0:
                continue

            #if( index % 20 == 0 ):
            #    self._out.write('//{ Name                                ,    Id, Flgs,  Class, Spc, Col, Row, SpellID, ReplcID, S1 },\n')

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

        self._out.write('} };')

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
        133755,                                     # Underlight Angler (Legion artifact fishing pole)
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

    def __init__(self, options, data_store):
        super().__init__(options, data_store)

        self._dbc = [ 'Item', 'ItemEffect', 'SpellEffect', 'SpellName', 'JournalEncounterItem', 'ItemNameDescription' ]
        if options.build < 23436:
            self._dbc.append('Item-sparse')
        else:
            self._dbc.append('ItemSparse')

    def initialize(self):
        if not DataGenerator.initialize(self):
            return False

        self._data_store.link('SpellEffect',        self._options.build < 25600 and 'id_spell' or 'id_parent', 'SpellName', 'add_effect' )
        self._data_store.link('ItemEffect', self._options.build < 25600 and 'id_item' or 'id_parent', self._options.build < 23436 and 'Item-sparse' or 'ItemSparse', 'spells')
        self._data_store.link('JournalEncounterItem', 'id_item', self._options.build < 23436 and 'Item-sparse' or 'ItemSparse', 'journal')

        return True

    def filter(self):
        ids = []
        db = self._options.build < 23436 and self._item_sparse_db or self._itemsparse_db
        for item_id, data in db.items():
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
            if item_id in constants.ITEM_WHITELIST:
                filter_ilevel = False
            elif classdata.classs in [ 2, 4 ]:
                # All shirts
                if data.inv_type == 4:
                    filter_ilevel = False
                # All tabards
                elif data.inv_type == 19:
                    filter_ilevel = False
                # All epic+ armor/weapons
                elif data.quality >= 4:
                    filter_ilevel = False
                else:
                    # On-use item, with a valid spell (and cooldown)
                    for item_effect in data.get_links('spells'):
                        if item_effect.trigger_type == 0 and item_effect.id_spell > 0 and (item_effect.cooldown_group_duration > 0 or item_effect.cooldown_duration > 0):
                            filter_ilevel = False
                            break
            # Gems
            elif classdata.classs == 3 or (classdata.classs == 7 and classdata.subclass == 4):
                if data.gem_props == 0:
                    continue
                else:
                    filter_ilevel = False
            # Consumables
            elif classdata.classs == 0:
                # Potions, Elixirs, Flasks. Simple spells only.
                if classdata.has_value('subclass', [1, 2, 3]):
                    for item_effect in data.get_links('spells'):
                        spell = self._spellname_db[item_effect.id_spell]
                        if not spell.has_effect('type', 6):
                            continue

                        # Grants armor, stats, rating or direct trigger of spells
                        if not spell.has_effect('sub_type', [13, 22, 29, 99, 189, 465, 43, 42]):
                            continue

                        filter_ilevel = False
                # Food
                elif classdata.has_value('subclass', 5):
                    for item_effect in data.get_links('spells'):
                        spell = self._spellname_db[item_effect.id_spell]
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
                    # Finally, check consumable whitelist
                    map_ = constants.CONSUMABLE_ITEM_WHITELIST.get(classdata.subclass, {})
                    if item_effect.id_parent in map_:
                        filter_ilevel = False
                    else:
                        continue
            # Hunter scopes and whatnot
            elif classdata.classs == 7:
                if classdata.has_value('subclass', 3):
                    for item_effect in data.get_links('spells'):
                        spell = self._spellname_db[item_effect.id_spell]
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

                    spell = self._spellname_db[spell_id]
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

        # All timewalking items through JournalEncounter
        journal_item_ids = [
            v for v in self.db('JournalItemXDifficulty').values() if v.id_difficulty in [24, 33]
        ]

        for entry in journal_item_ids:
            item = entry.parent_record().ref('id_item')
            if not item.id:
                continue

            item_ = self.db('Item')[item.id]
            if item_.id == 0:
                continue

            if item_.classs in [2, 4] and item.id not in ids:
                ids.append(item.id)

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

        # filter out missing items XXX: is this possible?
        def check_item(id):
            item = self._itemsparse_db[id]
            if not item.id and id > 0:
                sys.stderr.write('Item id {} not found\n'.format(id))
                return False
            return True
        ids[:] = [ id for id in ids if check_item(id) ]

        class Index(object):
            def __init__(self):
                self._list = []
                self._index = {}
                self._dict = defaultdict(list)

            def __len__(self):
                return len(self._list)

            def add(self, id, data):
                assert isinstance(data, list)
                assert len(data)

                pos = self._find_pos(data)
                if pos is None:
                    pos = len(self._list)
                    for i in range(len(data)):
                        self._dict[data[i]].append(pos + i)
                    self._list.extend(data)
                self._index[id] = (pos, len(data))

            def get(self, id):
                return self._index.get(id)

            def items(self):
                return self._list

            def _find_pos(self, data):
                for pos in self._dict[data[0]]:
                    if self._list[pos:pos + len(data)] == data:
                        return pos
                return None

        items_stats_index = Index()

        for id in ids:
            item = self._itemsparse_db[id]
            stats = []
            for i in range(1, 11):
                stat_type = getattr(item, 'stat_type_{}'.format(i))
                if stat_type > 0:
                    stats.append(( stat_type,
                                   getattr(item, 'stat_alloc_{}'.format(i)),
                                   item.field('stat_socket_mul_{}'.format(i))[0] ))
            if len(stats):
                items_stats_index.add(id, sorted(stats, key=lambda s: s[:2]))

        self._out.write('static const dbc_item_data_t::stats_t __{}_data[{}] = {{\n'.format(
            self.format_str('item_stats'), len(items_stats_index)))
        for stats in items_stats_index.items():
            self._out.write('  {{ {:>2}, {:>5}, {} }},\n'.format(*stats))
        self._out.write('};\n')

        def item_stats_fields(id):
            stats = items_stats_index.get(id)
            if stats is None:
                return [ '0', '0' ]
            return [ '&__{}_data[{}]'.format(self.format_str('item_stats'), stats[0]), str(stats[1]) ]

        self._out.write('// Items, ilevel %d-%d, wow build level %s\n' % (
            self._options.min_ilevel, self._options.max_ilevel, self._options.build))
        self._out.write('static const std::array<dbc_item_data_t, %d> __%s_data { {\n' % (
            len(ids), self.format_str('item')))

        index = 0
        for id in ids:
            item = self._itemsparse_db[id]
            item2 = self._item_db[id]

            fields = item.field('name', 'id', 'flags_1', 'flags_2')

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
            fields += item.field('bonding', 'delay', 'dmg_range', 'item_damage_modifier')
            fields += item_stats_fields(id)
            fields += item.field('class_mask', 'race_mask')
            fields += [ '{ %s }' % ', '.join(item.field('socket_color_1', 'socket_color_2', 'socket_color_3')) ]
            fields += item.field('gem_props', 'socket_bonus', 'item_set', 'id_curve', 'id_artifact' )

            self._out.write('  { %s },\n' % (', '.join(fields)))

        self._out.write('} };\n\n')

    def generate_json(self, ids = None):

        ids.sort()

        s2 = dict()
        s2['wow_build'] = self._options.build
        s2['min_ilevel'] = self._options.min_ilevel
        s2['max_ilevel'] = self._options.max_ilevel
        s2['num_items'] = len(ids)

        s2['items'] = list()

        for id in ids + [0]:
            item = self._options.build < 23436 and self._item_sparse_db[id] or self._itemsparse_db[id]
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
         # 7.1.5 de-links Mark of the Hidden Satyr damage spell fully from the driver
         191259,
         # 7.1.5 Entwined Elemental Foci buffs
         225729, 225730,
         # 7.1.5 Archimonde's Hatred Reborn damage spell
         235188,
         # 7.2.0 Dreadstone of Endless Shadows stat buffs
         238499, 238500, 238501,
         # 7.3.0 Netherlight stuff
         252879, 252896, 253098, 253073,
         # 7.3.0 Acrid Catalyst Injector stat buffs
         255742, 255744,
         # 7.3.0 Sheath of Asara
         255870, 257702,
         # 7.3.0 Terminus Signaling Beacon
         257376,
         # 7.3.0 Gorshalach's Legacy
         255672,
         # 7.3.0 Forgefiend's Fabricator
         253322, 256025,
         # 7.3.0 Riftworld Codex buffs and damage component
         252545, 251938, 256415, 252550,
         # 7.3.5 Auxilary spells for the new races
         259756, 259760,
         # 8.0.1 Azerite power Elemental Whirl buffs
         268954, 268955, 268953,
         # Mag'har Orc Ancestral Call buffs
         274739, 274740, 274741, 274742,
         # 8.0 Galley Banquet food buffs
         259448, 259449, 259452, 259453,
         # 8.0 Bountiful Captain's Feast food buffs
         259454, 259455, 259456, 259457,
         # 8.1 Boralus Blood Sausage food buffs
         290469, 290467, 290468,
         # 8.0 Incendiary Ammunition
         265092,
         # Void Slash (Void Stalker's Contract damage spell)
         251034,
         # 8.0.1 Azerite power Thunderous Blast buff
         280400,
         # 8.0.1 Azerite power Retaliatory Fury & Last Gift buffs
         280787, 280861,
         # 8.0.1 Azerite power Azerite Globules
         279958,
         # 8.0.1 Azerite power Heed My Call
         271685, 271686,
         # 8.0.1 Azerite power Gutripper
         269031,
         # 8.0.1 Azerite power Overwhelming Power
         271711,
         # 8.0.1 Azerite power Earthlink
         279928,
         # 8.0.1 Frenetic Corpuscle trinket
         278144,
         # 8.0.1 Azerite power Laser Matrix
         280705, 280707,
         # 8.0.1 Harlan's Loaded dice
         267325, 267326, 267327, 267329, 267330, 267331,
         # 8.0.1 Kul Tiran Cannonball Runner damage
         271197,
         # 8.0.1 Endless Tincture of Fractional Power
         265442, 265443, 265444, 265446,
         # Hadal's Nautilus damage
         270925,
         # Incessantly Ticking Clock mastery buff
         274431,
         # Combined Might buffs
         280841, 280842, 280843, 280844, 280845,
         280866, 280867, 280868, 280869, 280870,
         # Synaptic Spark Capacitor
         280846,
         # Battlefield Focus/Precision debuff and damages
         280817, 282724, 280855, 282720,
         # Squalls Deck spells
         276124, 276125, 276126, 276127, 276128, 276129, 276130, 276131,
         # Fathoms Deck spells
         276187, 276188, 276189, 276190, 276191, 276192, 276193, 276194, 276196, 276199,
         # Leyshock's Grand Compendium
         281792, 281794, 281795,
         # Bloody Bite for Vanquished Tendril of G'huun
         279664,
         # Secrets of the Sands for Sandscoured Idol
         278267,
         # Deep Roots for Ancients' Bulwark
         287610,
         # Webweaver's Soul Gem
         270827,
         # Boralus Blood Sausage Feast
         290469, 290468, 290467, 288075, 288074,
         # Buffs for Variable Intensity Gigavolt Oscillating Reactor
         287916, 290052, 290051, 290042, 287967,
         # Tidestorm Codex
         288086,
         #Zandalari Racials - Permanent Buffs
         292359, 292360, 292361, 292362, 292363, 292364,
         #Zandalari Racials - Spell Procs
         292380, 292463, 292473, 292474, 292486,
         #Leggings of the Aberrant Tidesage damage proc
         295811,
         # Fa'thuul's Floodguards
         295257,
         # Legplates of Unbound Anguish damage proc
         295428,
         # Darkmoon Faire food buff
         185786,
         # The Unbound Force Azerite Essence damage spell
         298453,
         # Essence of the Focusing Iris major damage spell
         295374, 295338,
         # Spell cast by summon from Condensed Life-Force major power
         295856,
         # 8.2 Famine Evaluator And Snack Table buffs
         297116, 297117, 297118, 297119,
         # 8.2 Force Multiplier enchant buffs (str, crit, haste, mastery, agi)
         300691, 300801, 300802, 300809, 300893,
         # 8.2 Machinist's Brilliance enchant buffs (crit, int, haste, mastery)
         298431, 300693, 300761, 300762,
         # 8.2 Naga Hide enchant buffs (agi, str)
         298466, 300800,
         # 8.2 Vision of Perfection RPPM
         297866, 297868, 297869,
         # Volcanic Eruption for the Highborne Compendium of Sundering
         300907,
         # Nullification Dynamo Essence damage spell
         296061,
         # Shiver Venom Relic
         305290,
         # Shivering Bolt/Venomous Lance (Shiver Venom Crossbow/Lance bonus damage)
         303559, 303562,
         # Leviathan's Lure
         302763,
         # Za'qul's Portal key
         302960,
         # Ashvane's Razor Coral
         303568, 303572,
         # Dribbling Inkpod
         302491,
         # Remote Guidance Device
         302311, 302312,
         # Diver's Folly
         303583, 303621,
         # Storm of the Eternal (Eternal Palace 2min cycle buff)
         303731,
         # Undulating Tides lockout debuff
         303438,
         # Exploding Pufferfish
         305315,
         # Harbinger's Inscrutable Will
         295395,
         # Mechagon combo rings
         306474, 301886, 301887,
         # Anu-Azshara lockout debuff
         304482,
         # 8.3.0 Essences
         # Breath of the Dying (minor damage spell, R2 minor heal spell, R3 major buff on kill, R3 reset grace period)
         311192, 311201, 311202, 311947,
         # Spark of Inspiration (minor buff)
         313643,
         # The Formless Void (major unknown buff)
         312734,
         # 8.3 Raid items/Corruption
         # Infinite Stars
         317262, 317265,
         # Titanic Empowerment Set
         315793, 315858,
         # Psyche Shredder damage spell
         316019,
         # Torment in a Jar stacks
         313088,
         # Echoing Void damage+period
         317029,317022,
         # Twilight Devastation damage
         317159,
         # Obsidian Destruction Range
         316661,
         # Twisted Appendage damage
         316835,
         # Searing Breath damage
         316704,
         # Shadowlands
         # Soulbinds
         320130, 320212, # Social Butterfly vers buff (night fae/dreamweaver)
         342181, 342183, # Embody the Construct damage/heal (necrolord/emeni)
        ),

        # Warrior:
        (
            ( 58385,  0 ),          # Glyph of Hamstring
            ( 118779, 0, False ),   # Victory Rush heal is not directly activatable
            ( 144442, 0 ),          # T16 Melee 4 pc buff
            ( 119938, 0 ),          # Overpower
            ( 209700, 0 ),          # Void Cleave (arms artifact gold medal)
            ( 218877, 0 ),          # Gaze of the Val'kyr
            ( 218850, 0 ),          # Gaze of the Val'kyr
            ( 218836, 0 ),          # Gaze of the Val'kyr
            ( 218835, 0 ),          # Gaze of the Val'kyr (heal)
            ( 218834, 0 ),          # Gaze of the Val'kyr
            ( 218822, 0 ),          # Gaze of the Val'kyr
            ( 209493, 0 ),          # Precise Strikes
            ( 242952, 0 ),		    # Bloody Rage
            ( 242953, 0 ),          # Bloody Rage
            ( 278497, 0 ),          # Seismic Wave (Azerite)
            ( 195707, 0 ),          # Rage gain from taking hits
            ( 161798, 0 ),          # Riposte passive (turns crit rating into parry)
            ( 279142, 0 ),          # Iron Fortress damage (azerite)
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
            ( 238996, 0 ),          # Righteous Verdict
            ( 242981, 0 ),          # Blessing of the Ashbringer
            ( 211561, 0 ),          # Justice Gaze
            ( 246973, 0 ),          # Sacred Judgment (Ret T20 4p)
            ( 275483, 0 ),          # Inner Light azerite trait damge
            ( 184689, 0 ),          # Shield of Vengeance damage proc
            ( 286232, 0 ),          # Light's Decree damage proc
            ( 339669, 0 ),          # Seal of Command
            ( 339376, 0 ),          # Truth's Wake
            ( 339538, 0 ),          # TV echo
        ),

        # Hunter:
        (
          ( 75, 0 ),     # Auto Shot
          ( 131900, 0 ), # Murder of Crows damage spell
          ( 312365, 0 ), # Thrill of the Hunt
          ( 171457, 0 ), # Chimaera Shot - Nature
          ( 201594, 1 ), # Stampede
          ( 118459, 5 ), # Beast Cleave
          ( 186254, 5 ), # Bestial Wrath
          ( 257622, 2 ), # Trick Shots buff
          ( 260395, 2 ), # Lethal Shots buff
          ( 259516, 3 ), # Flanking Strike
          ( 267666, 3 ), # Chakrams
          ( 265888, 3 ), # Mongoose Bite (AotE version)
          # Wildfire Infusion (Volatile Bomb spells)
          ( 271048, 3 ), ( 271049, 3 ), ( 260231, 3 ),
          ( 273289, 0 ), # Latent Poison (Azerite)
          ( 272745, 0 ), # Wildfire Cluster (Azerite)
          ( 278565, 0 ), # Rapid Reload (Azerite)
          ( 120694, 0 ), # Dire Beast energize
          ( 83381, 5 ),  # BM Pet Kill Command
          ( 259277, 5 ), # SV Pet Kill Command
          ( 206685, 0 ), # BM Spitting Cobra Cobra Spit
          ( 324156, 0 ), # Flayer's Mark (Covenenat)
          ( 328757, 0 ), # Wild Spirits (Covenenat)
          ( 339928, 2 ), ( 339929, 2 ), # Brutal Projectiles (Conduit)
          ( 341223, 3 ), # Strength of the Pack (Conduit)
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
            ( 246558, 0 ),          # Outlaw T20 4pc Lesser Adrenaline Rush
            ( 279043, 0 ),          # Shadow Blades damaging spell
            ( 121153, 0 ),          # Blindside proc buff
            ( 278981, 0 ),          # The First Dance, Azerite trait aura
            ( 273009, 0 ),          # Double Dose, Azerite trait dmg spell
            ( 278962, 0 ),          # Paradise Lost, Azerite trait buff
            ( 286131, 0 ),          # Replicating Shadows, Azerite trait dmg effect
            ( 197834, 0 ),          # Sinister Strike extra attack damage spell
            ( 257506, 0 ),          # Shot in the Dark buff for free Cheap Shot casts
            ( 282449, 0 ),          # Secret Technique clone attack
            ( 130493, 0 ),          # Nightstalker dmg increase driver
            ( 227151, 0 ),          # Symbols of Death Rank 2 autocrit buff
            ( 328082, 0 ),          # Eviscerate rank 2 shadow damage spell
            ( 341541, 0 ),          # Sinister Strike third attack from Triple Threat conduit
            ( 328306, 0 ),          # Sepsis expiry direct damage hit
            ( 323660, 0 ),          # Slaughter instant damage
            ( 323558, 0 ), ( 323559, 0 ), ( 323560, 0 ), # Echoing Reprimand buffs
            ( 324074, 0 ), ( 341277, 0 ), # Serrated Bone Spike secondary instant damage spells
            ( 340582, 0 ), ( 340583, 0 ), ( 340584, 0 ), # Guile Charm legendary buffs
            ( 340600, 0 ), ( 340601, 0 ), ( 340603, 0 ), # Finality legendary buffs
            ( 341111, 0 ),          # Akaari's Soul Fragment legendary debuff
            ( 340587, 0 ),          # Concealed Blunderbuss legendary buff
            ( 340573, 0 ),          # Greenskin's Wickers legendary buff
            ( 340431, 0 ),          # Doomblade legendary debuff
            ( 343173, 0 ),          # Premeditation buff
            ( 319190, 0 ),          # Shadow Vault shadow damage spell
        ),

        # Priest:
        (   (  63619, 5 ), 			# Shadowfiend "Shadowcrawl"
            (  94472, 0 ), 			# Atonement Crit
            ( 114908, 0, False ), 	# Spirit Shell absorb
            ( 190714, 3, False ), 	# Shadow Word: Death - Insanity gain
            ( 193473, 5 ),			# Void Tendril "Mind Flay"
            ( 217676, 3 ),			# Mind Spike Detonation
            ( 194249, 3, False ),   # Void Form extra data
            ( 212570, 3, False ),   # Surrendered Soul (Surrender To Madness Death)
            ( 269555, 3 ),          # Azerite Trait Torment of Torments
            ( 280398, 1, False ),   # Sins of the Many buff
            ( 275725, 0 ),          # Whispers of the Damned trigger effect
            ( 275726, 0 ),          # Whispers of the damned insanity gain
            ( 288342, 0 ),          # Thought Harvester trigger buff for Mind Sear
            ( 336142, 5 ),          # Shadowflame Prism legendary effect DMG Component
            ( 343144, 0 ),          # Dissonant Echoes free Void Bolt proc
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
          ( 57330, 0, True ), # Horn of Winter needs to be explicitly put in the general tree, as our (over)zealous filtering thinks it's not an active ability
          ( 47568, 0, True ), # Same goes for Empower Rune Weapon
          ( 211947, 0 ),    # Shadow Empowerment for Gargoyle
          ( 81141, 0 ),     # Crimson Scourge buff
          ( 212423, 5 ),    # Skulker Shot for All Will Serve
          ( 45470, 0 ),     # Death Strike heal
          ( 196545, 0 ),    # Bonestorm heal
          ( 253590, 0 ),    # T21 4P frost damage component
          ( 274373, 0 ),    # Festermight azerite trait
          ( 199373, 5 ),    # Army ghoul claw spell
          ( 275918, 0 ),    # Echoing Howl azerite trait
          ( 279606, 0 ),    # Last Surprise azerite trait
          ( 273096, 0 ),    # Horrid Experimentation azerite trait
          ( 49088, 0 ),     # Anti-magic Shell RP generation
          ( 288548, 5 ), ( 288546, 5 ),   # Magus of the Dead's (azerite trait) Frostbolt and Shadow Bolt spells
          ( 286836, 0 ), ( 286954, 0 ), ( 290814, 0 ), # Helchains damage (azerite trait)
          ( 287320, 0 ),    # Frostwhelp's Indignation (azerite)
          ( 283499, 0 ), ( 292493, 0 ), # Frost Fever's RP generation spells
          ( 302656, 0 ), # Vision of Perfection's resource generation for Frost DK

        ),

        # Shaman:
        ( (  77451, 0 ), (  45284, 0 ), (  45297, 0 ),  #  Overloads
          ( 114074, 1 ), ( 114738, 0 ),                 # Ascendance: Lava Beam, Lava Beam overload
          ( 120687, 0 ), ( 120588, 0 ),                 # Stormlash, Elemental Blast overload
          ( 121617, 0 ),                                # Ancestral Swiftness 5% haste passive
          ( 25504, 0, False ), ( 33750, 0, False ),     # Windfury passives are not directly activatable
          ( 8034, 0, False ),                           # Frostbrand false positive for activatable
          ( 145002, 0, False ),                         # Lightning Elemental nuke
          ( 157348, 5 ), ( 157331, 5 ), ( 157375, 5 ),  # Storm elemental spells
          ( 275382, 5 ),                                # Azerite trait 'Echo of the Elementals' Ember Elemental attack
          ( 275386, 5 ),                                # Azerite trait Spark Elemental
          ( 275384, 5 ),                                # Spark Elemental attack
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
          ( 252143, 0 ),                                # Earth Shock Overload (Elemental T21 2PC)
          ( 279515, 0 ),                                # Roiling Storm
          ( 275394, 0 ),                                # Lightning Conduit
          ( 273466, 0 ),                                # Strength of Earth
          ( 279556, 0 ),                                # Rumbling Tremors damage spell
          ( 286976, 0 ),                                # Tectonic Thunder Azerite Trait buff
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
          ( 244813, 0 ),                            # Second Living Bomb DoT
          ( 187677, 0 ),                            # Aegwynn's Ascendance AOE
          ( 191764, 0 ), ( 191799, 0 ),             # Arcane T18 2P Pet
          ( 194432, 0 ),                            # Felo'melorn - Aftershocks
          ( 225119, 5 ),                            # Arcane Familiar attack, Arcane Assault
          ( 210833, 0 ),                            # Touch of the Magi
          ( 228358, 0 ),                            # Winter's Chill
          ( 242253, 0 ),                            # Frost T20 2P Frozen Mass
          ( 240689, 0 ),                            # Aluneth - Time and Space
          ( 248147, 0 ),                            # Erupting Infernal Core
          ( 248177, 0 ),                            # Rage of the Frost Wyrm
          ( 222305, 0 ),                            # Sorcerous Fireball
          ( 222320, 0 ),                            # Sorcerous Frostbolt
          ( 222321, 0 ),                            # Sorcerous Arcane Blast
          ( 205473, 0 ),                            # Icicles buff
          ( 253257, 0 ),                            # Frost T21 4P Arctic Blast
          ( 187292, 0 ),                            # Ro3 buff (?)
          ( 264352, 0 ),                            # Mana Adept
          ( 263725, 0 ),                            # Clearcasting buff
          ( 264774, 0 ),                            # Ro3 buff (talent)
          ( 277726, 0 ),                            # Amplification (?)
          ( 279856, 0 ),                            # Glacial Assault
          ( 333170, 0 ),                            # Molten Skyfall
          ( 333182, 0 ),                            # Molten Skyfall
          ( 333314, 0 ),                            # Sun King's Blessing
          ( 327327, 0 ),                            # Cold Front
          ( 327330, 0 ),                            # Cold Front
          ( 327478, 0 ),                            # Freezing Winds
          ( 327368, 0 ),                            # Disciplinary Command (Fire tracker)
          ( 327369, 0 ),                            # Disciplinary Command (Arcane tracker)
          ( 336889, 0 ),                            # Nether Precision
          ( 337090, 0 ),                            # Siphoned Malice
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
          ( 213229, 0 ),
          ( 243050, 0 ),        # Fire Rift
          ( 242922, 0 ),        # Jaws of Shadow
          ( 270569, 2 ),	# From the Shadows debuff
          ( 265279, 5 ),	# Demonic Tyrant - Demonfire Blast
          ( 270481, 5 ),	# Demonic Tyrant - Demonfire
          ( 267971, 5 ),	# Demonic Tyrant - Demonic Consumption
          ( 267972, 5 ),	# Demonic Tyrant - Demonic Consumption (not sure which is needed)
          ( 267997, 5 ),	# Vilefiend - Bile Spit
          ( 267999, 5 ),	# Vilefiend - Headbutt
          ( 279910, 2 ),	# Summon Wild Imp - Inner Demons version
          ( 267994, 2 ),	# Summon Shivarra
          ( 267996, 2 ),	# Summon Darkhound
          ( 267992, 2 ),	# Summon Bilescourge
          ( 268001, 2 ),	# Summon Ur'zul
          ( 267991, 2 ),	# Summon Void Terror
          ( 267995, 2 ),	# Summon Wrathguard
          ( 267988, 2 ),	# Summon Vicious Hellhound
          ( 267987, 2 ),	# Summon Illidari Satyr
          ( 267989, 2 ),	# Summon Eyes of Gul'dan
          ( 267986, 2 ),	# Summon Prince Malchezaar
          ( 272172, 5 ),	# Shivarra - Multi-Slash
          ( 272435, 5 ),	# Darkhound - Fel Bite
          ( 272167, 5 ),	# Bilescourge - Toxic Bile
          ( 272439, 5 ),	# Ur'zul - Many Faced Bite
          ( 272156, 5 ),	# Void Terror - Double Breath
          ( 272432, 5 ),	# Wrathguard - Overhead Assault
          ( 272013, 5 ),	# Vicious Hellhound - Demon Fangs
          ( 272012, 5 ),	# Illidari Satyr - Shadow Slash
          ( 272131, 5 ),	# Eye of Gul'dan - Eye of Gul'dan
          ( 267964, 0 ),	# new soul strike?
          ( 289367, 1 ),    # Pandemic Invocation Damage
          ( 265391, 3 ),    # Roaring Blaze Debuff
          ( 266087, 3 ),    # Rain of Chaos Buff
          ( 339784, 2 ),    # Tyrant's Soul Buff
          ( 337142, 2 )     # Grim Inquisitor's Dread Calling Buff
        ),

        # Monk:
        (
          # General
          # Brewmaster
          ( 195630, 1 ), # Brewmaster Mastery Buff
          ( 115129, 1 ), # Expel Harm Damage
          ( 124503, 0 ), # Gift of the Ox Orb Left
          ( 124506, 0 ), # Gift of the Ox Orb Right
          ( 178173, 0 ), # Gift of the Ox Explosion
          ( 124275, 1 ), # Light Stagger
          ( 124274, 1 ), # Medium Stagger
          ( 124273, 1 ), # Heavy Stagger
          ( 216521, 1 ), # Celestial Fortune Heal
          ( 227679, 1 ), # Face Palm
          ( 227291, 1 ), # Niuzao pet Stomp
          # Mistweaver
          ( 228649, 2 ), # Teachings of the Monastery - Blackout Proc
          ( 343820, 2 ), # Invoke Chi-Ji, the Red Crane - Enveloping Mist cast reduction
          # Windwalker
          ( 115057, 3 ), # Flying Serpent Kick Movement spell
          ( 116768, 3 ), # Combo Breaker: Blackout Kick
          ( 121283, 0 ), # Chi Sphere from Power Strikes
          ( 125174, 3 ), # Touch of Karma redirect buff
          ( 195651, 3 ), # Crosswinds Artifact trait trigger spell
          ( 196061, 3 ), # Crosswinds Artifact trait damage spell
          ( 196742, 3 ), # Whirling Dragon Punch Buff
          ( 211432, 3 ), # Tier 19 4-piece DPS Buff
          ( 220358, 3 ), # Cyclone Strikes info
          ( 228287, 3 ), # Spinning Crane Kick's Mark of the Crane debuff
          ( 240672, 3 ), # Master of Combinations Artifact trait buff
          ( 242387, 3 ), # Thunderfist Artifact trait buff
          ( 252768, 3 ), # Tier 21 2-piece DPS effect
          ( 261682, 3 ), # Chi Burst Chi generation cap
          ( 285594, 3 ), # Good Karma Healing Spell
		      ( 290461, 3 ), # Reverse Harm Damage
          # Azerite Traits
          ( 278710, 3 ), # Swift Roundhouse
          ( 278767, 1 ), # Training of Niuzao buff
          ( 285958, 1 ), # Straight, No Chaser trait
          ( 285959, 1 ), # Straight, No Chaser buff
          ( 286585, 3 ), # Dance of Chi-Ji trait
          ( 286586, 3 ), # Dance of Chi-Ji RPPM
          ( 286587, 3 ), # Dance of Chi-Ji buff
          ( 287055, 3 ), # Fury of Xuen trait
          ( 287062, 3 ), # Fury of Xuen buff
          ( 287063, 3 ), # Fury of Xuen proc
          ( 287831, 2 ), # Secret Infusion Crit Buff
          ( 287835, 2 ), # Secret Infusion Haste Buff
          ( 287836, 2 ), # Secret Infusion Mastery Buff
          ( 287837, 2 ), # Secret Infusion Versatility Buff
          ( 288634, 3 ), # Glory of the Dawn trait
          ( 288636, 3 ), # Glory of the Dawn proc
          # Conduits
          ( 336874, 0 ), # Fortifying Ingredients
          # Shadowland Legendaries
          ( 338141, 1 ), # Flaming Kicks Legendary damage
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
          ( 157228, 1 ),       # Owlkin Frenzy
          ( 185321, 3 ),       # Stalwart Guardian buff (T18 trinket)
          ( 188046, 5 ),       # T18 2P Faerie casts this spell
          ( 202461, 1 ),       # Stellar Drift mobility buff
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
          ( 252750, 2 ),       # Feral tier21_2pc
          ( 272873, 1 ),       # Streaking Star (azerite) damage spell
          ( 274282, 1 ),       # Half Moon
          ( 274283, 1 ),       # Full Moon
          ( 279641, 1 ),       # Lunar Shrapnel Azerite Trait
          ( 279729, 1 ),       # Solar Empowerment
          ( 276026, 2 ),       # Iron Jaws, Feral Azerite Power
          ( 289314, 3 ),       # Burst of Savagery Azerite Trait
          ( 289315, 3 ),       # Burst of Savagery Azerite Trait buff
          ( 305497, 0 ), ( 305496, 0 ), # Thorns pvp talent

          # Shadowlands Legendary
          ( 338825, 1 ),       # Primordial Arcanic Pulsar buff
          ( 339943, 1 ),       # Runecarve #3 Nature crit buff
          ( 339946, 1 ),       # Runecarve #3 Arcane crit buff
          ( 339797, 1 ),       # Oneth's Clear Vision (free starsurge)
          ( 339800, 1 ),       # Oneth's Perception (free starfall)

          # Shadowlands Covenant
          ( 326446, 0, True ), # Kyrian Empower Bond on DPS
          ( 326462, 0, True ), # Kyrian Empower Bond on Tank
          ( 326647, 0, True ), # Kyrian Empower Bond on Healer
          ( 338142, 0, True ), # Kyrian Lone Empowerment

          ( 340720, 1 ), ( 340708, 1 ), ( 340706, 1 ), ( 340719, 1 ), # balance potency conduits
          ( 340682, 2 ), ( 340688, 2 ), ( 340694, 2 ), ( 340705, 2 ), # feral potency conduits
          ( 340552, 3 ), ( 340609, 3 ), # guardian potency conduits
          ( 341378, 0 ), ( 341447, 0 ), ( 341446, 0 ), ( 341383, 0 ), # convenant potency conduits
        ),
        # Demon Hunter:
        (
          # General
          ( 225102, 0 ), # Fel Eruption damage

          # Havoc
          ( 236167, 1 ), # Felblade proc rate
          ( 208605, 1 ), # Nemesis player buff
          ( 203796, 1 ), # Demon Blade proc
          ( 217070, 1 ), # Rage of the Illidari explosion
          ( 217060, 1 ), # Rage of the Illidari buff
          ( 202446, 1 ), # Anguish damage spell
          ( 222703, 1 ), # Fel Barrage proc rate
          ( 247938, 1 ), # Chaos Blades primary spell, still used by Chaos Theory
          ( 211796, 1 ), # Chaos Blades MH damage spell
          ( 211797, 1 ), # Chaos Blades OH damage spell
          ( 275148, 1 ), # Unbound Chaos Azerite damage spell
          ( 337313, 1 ), # Inner Demon aura for Unbound Chaos talent
          ( 275147, 1 ), # Unbound Chaos delayed trigger aura
          ( 333105, 1 ), # Sigil of the Illidari Legendary fake Fel Eruption aura
          ( 333110, 1 ), # Sigil of the Illidari Legendary fake Fel Eruption damage trigger
          ( 333120, 1 ), # Sigil of the Illidari Legendary fake Fel Eruption heal
          ( 339229, 0 ), # Serrated Glaive conduit debuff

          # Vengeance
          ( 203557, 2 ), # Felblade proc rate
          ( 209245, 2 ), # Fiery Brand damage reduction
          ( 213011, 2 ), # Charred Warblades heal
          ( 212818, 2 ), # Fiery Demise debuff
          ( 207760, 2 ), # Burning Alive spread radius
          ( 333386, 2 ), # Sigil of the Illidari Legendary fake Eye Beam aura
          ( 333389, 2 ), # Sigil of the Illidari Legendary fake Eye Beam damage trigger
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
        ( 2597, ),               # Dark Iron Dwarf 0x0800
        ( 2775, ),               # Vulpera 0x1000
        ( 2598, ),               # Mag'har Orc 0x2000
        ( 2774, ),               # Mechagnome 0x4000
        (),                      # Vrykul
        (),                      # Tuskarr
        (),                      # Forest Troll
        (),                      # Taunka
        (),                      # Northrend Skeleton
        (),                      # Ice Troll
        ( 789, ),                # Worgen   0x200000
        (),                      # Gilnean
        (),
        ( 899, ),                # Pandaren 0x1000000
        (),
        ( 2419, ),               # Nightborne 0x4000000
        ( 2420, ),               # Highmountain Tauren 0x8000000
        ( 2423, ),               # Void Elf 0x10000000
        ( 2421, ),               # Lightforged Draenei 0x20000000
        ( 2721, ),               # Zandalari Troll 0x40000000
        ( 2723, ),               # Kul Tiran 0x80000000
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
        131,    # Play sound
        162,    # Select specialization
        166,    # Grant currency
        198,    # Play a scene
        205,    # Quest choice stuff
        219,    # Conversiation stuff
        223,    # Various item bonus grants
        225,    # Battle pet level grant
        231,    # Follower experience grant
        238,    # Increase skill
        240,    # Artifact experience grant
        242,    # Artifact experience grant
        245,    # Upgrade heirloom
        247,    # Add garrison mission
        248,    # Rush orders for garrisons
        249,    # Force equip an item
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
        200,    # Exp gain from kills
        206,    # SPELL_AURA_MOD_FLIGHT_SPEED_xx begin
        207,
        208,
        209,
        210,
        211,
        212,    # SPELL_AURA_MOD_FLIGHT_SPEED_xx ends
        244,    # Language stuff
        260,    # Screen effects
        261,    # Phasing
        291,    # Exp gain from quests
        430,    # Play scene
    ]

    _mechanic_blacklist = [
        21,     # MECHANIC_MOUNT
    ]

    # Effect subtype, field name
    _label_whitelist = [
        ( 218, 'misc_value_2' ),
        ( 219, 'misc_value_2' ),
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
        221477, # Underlight (from Underlight Angler - Legion artifact fishing pole)
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

    def __init__(self, options, data_store):
        super().__init__(options, data_store)

        self._dbc = [ 'Spell', 'SpellEffect', 'SpellScaling', 'SpellCooldowns', 'SpellRange',
                'SpellClassOptions', 'SpellDuration', 'SpellPower', 'SpellLevels',
                'SpellCategories', 'SpellCategory', 'Talent', 'SkillLineAbility',
                'SpellAuraOptions', 'SpellRadius', 'GlyphProperties', 'Item',
                'SpellCastTimes', 'ItemSet', 'SpellDescriptionVariables', 'SpellItemEnchantment',
                'SpellEquippedItems', 'SpecializationSpells', 'ChrSpecialization',
                'SpellMisc', 'SpellProcsPerMinute', 'ItemSetSpell',
                'ItemEffect', 'MinorTalent', 'SpellShapeshift', 'SpellMechanic', 'SpellLabel',
                'AzeritePower', 'AzeritePowerSetMember', 'SpellName' ]

        if self._options.build < 25600:
            self._dbc.append('SpellEffectScaling')

        if self._options.build < 23436:
            self._dbc.append('Item-sparse')
        else:
            self._dbc.append('ItemSparse')

        if self._options.build >= 24651:
            self._dbc.append('RelicTalent')

        if self._options.build >= 25600:
            self._dbc.append('SpellXDescriptionVariables')

        if self._options.build >= dbc.WowVersion(8, 2, 0, 30080):
            self._dbc.append('AzeriteItemMilestonePower')
            self._dbc.append('AzeriteEssencePower')
            self._dbc.append('AzeriteEssence')

        if self._options.build >= dbc.WowVersion(8, 3, 0, 0):
            self._dbc.append('ItemBonus')

    def initialize(self):
        if not super().initialize():
            return False

        self._data_store.link('SpellEffect',        self._options.build < 25600 and 'id_spell' or 'id_parent', 'SpellName', 'add_effect' )
        self._data_store.link('SpellPower',         self._options.build < 25600 and 'id_spell' or 'id_parent', 'SpellName', 'power'      )
        self._data_store.link('SpellCategories',    self._options.build < 25600 and 'id_spell' or 'id_parent', 'SpellName', 'categories' )
        self._data_store.link('SpellLevels',        self._options.build < 25600 and 'id_spell' or 'id_parent', 'SpellName', 'level'      )
        self._data_store.link('SpellCooldowns',     self._options.build < 25600 and 'id_spell' or 'id_parent', 'SpellName', 'cooldown'   )
        self._data_store.link('SpellAuraOptions',   self._options.build < 25600 and 'id_spell' or 'id_parent', 'SpellName', 'aura_option')
        self._data_store.link('SpellLabel',         self._options.build < 25600 and 'id_spell' or 'id_parent', 'SpellName', 'label'      )

        self._data_store.link('SpellScaling',       'id_spell', 'SpellName', 'scaling'       )
        self._data_store.link('SpellEquippedItems', 'id_spell', 'SpellName', 'equipped_item' )
        self._data_store.link('SpellClassOptions',  'id_spell', 'SpellName', 'class_option'  )
        self._data_store.link('SpellShapeshift',    'id_spell', 'SpellName', 'shapeshift'    )
        self._data_store.link('Spell',              'id',       'SpellName', 'text'          )
        self._data_store.link('AzeritePower',       'id_spell', 'SpellName', 'azerite_power' )

        if self._options.build >= 25600:
            self._data_store.link('SpellMisc',                  'id_parent', 'SpellName', 'misc'         )
            self._data_store.link('SpellXDescriptionVariables', 'id_spell',  'SpellName', 'desc_var_link')

        if self._options.build < 25600:
            self._data_store.link('SpellEffectScaling', 'id_effect', 'SpellEffect', 'scaling')

        if self._options.build >= dbc.WowVersion(8, 2, 0, 30080):
            self._data_store.link('AzeriteEssencePower', 'id_power', 'AzeriteEssence', 'powers')
            self._data_store.link('AzeriteEssencePower', 'id_spell_major_base', 'SpellName', 'azerite_essence')
            self._data_store.link('AzeriteEssencePower', 'id_spell_minor_base', 'SpellName', 'azerite_essence')
            self._data_store.link('AzeriteEssencePower', 'id_spell_major_upgrade', 'SpellName', 'azerite_essence')
            self._data_store.link('AzeriteEssencePower', 'id_spell_minor_upgrade', 'SpellName', 'azerite_essence')

        self._data_store.link('ItemSetSpell', 'id_item_set', 'ItemSet', 'bonus')

        self._data_store.link('ItemEffect', self._options.build < 25600 and 'id_item' or 'id_parent', self._options.build < 23436 and 'Item-sparse' or 'ItemSparse', 'spells')
        return True

    def class_mask_by_skill(self, skill):
        for i in range(0, len(constants.CLASS_SKILL_CATEGORIES)):
            if constants.CLASS_SKILL_CATEGORIES[i] == skill:
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
        return util.race_mask(skill = skill)

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

        # Check for blacklisted spells
        if spell.id in SpellDataGenerator._spell_blacklist:
            logging.debug("Spell id %u (%s) is blacklisted", spell.id, spell_name)
            return False

        # Check for spell name blacklist
        if spell_name:
            if util.is_blacklisted(spell_name = spell_name):
                logging.debug("Spell id %u (%s) is blacklisted", spell.id, spell_name)
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
        spell = self._spellname_db[spell_id]
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

            # Treat 'Override Action Spell' values as trigger spells for generation
            if effect.type == 6 and effect.sub_type == 332:
                trigger_spell = effect.base_value
            else:
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

        spell_text = self._spell_db[spell_id]
        spell_refs = re.findall(SpellDataGenerator._spell_ref_rx, spell_text.desc or '')
        spell_refs += re.findall(SpellDataGenerator._spell_ref_rx, spell_text.tt or '')
        if self._options.build < 25600:
            desc_var = spell.id_desc_var
            if desc_var > 0:
                data = self._spelldescriptionvariables_db[desc_var]
                if data.id > 0:
                    spell_refs += re.findall(SpellDataGenerator._spell_ref_rx, data.desc)
        else:
            link = spell.get_link('desc_var_link')
            data = self._spelldescriptionvariables_db[link.id_desc_var]
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
            if self._options.build < 25600:
                spell_id = perk_data.id_spell
            else:
                spell_id = perk_data.id_parent
            if spell_id == 0:
                continue

            spec_data = self._chrspecialization_db[perk_data.id_parent]
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
            spell = self._spellname_db[spell_id]
            if not spell.id:
                continue

            self.process_spell(spell_id, ids, ability_data.mask_class or mask_class_category, ability_data.mask_race or mask_race_category)

        # Get specialization skills from SpecializationSpells and masteries from ChrSpecializations
        for spec_id, spec_spell_data in self._specializationspells_db.items():
            # Guess class based on specialization category identifier
            spec_data = self._chrspecialization_db[spec_spell_data.spec_id]
            if spec_data.id == 0:
                continue

            if spec_data.name == 'Initial':
                continue

            spell_id = spec_spell_data.spell_id
            spell = self._spellname_db[spell_id]
            if not spell.id:
                continue

            mask_class = 0
            if spec_data.class_id > 0:
                mask_class = DataGenerator._class_masks[spec_data.class_id]
            # Hunter pet classes have a class id of 0, tag them as "hunter spells" like before
            else:
                mask_class = DataGenerator._class_masks[3]

            self.process_spell(spell_id, ids, mask_class, 0, False)

        for spec_id, spec_data in self._chrspecialization_db.items():
            s = self._spellname_db[spec_data.id_mastery_1]
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

            spell = self._spellname_db[ability_data.id_spell]
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
                    item = self._options.build < 23436 and self._item_sparse_db[effect.item_type] or self._itemsparse_db[effect.item_type]
                    if not item.id or item.gem_props == 0:
                        continue

                    for spell in item.get_links('spells'):
                        id_spell = spell.id_spell
                        enchant_spell = self._spellname_db[id_spell]
                        for enchant_effect in enchant_spell._effects:
                            if not enchant_effect or (enchant_effect.type != 53 and enchant_effect.type != 6):
                                continue

                            enchant_spell_id = id_spell
                            break

                        if enchant_spell_id > 0:
                            break
                elif effect.type == 53:
                    spell_item_ench = self._spellitemenchantment_db[effect.misc_value_1]
                    #if (spell_item_ench.req_skill == 0 and self._spelllevels_db[spell.id_levels].base_level < 60) or \
                    #   (spell_item_ench.req_skill > 0 and spell_item_ench.req_skill_value <= 375):
                    #    continue
                    enchant_spell_id = spell.id

                if enchant_spell_id > 0:
                    break

            # Valid enchant, process it
            if enchant_spell_id > 0:
                self.process_spell(enchant_spell_id, ids, 0, 0)

        item_db = self._options.build < 23436 and self._item_sparse_db or self._itemsparse_db
        # Rest of the Item enchants relevant to us, such as Shoulder / Head enchants
        for item_id, data in item_db.items():
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

                    spell = self._spellname_db[spell_id]
                    for effect in spell._effects:
                        if not effect or effect.type != 53:
                            continue

                        self.process_spell(spell_id, ids, 0, 0)
            # Grab relevant spells from consumables as well
            elif class_ == 0:
                for item_effect in data.get_links('spells'):
                    spell_id = item_effect.id_spell
                    spell = self._spellname_db[spell_id]
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

                    # Finally, check consumable whitelist
                    map_ = constants.CONSUMABLE_ITEM_WHITELIST.get(classdata.subclass, {})
                    if item_effect.id_parent in map_:
                        self.process_spell(spell_id, ids, 0, 0)

            # Hunter scopes and whatnot
            elif class_ == 7:
                if classdata.has_value('subclass', 3):
                    for item_effect in data.get_links('spells'):
                        spell_id = item_effect.id_spell
                        spell = self._spellname_db[spell_id]
                        for effect in spell._effects:
                            if not effect:
                                continue

                            if effect.type == 53:
                                self.process_spell(spell_id, ids, 0, 0)

        # Relevant set bonuses
        for id, set_spell_data in self._itemsetspell_db.items():
            set_id = self._options.build < 25600 and set_spell_data.id_item_set or set_spell_data.id_parent
            if not SetBonusListGenerator.is_extract_set_bonus(set_id)[0]:
                continue

            self.process_spell(set_spell_data.id_spell, ids, 0, 0)

        # Glyph effects, need to do trickery here to get actual effect from spellbook data
        for ability_id, ability_data in self._skilllineability_db.items():
            if ability_data.id_skill != 810 or not ability_data.mask_class:
                continue

            use_glyph_spell = self._spellname_db[ability_data.id_spell]
            if not use_glyph_spell.id:
                continue

            # Find the on-use for glyph then, misc value will contain the correct GlyphProperties.dbc id
            for effect in use_glyph_spell._effects:
                if not effect or effect.type != 74: # Use glyph
                    continue

                # Filter some erroneous glyph data out
                glyph_data = self._glyphproperties_db[effect.misc_value_1]
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
        for iid, data in item_db.items():
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

        for _, data in self._azeritepowersetmember_db.items():
            power = self._azeritepower_db[data.id_power]
            if power.id != data.id_power:
                continue

            spell_id = power.id_spell
            spell = self._spellname_db[spell_id]
            if spell.id != spell_id:
                continue

            self.process_spell(spell_id, ids, 0, 0, False)
            if spell_id in ids:
                mask_class = self._class_masks[data.class_id] or 0
                ids[spell_id]['mask_class'] |= mask_class

        # Azerite esssence spells
        if self._options.build >= dbc.WowVersion(8, 2, 0, 30080):
            for _, data in self._azeriteitemmilestonepower_db.items():
                power = self._azeritepower_db[data.id_power]
                if power.id != data.id_power:
                    continue

                spell_id = power.id_spell
                spell = self._spellname_db[spell_id]
                if spell.id != spell_id:
                    continue

                self.process_spell(spell_id, ids, 0, 0, False)

            for _, data in self._azeriteessencepower_db.items():
                self.process_spell(data.id_spell_major_base, ids, 0, 0, False)
                self.process_spell(data.id_spell_major_upgrade, ids, 0, 0, False)
                self.process_spell(data.id_spell_minor_base, ids, 0, 0, False)
                self.process_spell(data.id_spell_minor_upgrade, ids, 0, 0, False)

        # Get dynamic item effect spells
        if self._options.build >= dbc.WowVersion(8, 3, 0, 0):
            for _, data in self._itembonus_db.items():
                if data.type != 23:
                    continue

                effect = self._itemeffect_db[data.val_1]
                if effect.id == 0:
                    continue

                self.process_spell(effect.id_spell, ids, 0, 0, False)

        # Soulbind trees abilities
        for _, entry in self.db('Soulbind').items():
            for talent in entry.ref('id_garr_talent_tree').child_refs('GarrTalent'):
                for rank in talent.children('GarrTalentRank'):
                    if rank.ref('id_spell').id == rank.id_spell:
                        self.process_spell(rank.id_spell, ids, 0, 0)

        # Covenant abilities
        for entry in self.db('UICovenantAbility').values():
            if entry.id_spell == 0 or entry.ref('id_spell').id != entry.id_spell:
                continue

            self.process_spell(entry.id_spell, ids, 0, 0)

        # Souldbind conduits
        for _, entry in self.db('SoulbindConduit').items():
            for spell_id in set(rank.id_spell for rank in entry.children('SoulbindConduitRank')):
                if self.db('SpellName')[spell_id].id == spell_id:
                    self.process_spell(spell_id, ids, 0, 0)

        # Last, get the explicitly defined spells in _spell_id_list on a class basis and the
        # generic spells from SpellDataGenerator._spell_id_list[0]
        for generic_spell_id in SpellDataGenerator._spell_id_list[0]:
            if generic_spell_id in ids.keys():
                spell = self._spellname_db[generic_spell_id]
                logging.debug('Whitelisted spell id %u (%s) already in the list of spells to be extracted',
                    generic_spell_id, spell.name)
            self.process_spell(generic_spell_id, ids, 0, 0)

        for cls in range(1, len(SpellDataGenerator._spell_id_list)):
            for spell_tuple in SpellDataGenerator._spell_id_list[cls]:
                if len(spell_tuple) == 2 and spell_tuple[0] in ids.keys():
                    spell = self._spellname_db[spell_tuple[0]]
                    logging.debug('Whitelisted spell id %u (%s) already in the list of spells to be extracted',
                        spell_tuple[0], spell.name)
                self.process_spell(spell_tuple[0], ids, self._class_masks[cls], 0)

        for spell_id, spell_data in self._spellname_db.items():
            for pattern in SpellDataGenerator._spell_name_whitelist:
                if pattern.match(spell_data.name):
                    self.process_spell(spell_id, ids, 0, 0)

        # After normal spells have been fetched, go through all spell ids,
        # and get all the relevant aura_ids for selected spells
        more_ids = { }
        for spell_id, spell_data in ids.items():
            spell = self._spellname_db[spell_id]
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
        powers = []
        # Whitelist labels separately, based on effect types, otherwise we get far too many
        included_labels = set()

        spelldata_array_position = {} # spell id -> array position
        spelleffect_index = {}
        spelllabel_index = defaultdict(int)
        spelldriver_index = defaultdict(set)

        # Hotfix data for spells, effects, powers
        spell_hotfix_data = HotfixDataGenerator('spell')
        effect_hotfix_data = HotfixDataGenerator('effect')
        power_hotfix_data = HotfixDataGenerator('power')

        for id in id_keys:
            spell = self.db('SpellName')[id]

            effect_ids = {} # effect index -> effect id
            for effect in spell._effects:
                if effect and ids.get(id, { 'effect_list': [ False ] })['effect_list'][effect.index]:
                    effect_ids[effect.index] = effect.id

                    # Check if we need to grab a specific label number for the effect
                    for sub_type, field in SpellDataGenerator._label_whitelist:
                        if effect.sub_type == sub_type:
                            included_labels.add(getattr(effect, field))

            # Effect lists can have "holes", this effectively plugs them with an "invalid" effect id
            # and produces a list of effect ids
            if len(effect_ids):
                spelleffect_index[ id ] = [ effect_ids.get(i, 0) for i in range(max(effect_ids.keys()) + 1) ]

        labels = []
        for label_id, label_data in self.db('SpellLabel').items():
            if label_data.label not in included_labels:
                continue
            if label_data.id_parent not in id_keys:
                continue
            if label_data.label in constants.SPELL_LABEL_BLACKLIST:
                continue
            labels.append(label_data)
            spelllabel_index[label_data.id_parent] += 1

        for spell_id, effect_ids in spelleffect_index.items():
            for effect_id in effect_ids:
                effect = self.db('SpellEffect')[effect_id]
                if effect.trigger_spell > 0 and effect.trigger_spell in id_keys and spell_id in id_keys:
                    spelldriver_index[effect.trigger_spell].add(spell_id)

        self._out.write('// {} spells, wow build level {}\n'.format( len(ids), self._options.build ))
        self._out.write('static spell_data_t __{}_data[{}] = {{\n'.format(
            self.format_str('spell'), len(ids) ))

        index = 0
        for id in id_keys:
            spell = self._spellname_db[id]
            hotfix = HotfixDataRecord()
            power_count = 0

            if not spell.id and id > 0:
                sys.stderr.write('Spell id %d not found\n') % id
                continue

            spelldata_array_position[spell.id] = index

            for power in spell.children('SpellPower'):
                power_count += 1
                powers.append( power )

            if self._options.build < 25600:
                misc = self._spellmisc_db[spell.id_misc]
            else:
                misc = spell.get_link('misc')

            #if index % 20 == 0:
            #  self._out.write('//{ Name                                ,     Id,             Hotfix,PrjSp,  Sch, Class,  Race,Sca,MSL,SpLv,MxL,MinRange,MaxRange,Cooldown,  GCD,Chg, ChrgCd, Cat,  Duration,  RCost, RPG,Stac, PCh,PCr, ProcFlags,EqpCl, EqpInvType,EqpSubclass,CastMn,CastMx,Div,       Scaling,SLv, RplcId, {      Attr1,      Attr2,      Attr3,      Attr4,      Attr5,      Attr6,      Attr7,      Attr8,      Attr9,     Attr10,     Attr11,     Attr12 }, {     Flags1,     Flags2,     Flags3,     Flags4 }, Family, Description, Tooltip, Description Variable, Icon, ActiveIcon, Effect1, Effect2, Effect3 },\n')

            sname = self._spellname_db[id]
            fields = sname.field('name', 'id')
            hotfix.add(sname, ('name', 0))

            fields += misc.field('school', 'proj_speed')
            hotfix.add(misc, ('proj_speed', 3), ('school', 4))

            # Hack in the combined class from the id_tuples dict
            fields += [ u'%#.16x' % ids.get(id, { 'mask_class' : 0, 'mask_race': 0 })['mask_race'] ]
            fields += [ u'%#.8x' % ids.get(id, { 'mask_class' : 0, 'mask_race': 0 })['mask_class'] ]

            # Set the scaling index for the spell
            fields += spell.get_link('scaling').field('id_class', 'max_scaling_level')
            hotfix.add(spell.get_link('scaling'), ('id_class', 7), ('max_scaling_level', 8))

            fields += spell.get_link('level').field('base_level', 'max_level', 'req_max_level')
            hotfix.add(spell.get_link('level'), ('base_level', 9), ('max_level', 10), ('req_max_level', 46))

            range_entry = self._spellrange_db[misc.id_range]
            fields += range_entry.field('min_range_1', 'max_range_1')
            hotfix.add(range_entry, ('min_range_1', 11), ('max_range_1', 12))

            fields += spell.get_link('cooldown').field('cooldown_duration', 'gcd_cooldown', 'category_cooldown')
            hotfix.add(spell.get_link('cooldown'), ('cooldown_duration', 13), ('gcd_cooldown', 14), ('category_cooldown', 15))

            category = spell.get_link('categories')
            category_data = self._spellcategory_db[category.charge_category]

            fields += category_data.field('charges', 'charge_cooldown')
            hotfix.add(category_data, ('charges', 16), ('charge_cooldown', 17))
            if category.charge_category > 0: # Note, some spells have both cooldown and charge categories
                fields += category.field('charge_category')
                hotfix.add(category, ('charge_category', 18))
            else:
                fields += category.field('cooldown_category')
                hotfix.add(category, ('cooldown_category', 18))
            fields += category.field('dmg_class')
            hotfix.add(category, ('dmg_class', 47))

            fields += spell.child('SpellTargetRestrictions').field('max_affected_targets')
            hotfix.add(spell.child('SpellTargetRestrictions'), ('max_affected_targets', 48))

            duration_entry = self._spellduration_db[misc.id_duration]
            fields += duration_entry.field('duration_1')
            hotfix.add(duration_entry, ('duration_1', 19))

            fields += spell.get_link('aura_option').field('stack_amount', 'proc_chance', 'proc_charges', 'proc_flags_1', 'internal_cooldown')
            hotfix.add(spell.get_link('aura_option'),
                    ('stack_amount', 20), ('proc_chance', 21), ('proc_charges', 22),
                    ('proc_flags_1', 23), ('internal_cooldown', 24))

            ppm_entry = self._spellprocsperminute_db[spell.get_link('aura_option').id_ppm]
            fields += ppm_entry.field('ppm')
            hotfix.add(ppm_entry, ('ppm', 25))

            fields += spell.get_link('equipped_item').field('item_class', 'mask_inv_type', 'mask_sub_class')
            hotfix.add(spell.get_link('equipped_item'),
                ('item_class', 26), ('mask_inv_type', 27), ('mask_sub_class', 28))

            fields += misc.ref('id_cast_time').field('cast_time')
            hotfix.add(misc.ref('id_cast_time'), ('cast_time', 30))

            # Add spell flags
            fields += [ '{ %s }' % ', '.join(misc.field('flags_1', 'flags_2', 'flags_3', 'flags_4',
                'flags_5', 'flags_6', 'flags_7', 'flags_8', 'flags_9', 'flags_10', 'flags_11',
                'flags_12', 'flags_13', 'flags_14', 'flags_15')) ]
            # Note, bunch up the flags checking into one field,
            hotfix.add(misc,
                (('flags_1', 'flags_2', 'flags_3',  'flags_4',  'flags_5',  'flags_6',  'flags_7',
                  'flags_8', 'flags_9', 'flags_10', 'flags_11', 'flags_12', 'flags_13', 'flags_14', 'flags_15'), 35))

            fields += [ '{ %s }' % ', '.join(spell.get_link('class_option').field('flags_1', 'flags_2', 'flags_3', 'flags_4')) ]
            fields += spell.get_link('class_option').field('family')
            hotfix.add(spell.get_link('class_option'),
                (('flags_1', 'flags_2', 'flags_3', 'flags_4'), 36), ('family', 37))

            fields += spell.get_link('shapeshift').field('flags_1')
            hotfix.add(spell.get_link('shapeshift'), ('flags_1', 38))

            mechanic = self._spellmechanic_db[spell.get_link('categories').mechanic]
            fields += mechanic.field('mechanic')
            hotfix.add(mechanic, ('mechanic', 39))

            power = spell.get_link('azerite_power')
            fields += power.field('id')

            # 8.2.0 Azerite essence stuff
            if self._options.build >= dbc.WowVersion(8, 2, 0, 30080):
                essences = [x.field('id_essence')[0] for x in spell.get_links('azerite_essence')]
                if len(essences) == 0:
                    fields += self._azeriteessence_db[0].field('id')
                else:
                    essences = list(set(essences))
                    if len(essences) > 1:
                        logging.warn('Spell %s (id=%d) associated with more than one Azerite Essence (%s)',
                            spell.name, spell.id, ', '.join(essences))

                    fields.append(essences[0])
            else:
                fields.append('0')

            # Pad struct with empty pointers for direct access to spell effect data
            fields += [ u'0', u'0', u'0', u'0' ]

            effect_ids = spelleffect_index.get(id, ())

            fields += [ str(len(effect_ids)) ]
            fields += [ str(power_count) ]
            fields += [ str(len(spelldriver_index.get(id, ()))) ]
            fields += [ str(spelllabel_index.get(id, 0)) ]

            # Finally, collect spell hotfix data if it exists
            if spell._flags == -1:
                hotfix.new_entry = True
            spell_hotfix_data.add(spell.id, hotfix)

            try:
                self._out.write('  { %s }, /* %s */\n' % (', '.join(fields), ', '.join('%u' % id for id in effect_ids)))
            except Exception as e:
                sys.stderr.write('Error: %s\n%s\n' % (e, fields))
                sys.exit(1)

            index += 1

        self._out.write('};\n\n')

        # Write out spell effects
        effects = []
        for _, ids in sorted(spelleffect_index.items()):
            effects.extend(ids)
        spelleffect_id_index = []

        self._out.write('// {} effects, wow build level {}\n'.format( len(effects), self._options.build ))
        self._out.write('static spelleffect_data_t __{}_data[{}] = {{\n'.format(
            self.format_str('spelleffect'), len(effects) ))

        for index, effect_id in enumerate(effects):
            effect = self.db('SpellEffect')[effect_id]

            if effect_id != 0:
                spelleffect_id_index.append((effect.id, index))

            #if index % 20 == 0:
            #    self._out.write('//{     Id,Flags,   SpId,Idx, EffectType                  , EffectSubType                              ,       Coefficient,         Delta,       Unknown,   Coefficient, APCoefficient,  Ampl,  Radius,  RadMax,   BaseV,   MiscV,  MiscV2, {     Flags1,     Flags2,     Flags3,     Flags4 }, Trigg,   DmgMul,  CboP, RealP,Die,Mech,ChTrg,0, 0 },\n')

            hotfix = HotfixDataRecord()

            fields = effect.field('id', 'id_parent', 'index', 'type', 'sub_type', 'coefficient', 'delta',
                                  'bonus', 'sp_coefficient', 'ap_coefficient', 'amplitude')
            hotfix.add(effect, ('index', 3), ('type', 4), ('sub_type', 5), ('coefficient', 6), ('delta', 7),
                               ('bonus', 8), ('sp_coefficient', 9), ('ap_coefficient', 10), ('amplitude', 11))

            radius_entry = self._spellradius_db[effect.id_radius_1]
            fields += radius_entry.field('radius_1')
            hotfix.add(radius_entry, ('radius_1', 12))

            radius_max_entry = self._spellradius_db[effect.id_radius_2]
            fields += radius_max_entry.field('radius_1')
            hotfix.add(radius_max_entry, ('radius_1', 13))

            fields += effect.field('base_value', 'misc_value_1', 'misc_value_2')
            hotfix.add(effect, ('base_value', 14), ('misc_value_1', 15), ('misc_value_2', 16))

            # note hotfix flags bunched together into one bit field
            fields += [ '{ %s }' % ', '.join( effect.field('class_mask_1', 'class_mask_2', 'class_mask_3', 'class_mask_4' ) ) ]
            hotfix.add(effect, (('class_mask_1', 'class_mask_2', 'class_mask_3', 'class_mask_4'), 17))

            fields += effect.field('trigger_spell', 'dmg_multiplier', 'points_per_combo_points', 'real_ppl')
            hotfix.add(effect, ('trigger_spell', 18), ('dmg_multiplier', 19),
                ('points_per_combo_points', 20), ('real_ppl', 21))

            mechanic = self._spellmechanic_db[effect.id_mechanic]
            fields += mechanic.field('mechanic')
            hotfix.add(mechanic, ('mechanic', 22))

            fields += effect.field('chain_target', 'implicit_target_1', 'implicit_target_2', 'val_mul', 'pvp_coefficient')
            hotfix.add(effect, ('chain_target', 23), ('implicit_target_1', 24),
                ('implicit_target_2', 25), ('val_mul', 26), ('pvp_coefficient', 27))

            # Pad struct with empty pointers for direct spell data access
            fields += [ u'0', u'0' ]

            # Finally, collect effect hotfix data if it exists
            if effect._flags == -1:
                hotfix.new_entry = True
            effect_hotfix_data.add(effect.id, hotfix)

            try:
                self._out.write('  { %s },\n' % (', '.join(fields)))
            except:
                sys.stderr.write('%s\n' % fields)
                sys.exit(1)

        self._out.write('};\n\n')

        self.output_id_index(
                index = [ e[1] for e in sorted(spelleffect_id_index, key = lambda e: e[0]) ],
                array = 'spelleffect')

        # Write out spell powers
        spellpower_id_index = []

        self.output_header(
                header = 'Spell powers',
                type = 'spellpower_data_t',
                array = 'spellpower',
                length = len(powers))

        for index, power in enumerate(sorted(powers, key=lambda p: (p.id_parent, p.id))):
            hotfix = HotfixDataRecord()

            spellpower_id_index.append((power.id, index))

            fields = power.field('id', 'id_parent', 'aura_id', 'type_power',
                                 'cost', 'cost_max', 'cost_per_second',
                                 'pct_cost', 'pct_cost_max', 'pct_cost_per_second')
            hotfix.add(power, ('aura_id', 2), ('type_power', 4),
                              ('cost', 5), ('cost_max', 6), ('cost_per_second', 7),
                              ('pct_cost', 8), ('pct_cost_max', 9), ('pct_cost_per_second', 10))

            # Finally, collect powet hotfix data if it exists
            if power._flags == -1:
                hotfix.new_entry = True
            power_hotfix_data.add(power.id, hotfix)

            self.output_record(fields)

        self.output_footer()

        self.output_id_index(
                index = [ e[1] for e in sorted(spellpower_id_index, key = lambda e: e[0]) ],
                array = 'spellpower')

        # Write out labels
        self.output_header(
                header = 'Applied spell labels',
                type = 'spelllabel_data_t',
                array = 'spelllabel',
                length = len(labels))

        for label in sorted(labels, key=lambda l: (l.id_parent, l.id)):
            self.output_record(label.field('id', 'id_parent', 'label'))

        self.output_footer()

        # Then, write out hotfix data
        spell_hotfix_data.output(self)
        effect_hotfix_data.output(self)
        power_hotfix_data.output(self)

        # Then, write out pseudo-runtime linking data for spell drivers
        self._out.write('static constexpr std::array<const spell_data_t*, {}> __{}_index_data {{ {{\n'.format(
            sum(len(ids) for ids in spelldriver_index.values()), self.format_str('spelldriver') ))
        for _, ids in sorted(spelldriver_index.items()):
            self._out.write('  {},\n'.format(
                ', '.join('&__{}_data[{}]'.format(
                        self.format_str('spell'), spelldata_array_position[id]) for id in sorted(ids))))
        self._out.write('} };\n')

        return ''

class SpellTextGenerator(SpellDataGenerator):
    def filter(self):
        data = []

        for spell_id in sorted(super().filter().keys()):
            entry = self.db('Spell')[spell_id]
            # skip entries with "empty" text
            if entry.desc == 0 and entry.tt == 0 and entry.rank == 0:
                continue
            data.append(entry)

        return data

    def generate(self, data = None):
        hotfix_data = HotfixDataGenerator('spelltext')

        self.output_header(
                header = 'Spell text',
                type = 'spelltext_data_t',
                array = 'spelltext',
                length = len(data))

        for entry in data:
            self.output_record(entry.field( 'id', 'desc', 'tt', 'rank' ))
            hotfix_data.add_single(entry.id, entry, ('desc', 0), ('tt', 1), ('rank', 2))

        self.output_footer()
        hotfix_data.output(self)

class SpellDescVarGenerator(SpellDataGenerator):
    def filter(self):
        data = []

        for spell_id in sorted(super().filter().keys()):
            spell = self.db('SpellName')[spell_id]
            if spell.id != spell_id:
                continue

            # assume there can only be one ref
            links = spell.child_refs('SpellXDescriptionVariables')
            if len(links) > 0:
                link = links[0]
                if link.ref('id_desc_var').id == link.id_desc_var:
                    data.append(links[0])
            if len(links) > 1:
                logging.warning('Spell %s (id=%d) associated with more than one description variable (%d)',
                                spell.name, spell.id, len(links))

        return data

    def generate(self, data = None):
        hotfix_data = HotfixDataGenerator('spelldesc_vars')

        self.output_header(
                header = 'Spell description variables',
                type = 'spelldesc_vars_data_t',
                array = 'spelldesc_vars',
                length = len(data))

        for entry in sorted(data, key=lambda e: e.id_spell):
            desc_var = entry.ref('id_desc_var')

            fields = entry.field('id_spell')
            fields += desc_var.field('desc')
            self.output_record(fields)

            hotfix_data.add_single(entry.id_spell, desc_var, ('desc', 0))

        self.output_footer()
        hotfix_data.output(self)

class MasteryAbilityGenerator(DataGenerator):
    def filter(self):
        return MasterySpellSet(self._options).get()

    def generate(self, data=None):
        data.sort(key=lambda v: v.id)

        self.output_header(
                header = 'Mastery abilities',
                type = 'mastery_spell_entry_t',
                array = 'mastery_spell',
                length = len(data))

        for entry in data:
            fields = entry.field('id', 'id_mastery_1')
            self.output_record(fields,
                    comment = '{} ({} {})'.format(
                        entry.ref('id_mastery_1').name,
                        entry.name,
                        util.class_name(class_id=entry.class_id)))

        self.output_footer()

class RacialSpellGenerator(DataGenerator):
    def filter(self):
        return RacialSpellSet(self._options).get()

    def generate(self, data = None):
        data.sort(key = lambda v: (util.race_mask(skill = v.id_skill), v.ref('id_spell').name, -v.mask_class))

        self.output_header(
                header = 'Racial abilities',
                type = 'racial_spell_entry_t',
                array = 'racial_spell',
                length = len(data))

        for v in data:
            # Ensure that all racial spells have a race mask associated with
            # them. Currently, pandaren racial spells lack one.
            _mask = v.mask_race
            if _mask == 0:
                _mask = util.race_mask(skill = v.id_skill)

            fields = [v.field_format('mask_race')[0] % _mask]
            fields += v.field('mask_class')
            fields += v.ref('id_spell').field('id', 'name')
            self.output_record(fields)
        self.output_footer()

class SpecializationSpellGenerator(DataGenerator):
    def filter(self):
        spec_spells = []
        for ssid, data in self.db('SpecializationSpells').items():
            chrspec = data.ref('spec_id')
            if chrspec.id == 0:
                continue

            if chrspec.flags == 0:
                continue

            spell = data.ref('spell_id')
            if spell.id == 0:
                continue

            spec_tuple = (chrspec, data)
            if spec_tuple in spec_spells:
                continue

            spec_spells.append(spec_tuple)

        return spec_spells

    def generate(self, data = None):
        self.output_header(
                header = 'Talent tree specialization abilities',
                type = 'specialization_spell_entry_t',
                array = 'specialization_spell',
                length = len(data))

        for chrspec, spell_data in sorted(data, key = lambda e: (e[0].class_id, e[0].index)):
            spell = self.db('Spell').get(spell_data.spell_id)
            fields = chrspec.field('class_id', 'id')
            fields += spell_data.ref('spell_id').field('id')
            fields += spell_data.field('replace_spell_id')
            fields += spell_data.ref('spell_id').field('name')

            # Only export "Rank x" descriptions, so find_specialization_spell(
            # name, desc ) will match them, while find_specialization_spell(
            # name ) functions like before.
            if spell.rank == 0 or 'Rank' in spell.rank:
                fields += spell.field('rank')
            else:
                fields += ['0',]

            self.output_record(fields)

        self.output_footer()

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
        {
            'name'   : 'tier20',
            'bonuses': [ 1301, 1302, 1303, 1304, 1305, 1306, 1307, 1308, 1309, 1310, 1311, 1312 ],
            'tier'   : 20
        },
        {
            'name'   : 'tier21',
            'bonuses': [ 1319, 1320, 1321, 1322, 1323, 1324, 1325, 1326, 1327, 1328, 1329, 1330 ],
            'tier'   : 21
        },
        {
            'name'   : 'waycrests_legacy',
            'bonuses': [ 1439 ],
            'tier'   : 21
        },
        {
            'name'   : 'gift_of_the_loa',
            'bonuses': [ 1442 ],
            'tier'   : 23
        },
        {
            'name'   : 'keepsakes',
            'bonuses': [ 1443 ],
            'tier'   : 23
        },
		{
            'name'   : 'titanic_empowerment',
            'bonuses': [ 1452 ],
            'tier'   : 24
        }
    ]

    @staticmethod
    def is_extract_set_bonus(bonus):
        for idx in range(0, len(SetBonusListGenerator.set_bonus_map)):
            if bonus in SetBonusListGenerator.set_bonus_map[idx]['bonuses']:
                return True, idx

        return False, -1

    def filter(self):
        data = []
        for id, set_spell_data in self.db('ItemSetSpell').items():
            set_id = set_spell_data.id_parent
            is_set_bonus, set_index = SetBonusListGenerator.is_extract_set_bonus(set_id)
            if not is_set_bonus:
                continue

            item_set = set_spell_data.parent_record()
            if not item_set.id:
                continue

            if not set_spell_data.ref('id_spell').id:
                continue

            base_entry = {
                'index'       : set_index,
                'set_bonus_id': id,
                'bonus'       : set_spell_data.n_req_items
            }

            class_ = []

            if set_spell_data.ref('id_spec').id > 0:
                spec_ = set_spell_data.ref('id_spec').id
                spec_data = set_spell_data.ref('id_spec')
                class_.append(spec_data.class_id)
            # No spec id, set spec to "-1" (all specs), and try to use set
            # items to figure out the class (or many classes)
            else:
                spec_ = -1
                # TODO: Fetch from first available item if blizzard for some
                # reason decides to not use _1 as the first one?
                item_data = item_set.ref('id_item_1')
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

    def generate(self, data = None):
        data.sort(key = lambda v: (v['index'], v['class'], v['bonus'], v['set_bonus_id']))

        self.output_header(
                header = 'Set bonus data',
                type = 'item_set_bonus_t',
                array = 'set_bonus',
                length = len(data))

        _hdr_specifiers = (
            '{: <43}', '{: <21}', '{: <6}', '{: <5}', '{: <4}', '{: <3}', '{: <3}', '{: <4}', '{: <7}', '{}'
        )

        _data_specifiers = (
            '{: <44}', '{: <21}', '{: >6}', '{: >5}', '{: >4}', '{: >3}', '{: >3}', '{: >4}', '{: >7}', '{}'
        )

        _hdr_format = ', '.join(_hdr_specifiers)
        _hdr = _hdr_format.format(
            'SetBonusName', 'OptName', 'EnumID', 'SetID', 'Tier', 'Bns', 'Cls', 'Spec',
            'SpellID', 'ItemIDs')

        _data_format = ', '.join(_data_specifiers)

        for data_idx in range(0, len(data)):
            entry = data[data_idx]

            if data_idx % 25 == 0:
                self._out.write('  // {}\n'.format(_hdr))

            item_set_spell = self.db('ItemSetSpell')[entry['set_bonus_id']]
            set_id = item_set_spell.id_parent
            item_set = item_set_spell.parent_record()
            map_entry = self.set_bonus_map[entry['index']]

            item_set_str = ""
            items = []

            for item_n in range(1, 17):
                item_id = getattr(item_set, 'id_item_%d' % item_n)
                if item_id > 0:
                    items.append('%6u' % item_id)

            if len(items) < 17:
                items.append(' 0')

            self._out.write('  {{ {} }},\n'.format(_data_format.format(
                '"%s"' % item_set.name.replace('"', '\\"'),
                '"%s"' % map_entry['name'].replace('"', '\\"'),
                entry['index'],
                set_id,
                map_entry['tier'],
                entry['bonus'],
                entry['class'],
                entry['spec'],
                item_set_spell.id_spell,
                '{ %s }' % (', '.join(items)))))

        self._out.write('} };\n')

class SpellItemEnchantmentGenerator(DataGenerator):
    def initialize(self):
        if not super().initialize():
            return False

        # Create a mapping of enchant id <-> spell
        self._spell_map = defaultdict(list)
        [
            self._spell_map[e.misc_value_1].append(e.id_parent)
                for e in self.db('SpellEffect').values() if e.type == 53 and e.misc_value_1 > 0
        ]

        return True

    def generate(self, data = None):
        self.output_header(
                header = 'Item enchantment data',
                type = 'item_enchantment_data_t',
                array = 'spell_item_ench',
                length = len(self.db('SpellItemEnchantment')))

        for id, data in sorted(self.db('SpellItemEnchantment').items()):
            # Find spell item enchantments mapped to unique items
            items = [
                item for gem_property in data.child_refs('GemProperties')
                     for item in gem_property.child_refs('ItemSparse')
            ]

            fields = data.field('id')
            if len(items) == 1:
                fields += items[0].field('id')
            else:
                fields += self.db('ItemSparse').default().field('id')

            fields += data.field('scaling_type', 'min_scaling_level', 'max_scaling_level',
                                 'req_skill', 'req_skill_value')
            fields += [ '{ %s }' % ', '.join(data.field('type_1', 'type_2', 'type_3')) ]
            fields += [ '{ %s }' % ', '.join(
                data.field('amount_1', 'amount_2', 'amount_3')) ]
            fields += [ '{ %s }' % ', '.join(
                data.field('id_property_1', 'id_property_2', 'id_property_3')) ]
            fields += [ '{ %s }' % ', '.join(
                data.field('coeff_1', 'coeff_2', 'coeff_3')) ]

            if len(self._spell_map[id]) >= 1:
                fields += self.db('SpellName')[self._spell_map[id][0]].field('id')
            else:
                fields += self.db('SpellName').default().field('id')

            fields += data.field('desc')

            self.output_record(fields)

        self.output_footer()

class RandomPropertyPointsGenerator(DataGenerator):
    def filter(self):
        return [
            v for v in self.db('RandPropPoints').values()
                if v.id >= 1 and v.id <= self._options.scale_ilevel
        ]

    def generate(self, data = None):
        self.output_header(
                header   = 'Random property points for item levels 1-{}'.format(
                    self._options.scale_ilevel),
                type = 'random_prop_data_t',
                array = 'rand_prop_points',
                length = len(data))

        for rpp in sorted(data, key = lambda e: e.id):
            fields = rpp.field('id', 'damage_replace_stat', 'damage_secondary')
            fields += [ '{ %s }' % ', '.join(
                rpp.field('epic_points_1', 'epic_points_2', 'epic_points_3',
                          'epic_points_4', 'epic_points_5')) ]
            fields += [ '{ %s }' % ', '.join(
                rpp.field('rare_points_1', 'rare_points_2', 'rare_points_3',
                          'rare_points_4', 'rare_points_5')) ]
            fields += [ '{ %s }' % ', '.join(
                rpp.field('uncm_points_1', 'uncm_points_2', 'uncm_points_3',
                          'uncm_points_4', 'uncm_points_5')) ]

            self.output_record(fields)

        self.output_footer()

class WeaponDamageDataGenerator(DataGenerator):
    def generate(self, data = None):
        for dbname in ['ItemDamageOneHand', 'ItemDamageOneHandCaster',
                       'ItemDamageTwoHand', 'ItemDamageTwoHandCaster']:

            tokenized_name = '_'.join(re.findall('[A-Z][^A-Z]*', dbname)).lower()
            struct_name = '{}_data_t'.format(tokenized_name)

            data = [
                v for v in self.db(dbname).values()
                    if v.id >= 1 and v.id <= self._options.scale_ilevel
            ]

            self.output_header(
                    header   = 'Item damage data from {}.db2'.format(dbname),
                    type = struct_name,
                    array = tokenized_name,
                    length = len(data))

            for entry in sorted(data, key = lambda e: e.id):
                fields = entry.field('ilevel')
                fields += [ '{ %s }' % ', '.join(
                    entry.field('v_1', 'v_2', 'v_3', 'v_4', 'v_5', 'v_6', 'v_7')) ]

                self.output_record(fields)

            self.output_footer()

class ArmorValueDataGenerator(DataGenerator):
    def generate(self, data  = None):
        for dbname in ('ItemArmorQuality', 'ItemArmorShield', 'ItemArmorTotal'):
            tokenized_name = '_'.join(re.findall('[A-Z][^A-Z]*', dbname)).lower()
            struct_name = '{}_data_t'.format(tokenized_name)

            data = [
                v for v in self.db(dbname).values()
                    if v.id >= 1 and v.id <= self._options.scale_ilevel
            ]

            self.output_header(
                    header   = 'Item armor values data from {}.db2, ilevels 1-{}'.format(
                        dbname, self._options.scale_ilevel),
                    type = struct_name,
                    array = tokenized_name,
                    length = len(data))

            for entry in sorted(data, key = lambda e: e.id):
                fields = entry.field('id')
                if dbname != 'ItemArmorTotal':
                    fields += ['{ %s }' % ', '.join(
                        entry.field('v_1', 'v_2', 'v_3', 'v_4', 'v_5', 'v_6', 'v_7'))]
                else:
                    fields += ['{ %s }' % ', '.join(entry.field('v_1', 'v_2', 'v_3', 'v_4'))]

                self.output_record(fields)

            self.output_footer()

class ArmorLocationDataGenerator(DataGenerator):
    def generate(self, data = None):
        self.output_header(
                header = 'Armor location based multipliers',
                type = 'item_armor_location_data_t',
                array = 'armor_location',
                length = len(self.db('ArmorLocation')))

        for inv_type, data in sorted(self.db('ArmorLocation').items()):
            fields = data.field('id')
            fields += [ '{ %s }' % ', '.join(data.field('v_1', 'v_2', 'v_3', 'v_4')) ]
            self.output_record(fields)

        self.output_footer()

class GemPropertyDataGenerator(DataGenerator):
    def filter(self):
        return [
            v for v in self.db('GemProperties').values() if v.color > 0 or v.ref('id_enchant').id != 0
        ]

    def generate(self, data = None):
        data.sort(key = lambda v: v.id)

        self.output_header(
                header = 'Gem properties',
                type = 'gem_property_data_t',
                array = 'gem_property',
                length = len(data))

        for entry in data:
            self.output_record(entry.field('id', 'id_enchant', 'color'),
                    comment = entry.ref('id_enchant').id and entry.ref('id_enchant').desc or '')

        self.output_footer()

class ItemBonusDataGenerator(DataGenerator):
    def generate(self, data = None):
        self.output_header(
                header = 'Item bonuses',
                type = 'item_bonus_entry_t',
                array = 'item_bonus',
                length = len(self.db('ItemBonus')))

        for id, data in sorted(self.db('ItemBonus').items(), key = lambda v: (v[1].id_node, v[1].id)):
            self.output_record(data.field('id', 'id_node', 'type', 'val_1', 'val_2', 'val_3', 'val_4', 'index'))

        self.output_footer()

class ScalingStatDataGenerator(DataGenerator):
    def generate(self, data = None):
        self.output_header(
                header = 'Curve points data',
                type = 'curve_point_t',
                array = 'curve_point',
                length = len(self.db('CurvePoint')))

        for id, data in sorted(self.db('CurvePoint').items(), key = lambda k: (k[1].id_distribution, k[1].curve_index)):
            self.output_record(data.field('id_distribution', 'curve_index', 'val_1', 'val_2', 'level_1', 'level_2' ))

        self.output_footer()

class ItemNameDescriptionDataGenerator(DataGenerator):
    def generate(self, data = None):
        self.output_header(
                header = 'Item name descriptions',
                type = 'item_name_description_t',
                array = 'item_name_description',
                length = len(self.db('ItemNameDescription')))

        for id, data in sorted(self.db('ItemNameDescription').items()):
            self.output_record(data.field( 'id', 'desc' ))

        self.output_footer()

class ItemChildEquipmentGenerator(DataGenerator):
    def generate(self, data = None):
        self.output_header(
                header = 'Item child equipment',
                type = 'item_child_equipment_t',
                length = len(self.db('ItemChildEquipment')))

        for id, data in sorted(self.db('ItemChildEquipment').items()):
            self.output_record(data.field( 'id', 'id_item', 'id_child' ),
                comment = 'parent={}, child={}'.format(
                    data.ref('id_item').name, data.ref('id_child').name))

        self.output_footer()

class AzeriteDataGenerator(DataGenerator):
    def filter(self):
        power_data = set()
        power_sets = set()

        # Figure out a valid set of power set ids
        for id, data in self.db('AzeriteEmpoweredItem').items():
            if data.ref('id_item').id == 0:
                continue

            power_sets.add(data.id_power_set)

        for id, data in self.db('AzeritePowerSetMember').items():
            # Only use azerite power sets that are associated with items
            if data.id_parent not in power_sets:
                continue

            if data.ref('id_power').ref('id_spell').id == 0:
                continue

            power_data.add(data.ref('id_power'))

        for id, data in self.db('AzeriteItemMilestonePower').items():
            if data.ref('id_power').ref('id_spell').id == 0:
                continue

            power_data.add(data.ref('id_power'))

        return list(power_data)

    def generate(self, data = None):
        data = sorted(data, key = lambda e: e.id)

        self.output_header(
                header = 'Azerite powers',
                type = 'azerite_power_entry_t',
                array = 'azerite_power',
                length = len(data))

        for entry in data:
            fields = entry.field('id', 'id_spell', 'id_bonus')
            fields += entry.ref('id_spell').field('name')

            # Azerite essence stuff needs special handling, fake a tier 0 for
            # them since they are not "real" azerite powers
            if entry.ref('id_spell').name == 'Perseverance' or entry.id in [574, 581]:
                fields += self.db('AzeritePowerSetMember')[0].field('tier')
            else:
                for id, set_entry in self.db('AzeritePowerSetMember').items():
                    if set_entry.id_parent == 1:
                        continue

                    if entry.id != set_entry.id_power:
                        continue

                    fields += set_entry.field('tier')
                    break

            self.output_record(fields)

        self.output_footer()

class AzeriteEssenceDataGenerator(DataGenerator):
    def generate(self, data = None):
        data = sorted(self.db('AzeriteEssence').values(), key = lambda e: e.id)

        self.output_header(
                header = 'Azerite Essences',
                type = 'azerite_essence_entry_t',
                array = 'azerite_essence',
                length = len(data))

        for entry in data:
            self.output_record(entry.field('id', 'category', 'name'))

        self.output_footer()

        data = sorted(self.db('AzeriteEssencePower').values(),
                key = lambda x: (x.id_essence, x.rank))

        self.output_header(
                header = 'Azerite Essence Powers',
                type = 'azerite_essence_power_entry_t',
                array = 'azerite_essence_power',
                length = len(data))

        for entry in data:
            fields = entry.field('id', 'id_essence', 'rank')

            fields.append('{ %s }' % ', '.join(
                entry.field('id_spell_major_base', 'id_spell_minor_base')))
            fields.append('{ %s }' % ', '.join(
                entry.field('id_spell_major_upgrade', 'id_spell_minor_upgrade')))

            self.output_record(fields, comment = entry.ref('id_essence').name)

        self.output_footer()

class ArmorMitigationConstantValues(DataGenerator):
    def generate(self, data = None):
        filtered_entries = [
            v for _, v in self.db('ExpectedStat').items()
                if v.id_expansion == -2 and v.id_parent <= self._options.level + 3
        ]
        entries = sorted(filtered_entries, key = lambda v: v.id_parent)

        self.output_header(
                header = 'Armor mitigation constants (K-values)',
                type = 'double',
                array = 'armor_mitigation_constants',
                length = len(entries))

        for index in range(0, len(entries), 5):
            n_entries = min(len(entries) - index, 5)
            values = [
                '{: >8.3f}'.format(entries[index + value_idx].armor_constant)
                    for value_idx in range(0, n_entries)
            ]

            self._out.write('  {},\n'.format(', '.join(values)))

        self.output_footer()

class NpcArmorValues(DataGenerator):
    def generate(self, data = None):
        filtered_entries = [
            v for _, v in self.db('ExpectedStat').items()
                if v.id_expansion == -2 and v.id_parent <= self._options.level + 3
        ]
        entries = sorted(filtered_entries, key = lambda v: v.id_parent)

        self.output_header(
                header = 'Npc base armor values',
                type = 'double',
                array = 'npc_armor',
                length = len(entries))

        for index in range(0, len(entries), 5):
            n_entries = min(len(entries) - index, 5)
            values = [
                '{: >8.3f}'.format(entries[index + value_idx].creature_armor)
                    for value_idx in range(0, n_entries)
            ]

            self._out.write('  {},\n'.format(', '.join(values)))

        self.output_footer()

class TactKeyGenerator(DataGenerator):
    def generate(self, data = None):
        map_ = {}

        for id, data in self.db('TactKeyLookup').items():
            vals = []
            for idx in range(0, 8):
                vals.append('{:02x}'.format(getattr(data, 'key_name_{}'.format(idx + 1))))

            map_[id] = {'id': id, 'key_name': ''.join(vals), 'key': None}


        for id, data in self.db('TactKey').items():
            if id not in map_:
                map_[id] = {'id': id, 'key_name': None, 'key': None}

            vals = []
            for idx in range(0, 16):
                vals.append('{:02x}'.format(getattr(data, 'key_{}'.format(idx + 1))))
            map_[id]['key'] = ''.join(vals)

        out = []
        for v in sorted(map_.keys()):
            data = map_[v]
            out.append("  {{ \"id\": {:-3d}, \"key_id\": \"{}\", \"key\": {:34s} }}".format(
                v, data['key_name'], data['key'] and "\"{}\"".format(data['key']) or "null"))

        self._out.write('[\n{}\n]'.format(',\n'.join(out)))

class ItemEffectGenerator(ItemDataGenerator):
    def __init__(self, options, data_store):
        super().__init__(options, data_store)

    def filter(self):
        ids = []

        # 8.3 required item effects for Corrupted gear
        for id, bonus in self.db('ItemBonus').items():
            if bonus.type != 23:
                continue

            effect = self.db('ItemEffect')[bonus.val_1]
            if effect.id == 0:
                continue

            if effect.ref('id_spell').id == 0:
                continue

            ids.append(effect)

        # exported items effects
        for id in super().filter():
            ids.extend(self.db('ItemSparse')[id].children('ItemEffect'))

        return list(set(ids))

    def generate(self, data = None):
        item_effect_id_index = []

        self.output_header(
                header = 'Item effects',
                type = 'item_effect_t',
                array = 'item_effect',
                length = len(data))

        for index, entry in enumerate(sorted(data, key = lambda e: (e.id_parent, e.index, e.id))):
            item_effect_id_index.append((entry.id, index))

            fields = entry.field( 'id', 'id_spell', 'id_parent', 'index', 'trigger_type',
                    'cooldown_group', 'cooldown_duration', 'cooldown_group_duration' )

            self.output_record(fields, comment = entry.ref('id_spell').name)

        self.output_footer()

        self.output_id_index(
                index = [ e[1] for e in sorted(item_effect_id_index, key = lambda e: e[0]) ],
                array = 'item_effect')

class ClientDataVersionGenerator(DataGenerator):
    # Note, just returns the moddified time of the hotfix file. Hotfix file
    # does not have any kind of timestamping in the contents.
    def generate_hotfix_mtime(self):
        if not os.access(self._options.hotfix_file.name, os.R_OK):
            return ""

        stat_obj = os.stat(self._options.hotfix_file.name)
        return time.strftime('%Y-%m-%d', time.gmtime(stat_obj.st_mtime))

    def generate(self, ids = None):
        self._out.write('#ifndef {}_INC\n'.format(
            self.format_str('CLIENT_DATA_VERSION').upper()))
        self._out.write('#define {}_INC\n'.format(
            self.format_str('CLIENT_DATA_VERSION').upper()))

        self._out.write('\n// Client data versioning information for {}\n\n'.format(self._options.build))
        self._out.write('#define {} "{}"\n'.format(
            self.format_str('CLIENT_DATA_WOW_VERSION').upper(),
            self._options.build))

        if self._options.hotfix_file:
            self._out.write('\n// Hotfix data versioning information\n\n')
            self._out.write('#define {} "{}"\n'.format(
                self.format_str('CLIENT_DATA_HOTFIX_DATE').upper(),
                self.generate_hotfix_mtime()))

            self._out.write('#define {} ({})\n'.format(
                self.format_str('CLIENT_DATA_HOTFIX_BUILD').upper(),
                self._data_store.cache.parser.build))

            self._out.write('#define {} "{}"\n'.format(
                self.format_str('CLIENT_DATA_HOTFIX_HASH').upper(),
                self._data_store.cache.parser.verify_hash.hex()))

        self._out.write('\n#endif /* {}_INC*/\n'.format(
            self.format_str('CLIENT_DATA_VERSION').upper()))

class ActiveClassSpellGenerator(DataGenerator):
    def filter(self):
        return ActiveClassSpellSet(self._options).get()

    def generate(self, data = None):
        data.sort(key = lambda v: isinstance(v[0], int) and (v[0], 0) or (v[0].class_id, v[0].id))

        self.output_header(
                header = 'Active class spells',
                type = 'active_class_spell_t',
                array = 'active_spells',
                length = len(data))

        for spec_data, spell, replace_spell in data:
            fields = []
            if isinstance(spec_data, int):
                fields += ['{:2d}'.format(spec_data), '{:3d}'.format(0)]
            else:
                fields += spec_data.field('class_id', 'id')
            fields += spell.field('id')
            fields += replace_spell.field('id')
            fields += spell.field('name')

            self.output_record(fields)

        self.output_footer()

class ActivePetSpellGenerator(DataGenerator):
    def filter(self):
        return PetActiveSpellSet(self._options).get()

    def generate(self, data = None):
        data.sort()

        self.output_header(
                header = 'Pet active spells',
                type = 'active_pet_spell_t',
                array = 'active_pet_spells',
                length = len(data))

        for class_id, spell_id in data:
            fields = ['{:2d}'.format(class_id)]
            fields += self.db('SpellName').get(spell_id).field('id', 'name')

            self.output_record(fields)

        self.output_footer()

class PetRaceEnumGenerator(DataGenerator):
    def generate(self, data = None):
        skill_ids = [
            id for pet_skills in constants.PET_SKILL_CATEGORIES for id in pet_skills
        ]

        self._out.write('#ifndef PET_RACES_HPP\n')
        self._out.write('#define PET_RACES_HPP\n\n')
        self._out.write('// Pet race definitions, wow build {}\n'.format(self._options.build))
        self._out.write('enum class pet_race : unsigned\n')
        self._out.write('{\n')
        self._out.write('  {:25s} = {},\n'.format('NONE', 0))

        name_re = re.compile('^(?:Pet[\s]+\-[\s]+|)(.+)', re.I)
        for id in sorted(skill_ids):
            skill = self.db('SkillLine').get(id)
            mobj = name_re.match(skill.name)
            enum_name = '{}'.format(mobj.group(1).upper().replace(' ', '_'))
            self._out.write('  {:25s} = {},\n'.format(enum_name, skill.id))

        self._out.write('};\n')
        self._out.write('#endif /* PET_RACES_HPP */')

class RuneforgeLegendaryGenerator(DataGenerator):
    def filter(self):
        entries = list(filter(lambda v: v.id_bonus and v.id_spell > 0,
            self.db('RuneforgeLegendaryAbility').values()
        ))

        # Generate spec set dict and expand entries
        spec_sets = defaultdict(list)
        for entry in self.db('SpecSetMember').values():
            spec_sets[entry.id_parent].append(entry)

        return [
            (entry, spec) for entry in entries for spec in spec_sets.get(entry.id_spec_set, [])
        ]

    def generate(self, data=None):
        self.output_header(
                header='Runeforge legendary bonuses',
                type='runeforge_legendary_entry_t',
                array='runeforge_legendary',
                length=len(data))

        for entry, spec in sorted(data, key=lambda v: (v[0].id_bonus, v[1].id_spec)):
            fields = entry.field('id_bonus')
            fields += spec.field('id_spec')
            fields += entry.field('id_spell', 'mask_inv_type', 'name')
            self.output_record(fields)

        self.output_footer()

class RankSpellGenerator(DataGenerator):
    def filter(self):
        return RankSpellSet(self._options).get()

    def generate(self, data = None):
        data.sort(key = lambda v: (v[2].name, self.db('Spell')[v[2].id].rank))

        self.output_header(
                header = 'Rank class spells',
                type = 'rank_class_spell_t',
                array = 'rank_spells',
                length = len(data))

        for class_id, spec_id, spell, replace_spell in data:
            fields = []
            fields += ['{:2d}'.format(class_id), '{:3d}'.format(spec_id)]
            fields += spell.field('id')
            fields += replace_spell.field('id')
            fields += spell.field('name')

            spell_text = self.db('Spell')[spell.id]
            fields += spell_text.field('rank')

            self.output_record(fields)

        self.output_footer()

class ConduitGenerator(DataGenerator):
    def filter(self):
        return ConduitSet(self._options).get()

    def generate(self, data=None):
        data.sort(key = lambda v: v[1])

        self.output_header(
                header = 'Conduits',
                type = 'conduit_entry_t',
                array = 'conduit',
                length = len(data))

        for spell, conduit_id in data:
            fields = ['{:3d}'.format(conduit_id),]
            fields += spell.field('id', 'name')

            self.output_record(fields)

        self.output_footer()

        # Generate spell_id-based index
        conduit_index = list((v[0].id, index) for index, v in enumerate(data))

        self.output_id_index(
            index = [ index for _, index in sorted(conduit_index) ],
            array = 'conduit_spell')


class ConduitRankGenerator(DataGenerator):
    def generate(self, data=None):
        data = list(filter(lambda v: v.ref('id_spell').id > 0, self.db('SoulbindConduitRank').values()))

        self.output_header(
                header = 'Conduit ranks',
                type = 'conduit_rank_entry_t',
                array = 'conduit_rank',
                length = len(data))

        for conduit_rank in sorted(data, key = lambda v: (v.id_parent, v.rank)):
            fields = conduit_rank.field('id_parent', 'rank', 'id_spell', 'spell_mod')

            self.output_record(fields)

        self.output_footer()

class SoulbindAbilityGenerator(DataGenerator):
    def filter(self):
        return SoulbindAbilitySet(self._options).get()

    def generate(self, data=None):
        data.sort(key = lambda v: v[0].id_spell)

        self.output_header(
                header = 'Soulbind abilities',
                type = 'soulbind_ability_entry_t',
                array = 'soulbind_ability',
                length = len(data))

        for garr_talent, soulbind in data:
            fields = garr_talent.ref('id_spell').field('id')
            fields += soulbind.field('id_covenant')
            fields += garr_talent.ref('id_spell').field('name')

            self.output_record(fields)

        self.output_footer()

class CovenantAbilityGenerator(DataGenerator):
    def filter(self):
        return CovenantAbilitySet(self._options).get()

    def generate(self, data=None):
        # Generate a spell id, classes map so we can explode the ability set
        # based on class info
        _class_map = defaultdict(lambda: [0])
        for entry in self.db('SkillLineAbility').values():
            if entry.mask_class == 0:
                continue

            _class_map[entry.id_spell] = util.class_id(mask = entry.mask_class)

        _output_data = []
        for entry in data:
            classes = _class_map[entry.id_spell]
            if isinstance(classes, int):
                classes = [classes]

            _output_data.extend([
                (c, entry) for c in classes
            ])

        _output_data.sort(key = lambda v: v[1].ref('id_spell').name)

        self.output_header(
                header = 'Covenant abilities',
                type = 'covenant_ability_entry_t',
                array = 'covenant_ability',
                length = len(_output_data))

        for class_id, entry in _output_data:
            fields = ['{:2d}'.format(class_id)]
            fields += entry.ref('id_covenant_preview').field('id_covenant')
            fields += entry.field('ability_type')
            fields += entry.ref('id_spell').field('id', 'name')

            self.output_record(fields)

        self.output_footer()
