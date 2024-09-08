import sys, os, re, types, html.parser, urllib, datetime, signal, json, pathlib, csv, logging, io, fnmatch, traceback, binascii, time

from collections import defaultdict

import dbc.db, dbc.data, dbc.parser, dbc.file

from dbc import constants, util
from dbc.constants import Class
from dbc.filter import ActiveClassSpellSet, PetActiveSpellSet, RacialSpellSet, MasterySpellSet, RankSpellSet, ConduitSet
from dbc.filter import SoulbindAbilitySet, CovenantAbilitySet, RenownRewardSet, TalentSet, TemporaryEnchantItemSet
from dbc.filter import PermanentEnchantItemSet, ExpectedStatModSet, TraitSet, EmbellishmentSet, CharacterLoadoutSet
from dbc.filter import TraitLoadoutSet

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
        return 'static constexpr %s _%s%s%s%s = {\n' % (
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
    _class_names = [ None,
                     'Warrior', 'Paladin', 'Hunter', 'Rogue',
                     'Priest', 'Death Knight', 'Shaman', 'Mage',
                     'Warlock', 'Monk', 'Druid', 'Demon Hunter',
                     'Evoker' ]
    _class_masks = [ None,
                     0x0001, 0x0002, 0x0004, 0x0008, # warrior paladin hunter rogue
                     0x0010, 0x0020, 0x0040, 0x0080, # priest deathknight shaman mage
                     0x0100, 0x0200, 0x0400, 0x0800, # warlock monk druid demonhunter
                     0x1000 ]                        # evoker
    _race_names  = [ None,
                     'Human', 'Orc', 'Dwarf', 'Night Elf',
                     'Undead', 'Tauren', 'Gnome', 'Troll',
                     'Goblin', 'Blood Elf', 'Draenei', 'Dark Iron Dwarf',
                     'Vulpera', 'Mag\'har Orc', 'Mechagnome', 'Dracthyr',
                     None, 'Earthen', None, None,
                     None, 'Worgen', None, None,
                     None, 'Pandaren', 'Nightborne', 'Highmountain Tauren',
                     'Void Elf','Lightforged Draenei', 'Zandalari Troll', 'Kul Tiran' ]
    # simc uses race_bits left shifted by 1 compared to PlayableRaceBit in ChrRaces.db2
    _race_masks  = [ None,
                     0x00000001, 0x00000002, 0x00000004, 0x00000008,  # human orc dwarf nightelf
                     0x00000010, 0x00000020, 0x00000040, 0x00000080,  # undead tauren gnome troll
                     0x00000100, 0x00000200, 0x00000400, 0x00000800,  # goblin bloodelf draenei darkirondwarf
                     0x00001000, 0x00002000, 0x00004000, 0x00008000,  # vulpera magharorc mechagnome dracthyr(H)
                     None,       0x00020000, None,       None,        # dracthyr(A) earthen(H) earthen(A) unused
                     None,       0x00200000, None,       None,        # unused worgen unused pandaren(N)
                     None,       0x02000000, 0x04000000, 0x08000000,  # pandaren(A) pandaren(H) nightborne highmountaintauren
                     0x10000000, 0x20000000, 0x40000000, 0x80000000 ] # voidelf lightforgeddraenei zandalaritroll kultiran
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
    def generate(self, ids = None):
        output_data = []

        for rppm in self.db('SpellProcsPerMinute').values():
            for aopts in rppm.child_refs('SpellAuraOptions'):
                spell_id = aopts.id_parent
                if spell_id == 0:
                     continue

                for data in rppm.children('SpellProcsPerMinuteMod'):
                    output_data.append((spell_id, data.id_chr_spec, data.unk_1, data.coefficient))

        self.output_header(
                header = 'RPPM Modifiers',
                type = 'rppm_modifier_t',
                array = 'rppm_modifier',
                length = len(output_data))

        for data in sorted(output_data, key = lambda v: v[0]):
            self._out.write('  { %6u, %4u, %2u, %7.4f },\n' % data)

        self.output_footer()

class SpecializationEnumGenerator(DataGenerator):
    def filter(self):
        enum_ids = []
        for idx in range(0, len(constants.CLASS_INFO) + 1):
            enum_ids.append([None] * constants.MAX_SPECIALIZATION)

        max_specialization = 0
        for spec_id, spec_data in sorted(self.db('ChrSpecialization').items()):
            # Ignore "Initial" and "Adventurer" specializations for now
            # TODO: Revisit
            if spec_data.name == 'Initial' or spec_data.name == 'Adventurer':
                continue

            if spec_data.class_id > 0:
                spec_name = '%s_%s' % (
                    util.class_name(class_id=spec_data.class_id).upper().replace(" ", "_"),
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

        # manually add augmentation evoker to live specialization enums
        if self._options.build.patch_level() < dbc.WowVersion( 10, 1, 5, 49516 ).patch_level():
            enum_ids[13][2] = { 'id': 1473, 'name': 'EVOKER_AUGMENTATION' }

        return enum_ids

    def generate(self, enum_ids = None):
        self._out.write('enum specialization_e {\n')
        self._out.write('  SPEC_NONE              = 0,\n')
        self._out.write('  SPEC_PET               = 1,\n')
        for specs in enum_ids:
            for spec in specs:
                if spec:
                    self._out.write('  {:<23}= {},\n'.format(spec['name'], spec['id']))
        self._out.write('};\n\n')

class SpecializationListGenerator(SpecializationEnumGenerator):
    def generate(self, enum_ids = None):
        self._out.write('#define MAX_SPECS_PER_CLASS (%u)\n' % (max(len(specs) for specs in enum_ids)))
        self._out.write('#define MAX_SPEC_CLASS  (%u)\n\n' % len(enum_ids))

        self._out.write('static constexpr specialization_e __class_spec_id[MAX_SPEC_CLASS][MAX_SPECS_PER_CLASS] =\n{\n')

        for specs in enum_ids:
            self._out.write('  {\n')
            for spec in specs:
                if spec == None:
                    self._out.write('    SPEC_NONE,\n')
                else:
                    self._out.write('    %s,\n' % spec['name'])
            self._out.write('  },\n')

        self._out.write('};\n\n')

class TalentDataGenerator(DataGenerator):
    def filter(self):
        return TalentSet(self._options).get()

    def generate(self, data = None):
        self._out.write('// %d talents, wow build %s\n' % ( len(data), self._options.build ))
        self._out.write('static std::array<talent_data_t, %d> __%s_data { {\n' % (
            len(data), self.format_str( 'talent' ) ))

        for talent in sorted(data, key=lambda v: v.id):
            fields = talent.ref('id_spell').field('name')
            fields += talent.field('id')
            fields += [ '%#.2x' % 0 ]
            fields += [ '%#.04x' % (util.class_mask(class_id=talent.class_id) or 0) ]
            fields += talent.field('spec_id', 'col', 'row', 'id_spell', 'id_replace' )
            # Pad struct with empty pointers for direct rank based spell data access
            fields += [ ' 0' ]

            self.output_record(fields)

        self.output_footer()

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

    def filter(self):
        data = []

        for item in self.db('ItemSparse').values():
            blacklist_item = False

            if item.id in self._item_blacklist:
                continue

            for pat in self._item_name_blacklist:
                if item.name and re.search(pat, item.name):
                    blacklist_item = True

            if blacklist_item:
                continue

            filter_ilevel = True

            classdata = self.db('Item')[item.id]

            # Item no longer in game
            # LEGION: Apparently no longer true?
            #if data.flags_1 & 0x10:
            #    continue

            # Various things in armors/weapons
            if item.id in constants.ITEM_WHITELIST:
                filter_ilevel = False
            elif classdata.classs in [ 2, 4 ]:
                # All shirts
                if item.inv_type == 4:
                    filter_ilevel = False
                # All tabards
                elif item.inv_type == 19:
                    filter_ilevel = False
                # All epic+ armor/weapons
                elif item.quality >= 4:
                    filter_ilevel = False
                else:
                    # On-use item, with a valid spell (and cooldown)
                    for item_effect in [c.ref('id_item_effect') for c in item.children('ItemXItemEffect')]:
                        if item_effect.trigger_type == 0 and item_effect.id_spell > 0 and (item_effect.cooldown_group_duration > 0 or item_effect.cooldown_duration > 0):
                            filter_ilevel = False
                            break
            # Gems
            elif classdata.classs == 3 or (classdata.classs == 7 and classdata.subclass == 4):
                if item.gem_props == 0:
                    continue
                else:
                    filter_ilevel = False
            # Consumables
            elif classdata.classs == 0:
                # Potions, Elixirs, Flasks. Simple spells only.
                if classdata.has_value('subclass', [1, 2, 3]):
                    for item_effect in [c.ref('id_item_effect') for c in item.children('ItemXItemEffect')]:
                        spell = item_effect.ref('id_spell')
                        if not spell.has_effect('type', 6):
                            continue

                        # Grants armor, stats, rating, direct trigger of spells, or debuff
                        if not spell.has_effect('sub_type', [13, 22, 29, 99, 189, 465, 43, 42, 270]):
                            continue

                        filter_ilevel = False
                # Food
                elif classdata.has_value('subclass', 5):
                    for item_effect in [c.ref('id_item_effect') for c in item.children('ItemXItemEffect')]:
                        spell = item_effect.ref('id_spell')
                        for effect in spell._effects:
                            if not effect:
                                continue

                            if effect.sub_type == 23 or effect.type == 134:
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
                    item = item_effect.child_ref('ItemXItemEffect').parent_record()
                    if item.id in map_:
                        filter_ilevel = False
                    else:
                        continue
            # Hunter scopes and whatnot
            elif classdata.classs == 7:
                if classdata.has_value('subclass', 3):
                    for item_effect in [c.ref('id_item_effect') for c in item.children('ItemXItemEffect')]:
                        spell = item_effect.ref('id_spell')
                        for effect in spell._effects:
                            if not effect:
                                continue

                            if effect.type == 53:
                                filter_ilevel = False
            # Only very select quest-item permanent item enchantments
            elif classdata.classs == 12:
                valid = False
                for item_effect in [c.ref('id_item_effect') for c in item.children('ItemXItemEffect')]:
                    spell = item_effect.ref('id_spell')
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
            elif item.inv_type == 19:
                filter_ilevel = False
            # All heirlooms
            elif item.quality == 7:
                filter_ilevel = False

            # Include all Armor and Weapons
            if classdata.classs in [2, 4]:
                filter_ilevel = False

            # Item-level based non-equippable items
            if filter_ilevel and item.inv_type == 0:
                continue
            # All else is filtered based on item level
            elif filter_ilevel and (item.ilevel < self._options.min_ilevel or item.ilevel > self._options.max_ilevel):
                continue

            data.append(item)

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

            if item_.classs in [2, 4] and item not in data:
                data.append(item)

        return data

    def generate(self, data = None):
        #if (self._options.output_type == 'cpp'):
        self.generate_cpp(data)
        #elif (self._options.output_type == 'js'):
        #    return self.generate_json(ids)
        #else:
        #    return "Unknown output type"

    def generate_cpp(self, data = None):
        data.sort(key=lambda i: i.id)

        # filter out missing items XXX: is this possible?
        def check_item(item):
            if not item.id:
                sys.stderr.write('Item id {} not found\n'.format(item.id))
                return False
            return True
        data[:] = [ item for item in data if check_item(item) ]

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

        for item in data:
            stats = []
            for i in range(1, 11):
                stat_type = getattr(item, 'stat_type_{}'.format(i))
                if stat_type > 0:
                    stats.append(( stat_type,
                                   getattr(item, 'stat_alloc_{}'.format(i)),
                                   item.field('stat_socket_mul_{}'.format(i))[0] ))
            if len(stats):
                items_stats_index.add(item.id, sorted(stats, key=lambda s: s[:2]))

        self._out.write('static const dbc_item_data_t::stats_t __{}_data[{}] = {{\n'.format(
            self.format_str('item_stats'), len(items_stats_index)))
        for stats in items_stats_index.items():
            self._out.write('  {{ {:>2}, {:>5}, {} }},\n'.format(*stats))
        self._out.write('};\n\n')

        def item_stats_fields(id):
            stats = items_stats_index.get(id)
            if stats is None:
                return [ '0', '0' ]
            return [ '&__{}_data[{}]'.format(self.format_str('item_stats'), stats[0]), str(stats[1]) ]

        self._out.write('// Items, ilevel {}-{}, wow build {}\n\n'.format(
                self._options.min_ilevel, self._options.max_ilevel, self._options.build))

        CHUNK_SIZE = 1<<14
        chunk_index = 0
        for i in range(0, len(data), CHUNK_SIZE):
            chunk = data[i: i + CHUNK_SIZE]
            self._out.write('static const std::array<dbc_item_data_t, {}> __{}_data_chunk{} {{ {{\n'.format(
                len(chunk), self.format_str('item'), chunk_index))

            for item in chunk:
                item2 = self.db('Item')[item.id]

                fields = item.field('name', 'id', 'flags_1', 'flags_2')

                flag_types = 0x00

                for entry in item.child_refs('JournalEncounterItem'):
                    if entry.flags_1 == 0x10:
                        flag_types |= self._type_flags['Raid Finder']
                    elif entry.flags_1 == 0xC:
                        flag_types |= self._type_flags['Heroic']

                flag_types |= self._type_flags.get(item.ref('id_name_desc').desc, 0)

                fields += [ '%#.2x' % flag_types ]
                fields += item.field('ilevel', 'req_level', 'req_skill', 'req_skill_rank', 'quality', 'inv_type')
                fields += item2.field('classs', 'subclass')
                fields += item.field('bonding', 'delay', 'dmg_range', 'item_damage_modifier')
                fields += item_stats_fields(item.id)
                fields += item.field('class_mask', 'race_mask')
                fields += [ '{ %s }' % ', '.join(item.field('socket_color_1', 'socket_color_2', 'socket_color_3')) ]
                fields += item.field('gem_props', 'socket_bonus', 'item_set', 'id_curve', 'id_artifact' )
                fields += item2.ref('id_crafting_quality').field('tier')

                self.output_record(fields)

            self._out.write('} };\n')
            chunk_index += 1

        self._out.write('\n')
        self._out.write('static const std::array<util::span<const dbc_item_data_t>, {}> __{}_data {{ {{\n'.format(
                chunk_index, self.format_str('item')))
        self._out.write(''.join('  __{}_data_chunk{},\n'.format(self.format_str('item'), i) for i in range(chunk_index)))
        self.output_footer()

    def generate_json(self, data = None):

        data.sort(key=lambda i: i.id)

        s2 = dict()
        s2['wow_build'] = self._options.build
        s2['min_ilevel'] = self._options.min_ilevel
        s2['max_ilevel'] = self._options.max_ilevel
        s2['num_items'] = len(data)

        s2['items'] = list()

        for item in data:
            item2 = self.db('Item')[item.id]

            if not item.id:
                sys.stderr.write('Item id %d not found\n' % item.id)
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

            for entry in item.child_refs('JournalEncounterItem'):
                if entry.flags_1 == 0x10:
                    flag_types |= self._type_flags['Raid Finder']
                elif entry.flags_1 == 0xC:
                    flag_types |= self._type_flags['Heroic']

            flag_types |= self._type_flags.get(item.ref('id_name_desc').desc, 0)

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

            item_effects = [c.ref('id_item_effect') for c in item.children('ItemXItemEffect')]
            if len(item_effects) > 0:
                item_entry['spells'] = list()
                for spell in item_effects:
                    spl = dict()
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
    _spell_ref_rx = r'(?:[?|&]\(?!?[Saps]|@spell(?:name|desc|icon|tooltip)|\$|&)([0-9]+)'

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
         134735,                    # PvP Rules Enabled
         109871, 109869,            # No'Kaled the Elements of Death - LFR
         107785, 107789,            # No'Kaled the Elements of Death - Normal
         109872, 109870,            # No'Kaled the Elements of Death - Heroic
         52586,  68043,  68044,     # Gurthalak, Voice of the Deeps - LFR, N, H
         109959, 109955, 109939,    # Rogue Legendary buffs for P1, 2, 3
         84745,  84746,             # Shallow Insight, Moderate Insight
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
         183950,                    # Darklight Ray (WoD 6.2 Int DPS Trinket 3 damage spell)
         184559,                    # Spirit Eruption (WoD 6.2 Agi DPS Trinket 3 damage spell)
         184279,                    # Felstorm (WoD 6.2 Agi DPS Trinket 2 damage spell)
         60235,                     # Darkmoon Card: Greatness proc
         96977, 96978, 96979,       # Restabilizing Matrix buffs
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
         # Surprisingly Palatable Feast
         327702, 327704, 327705,
         # Feast of Gluttonous Hedonism
         327707, 327708, 327709,
         # Darkmoon Deck Putrescence
         311465, 311466, 311467, 311468, 311469, 311470,
         # Darkmoon Deck Voracity
         311484, 311485, 311486, 311487, 311488, 311489,
         # Soul Igniter Trinket
         345215,
         # Sunblood Amethyst Trinket
         343395, 343396,
         # Inscrutable Quantum Device Trinket
         330363, 330364, 330366, 330367, 330368, 330372, 330373, 330376, 330380,
         # Grim Codex Trinket
         345864,
         # Soulbinds
         321524, # Niya's Tools: Poison (night fae/niya)
         320130, 320212, # Social Butterfly vers buff (night fae/dreamweaver)
         332525, 341163, 341165, 332526, # Bron's Call to Action (kyrian/mikanikos)
         323491, 323498, 323502, 323504, 323506, # Volatile Solvent's different buff variations
         344052, 344053, 344057, # Night Fae Stamina Passives
         344068, 344069, 344070, # Venthyr Stamina Passives
         344076, 344077, 344078, # Necrolord Stamina Passives
         344087, 344089, 344091, # Kyrian Stamina Passives
         354054, 354053, # Fatal Flaw vers/crit buffs (venthyr/nadjia)
         351687, # Necrolord Bonesmith Heirmir Mnemonic Equipmen debuff
         352520, # Marileth's Kevin's Ooozeling Kevin's Wrath damage spell
         # Cabalists Hymnal
         344820,
         # Empyreal Ordnance
         345544,
         # Satchel of Misbegotten Minions
         345638,
         # Unbound Changeling
         330764,
         # Infinitely Divisible Ooze
         345495,
         # Echo Of Eonar Legendary
         347660, 347660, # Healing bonuses
         347662, 347665, # Damage Reduction bonuses
         # Anima Field for Anima Field Emitter trinket
         345535,
         # Hateful Chain
         345364, 345361,
         # Shadowgrasp Totem damage
         331537,
         # Hymnal of the Path damage/healing spells
         348141, 348140,
         # Mistcaller Ocarina
         330132, 332077, 332078, 332079,
         # Skulking Predator leap/damage
         345020,
         # Tablet of Despair
         336183,
         # Rotbriar Sprout
         329548,
         # 9.1 Soulbinds
         352881, # Bonded Hearts (night fae/niya)
         352918, 358404, # Newfound Resolve (kyrian/pelagos)
         352086, 352095, # Pustule Eruption (Necrolord/Emeni)
         352938, 352940, 358379, # Soulglow Spectrometer (Kyrian/Mikanikos)
         # Miniscule Mailemental in an Envelope
         352542,
         # Tome of Monstrous Constructions
         357163, 357168, 357169,
         # Blood Link (Shard of Domination Rune Word)
         355761, 359395, 359420, 359421, 359422, 355767, 355768, 355769, 355804,
         # Winds of Winter (Shard of Domination Rune Word)
         355724, 359387, 359423, 359424, 359425, 355733, 355735,
         # Chaos Bane (Shard of Domination Rune Word)
         355829, 359396, 359435, 359436, 359437, 356042, 356043, 356046,
         # Banshee's Blight (Sylvanas Dagger) damage spell
         358126,
         # Exsanguinated (Shard of Bek)
         356372,
         # Siphon Essence (Shard of Zed)
         356320,
         # Accretion (Shard of Kyr)
         356305,
         # Shredded Soul (Ebonsoul Vise)
         357785,
         # Duelist's Shot (Master Duelist's Chit)
         336234,
         # Nerubian Ambush, Frost-Tinged Carapace Spikes (Relic of the Frozen Wastes)
         355912, 357409,
         # Volatile Detonation (Ticking Sack of Terror)
         351694, 367903,
         # Reactive Defense Matrix (Trinket damage)
         356857,
         # Withering Fire (Dark Ranger's Quiver)
         353515,
         # Preternatural Charge (Yasahm the Riftbreaker)
         351561,
         # So'leah's Secret Technique (External Stat buff)
         368510, 351953,
         # Mythic Plus Season 2 Anima Powers
         357575, 357582, 357584, # champion's brand
         357609, # dagger of necrotic wounding
         357864, # raging battle-axe
         357706, # volcanic plume
         # Codex of the First Technique's damage spell
         351450,
         # 9.2 Encrypted M+ Affix
         368239, 368240, 368241, 368495,
         # 9.2 Trinkets & Weapons
         368845, 368863, 368865, # Antumbra, Shadow of the Cosmos
         368635, 368636, 368637, 368638, 363839, 368641, 368642, 368850, # Scars of Fraternal Strife
         368223, 368224, 368225, 368229, 368231, 368232, 368233, 368234, # Resonant Reservoir
         369439, 369544, # Elegy of the Eternals
         369294, 369318, # Grim Eclipse
         368654, 368655, 368656, 368657, 367804, 368649, 368650, 368651, 368652, 368653, # Cache of Acquired Treasures
         368689, # Soulwarped Seal of Wrynn buff Lion's Hope
         360075, # Magically Regulated Automa Core damage
         368587, # Bells of the Endless Feast
         368693, 368694, 368695, 368696, 368697, 368698, 368699, 368700, 368701, 368702, 369067, 369071, 369097, 369163, 369165, 369183, # Gavel of the First Arbiter
         360074, # Magically Regulated Automa Core
         367971, # Extract of Prodigious Sands
         367466, 367467, # Broker's Lucky Coin
         368747, # Pulsating Riftshard
         367327, 367455, 367457, 367458, # Gemstone of Prismatic Brilliance
         368643, # Chains of Domination AoE damage
         363338, # Jailer fight buff
         373108, 373113, 373116, 373121, # Season 4 M+ Bounty buffs
         # 10.0 Dragonflight ================================================
         371070, # Rotting from Within debuff on 'toxic' consumables
         371348, 371350, 371351, 371353, # Phial of Elemental Chaos
         370772, 370773, # Phial of Static Empowerment
         374002, 374037, # Iced Phial of Corrupting Rage
         371387, # Phial of Charged Isolation
         382835, 382837, 382838, 382839, 382840, 382841, 382842, 382843, # Darkmoon Deck: Inferno
         382852, 382853, 382854, 382855, 382856, 382857, 382858, 382859, # Darkmoon Deck: Watcher
         382844, 382845, 382846, 382847, 382848, 382849, 382850, 382851, # Darkmoon Deck: Rime
         382860, 382861, 382862, 382863, 382864, 382865, 382866, 382867, # Darkmoon Deck: Dance
         375335, 375342, 375343, 375345, # Elemental Lariat JC Neck
         387336, # Blue Silk Lining
         376932, # MAGIC SNOWBALL
         379403, 379407, # Toxic Thorn Footwraps LW Boots
         382095, 382096, 382097, # Rumbling Ruby trinket
         382078, 382079, 382080, 382081, 382082, 382083, 394460, 394461, 394462, # Whispering Incarnate Icon
         377451, 381824, # Conjured Chillglobe
         382425, 382428, 394864, # Spiteful Storm
         382090, # Storm-Eater's Boon
         382131, 394618, # Iceblood Deathsnare
         377463, 382135, 382136, 382256, 382257, 394954, 395703, 396434, # Manic Grieftorch
         377458, 377459, 377461, 382133, # All-Totem of the Master
         388608, 388611, 388739,  # Idol of Pure Decay
         389407, # Furious Ragefeather
         381586, # Erupting Spear Fragment
         381954, 381955, 381956, 381957, # Spoils of Neltharus
         383758, # Bottle of Spiraling Winds
         383813, 383814, 389817, 389818, 389820, 390896, 390941, # Ruby Whelp Shell
         388560, 388583,  # Tome of Unstable Power
         381965, 381966, 381967, # Controlled Currents Technique
         381698, 381699, 381700, # Forgestorm
         386708, 386709, # Dragon Games Equipment
         381760, # Mutated Magmammoth Scale
         389710, 392899, 389707, # Blazebinder's Hoof
         394453, # Broodkeeper's Blaze
         397118, 397478, # Neltharax, Enemy of the Sky
         390899, 390869, 390835, 390655, # Primal Ritual Shell
         396369, # Season 1 Thundering M+ Affix
         392271, 392275, # Seasoned Hunter's Trophy stat buffs
         393987, # Azureweave Vestments set bonus driver
         390529, 390579, 392038, # Raging Tempest set 4pc
         389820, 383813, 383812, 389816, 389839, 383814, 390234, # Ruby Whelp Shell
         389581, # Emerald Coach's Whistle
         # 10.0.7
         403094, 403170, # Echoing Thunder Stone buffs
         405209, # Humming Arcane Stone
         398293, 398320, # Winterpelt Totem
         # 10.1.0
         408791, # Ashkandur
         408089, # Idol of Debilitating Arrogance Death Knight
         408090, # Idol of Debilitating Arrogance Druid?
         408042, # Idol of Debilitating Arrogance Rogue?
         408087, # Idol of Debilitating Arrogance Priest
         408126, # Neltharion's Call to Chaos Mage
         408122, # Neltharion's Call to Chaos Warrior
         408127, # Neltharion's Call to Chaos Evoker
         408128, # Neltharion's Call to Chaos Demon Hunter
         408123, # Neltharion's Call to Chaos Paladin
         408256, # Neltharion's Call to Dominance Warlock
         408259, # Neltharion's Call to Dominance Shaman
         408260, # Neltharion's Call to Dominance Monk
         408262, # Neltharion's Call to Dominance Hunter
         401303, 401306, 408513, 408533, 408534, 408535, 408536, 408537, 408538, 408539, 408540, 408584, 408578, 410264, 401324, # Elemntium Pocket Anvil
         408815, 408832, 408835, 408836, 408821, 403545, # Djaruun, Pillar of the Elder Flame
         408667, 408671, 408675, 408770, 408682, 408694, # Dragonfire Bomb Dispenser
         406244, 378758, 407090, 407092, # Spore Colony Shoulderguards
         407949, 408015, # Tinker: Shadowflame Rockets
         406753, 406764, 406770, # Shadowflame Wreathe
         401513, 405620, 401516, 405613, 402221, 405615, 401521, 405608, 401518, 405612, 401519, 405611, # Glimmering Chromatic orb
         406251, 406254, 406887, # Shadowflame-Tempered Armor Patch
         407087, # Ever-Decaying Spores
         400959, # Zaquali Chaos Grapnel
         402813, 402894, 402896, 402897, 402898, 402903, 407961, 407982, # Igneous Tidestone
         401428, 401422, # Vessel of Searing Shadow
         407914, 407913, 407939,  # Might of the Drogbar set
         406550, # Enduring Dreadplate
         # 10.1.5
         418076, 418527, 418547, 418773, 418774, 418775, 418776, 419591, 418588, 418999, 418605, 419052,  # Mirror of Fractured Tomorrows
         417356, 417290, 417139, 417069, 417050, 417049, # Prophetic Stonescales
         417534, 417545, 417792, 417543, # Time-Thief's Gambit
         417449, 417458, 417456, 417452, # Accelerating Sandglass
         415284, 415410, 415339, 415403, 415412, 415395, # Paracausal Fragment of Thunderfin
         415006, 415130, 415245, 415033, 419539, 415052, 415038, # Paracausal Fragment of Frostmourne
         414976, 414977, 414968, 417468, # Paracausal Fragment of Azzinoth
         414856, 414864, 414858, 414857, 414865, 414866, # Paracausal Fragment of Sulfuras
         414928, 414936, 414951, 414950, 414935, # Paracausal Fragment of Doomhammer/Shalamayne
         419737, 420144, # Timestrike
         419262, 419261, # Demonsbane
         # 10.2
         425838, 426339, 426262, 426288, 426341, 426535, 426534, 426486, 426431, # Incandescent Essence Enchant
         422858, 426676, 426647, 426672, # Pip's Emerald Frendship Badge
         423611, 426898, 426911, 426906, 423021, 426897, # Ashes of the Embersoul
         426827, 427059, 426834, 427037, 427058, 427057, 427056, 426840, 427047, # Coiled Serpent Idol
         423124, 426564, 426553, # Augury of the Primal Flame
         426114, # Bandolier of Twisted Blades
         424228, 424276, 424274, 424272, 424275, # Pinch of Dream Magic: Shapeshift Buffs
         427209, 427212, # Dreambinder, Loom of the Great Cycle Damage Spell and Slowed Debuff
         425509, # Branch of the Tormented Ancient
         425180, 425156, # Infernal Signet Brand
         421990, 421996, 421994, 422016, # Gift of Ursine Vengeance
         422651, 422652, 422750, 425571, 425701, 425703, 425461, # Fyrakks Tainted Rageheart
         426474, # Verdant Tether embellishment stat buff
         424051, 424057, # String of Delicacies
         417138, # Fyr'alath the Dreamrender
         # 10.2.6
         433889, 433915, 433930, 433954, 433956, 433957, 433958, 434021, 434022, 434070, 434071, 434072, 434233, # reworked Tome of Unstable Power
         433826, 433829, 433830, # reworked globe of jagged ice
         433768, 433786, # reworked umbrelskul's fractured heart
         434069, # reworked granyth's enduring scale
         # 10.2.7
         431760, 440393, # Cloak of Infinite Potential / Timerunner's Advantage
         432334, # Explosive Barrage Damage
         429409, # Fervor Damage
         437064, # Lifestorm Damage
         429273, # Arcanist's Edge Damage
         429377, # Slay Damage
         # 11.0 The War Within ================================================
         451916, 451917, 451918, 451920, 451921, # earthen racial well fed buff
         206150, 461904, 461910, 462661, # new M+ affixes
         443585, # fateweaved needle
         452279, # aberrant spellforge
         448621, 448643, # void reaper's chime
         442267, 442280, # befouler's syringe
         448436, # sik'ran's shadow arsenal
         447097, # swarmlord's authority
         449966, # malfunctioning ethereum module
         452325, 452457, 452498, # Sigil of Algari Concordance
         452226, # Ara-Kara Sacbrood
         455910, # voltaic stormcaller
         450921, 451247, # high speaker's accretion
         450882, # signet of the priory
         451303, 451991, # harvester's edict
         450416, 450429, 450458, 450459, 450460, # candle conductor's whistle
         450204, # twin fang instruments
         457284, # TWW Primary Stat Food
         443764, 451698, 451699, # Embrace of the Cinderbee Set
         446234, # Spark of Beledar
         461957, 454137, 461939, 461943, 461925, 461924, 461922, 461927, 461947, 461948, 461949, 461946, 461861, 461860, 461859, 461858, 461857, 461856, 461855, 461854, 461853, 461845, # TWW Food Buffs
         457630, 455523, 457628, # Woven Dusk Tailoring Set
         463059, # Darkmoon Deck: Ascension
         463232, # Darkmoon Sigil: Symbiosis
         441508, 441507, 441430, # Nerubian Phearomone Secreter
         455441, 455454, 455455, 455456, #Unstable Power Core Mastery Crit Haste Vers
         455521, 455522, 457627, # Woven Dawn Tailoring Set
        ),

        # Warrior:
        (
            ( 58385,  0 ),          # Glyph of Hamstring
            ( 118779, 0, False ),   # Victory Rush heal is not directly activatable
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
            ( 409983, 0 ),          # Merciless Assault (T30 Fury 4p)
            ( 280776, 0 ),          # Sudden Death (Arms/Prot buff)
            ( 52437,  0 ),          # Sudden Death (Fury buff)
            ( 425534, 0 ),          # T31 prot Fervid Bite
            ( 385228, 0 ),          # Seismic Reverbration Arms WW
            ( 385233, 0 ),          # Seismic Reverbration Fury MH WW
            ( 385234, 0 ),          # Seismic Reverbration Fury OH WW
        ),

        # Paladin:
        (
            ( 86700, 5 ),           # Ancient Power
            ( 122287, 0, True ),    # Symbiosis Wrath
            ( 42463, 0, False ),    # Seal of Truth damage id not directly activatable
            ( 114852, 0, False ),   # Holy Prism false positives for activated
            ( 114871, 0, False ),
            ( 65148, 0, False ),    # Sacred Shield absorb tick
            ( 136494, 0, False ),   # World of Glory false positive for activated
            ( 113075, 0, False ),   # Barkskin (from Symbiosis)
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
            ( 275483, 0 ),          # Inner Light azerite trait damge
            ( 184689, 0 ),          # Shield of Vengeance damage proc
            ( 286232, 0 ),          # Light's Decree damage proc
            ( 327225, 2 ),          # First Avenger absorb buff
            ( 339669, 0 ),          # Seal of Command
            ( 339376, 0 ),          # Truth's Wake
            ( 339538, 0 ),          # TV echo
            ( 327510, 0 ),          # Shining Light free WoG
            ( 182104, 0 ),          # Shining Light stacks 1-4
            ( 340147, 0 ),          # Royal Decree
            ( 339119, 0 ),          # Golden Path
            ( 340193, 0 ),          # Righteous Might heal
            ( 337228, 0 ),          # Final verdict buff
            ( 326011, 0 ),          # Divine Toll buff to judgment damage
            ( 328123, 0 ),          # Blessing of Summer damage spell
            ( 340203, 0 ),          # Hallowed Discernment damage
            ( 340214, 0 ),          # Hallowed Discernment heal
            ( 355567, 0 ),          # Equinox
            ( 355455, 0 ),          # Divine Resonance
            ( 387643, 0 ),          # Sealed Verdict buff
            ( 382522, 0 ),          # Consecrated Blade buff
            ( 383305, 0 ),          # Virtuous Command damage
            ( 387178, 0 ),          # Empyrean Legacy buff
            ( 224239, 0 ),          # Tempest divine storm
            ( 384810, 0 ),          # Seal of Clarity buff
            ( 404140, 0 ),          # Blessed Hammers for Adjudication
            ( 387113, 0 ),          # ES
            ( 425261, 0 ),          # Cleansing Flame (Damage - Prot T31 4pc)
            ( 425262, 0 ),          # Cleansing Flame (Healing - Prot T31 4pc)
            ( 423590, 0 ),          # Echoes of Wrath (Ret T31 4pc buff)
            ( 423593, 0 ),          # Tempest of the Lightbringer triggered by T31 4pc
            # The War Within
            ( 433807 , 0),          # Divine Guidance healing
            ( 433808 , 0),          # Divine Guidance damage
            ( 434132 , 0),          # Blessing of the Forge summon event
            ( 434255 , 0),          # Blessing of the Forge related buff
            ( 447258 , 0),          # Blessing of the Forge Forge's Reckoning - SotR trigger
            ( 447246 , 0),          # Blessing of the Forge's Sacred Word - WoG trigger
            ( 432472 , 0),          # Sacred Weapon Driver
            ( 441590 , 0),          # Sacred Weapon healing
            ( 432607 , 0),          # Holy Bulwark shield
            ( 431910 , 0),          # Sun's Avatar buff, healing buff
            ( 431907 , 0),          # Sun's Avatar buff, damage buff
            ( 431939 , 0),          # Sun's Avatar healing
            ( 431911 , 0),          # Sun's Avatar damage
            ( 431382 , 0),          # Dawnlight aoe healing
            ( 431399 , 0),          # Dawnlight aoe damage
            ( 449198 , 0),          # Highlord's Judgment hidden spell
            ( 431568 , 0),          # Morning Star driver
        ),

        # Hunter:
        (
          ( 75, 0 ),     # Auto Shot
          ( 131900, 0 ), # Murder of Crows damage spell
          ( 312365, 0 ), # Thrill of the Hunt
          ( 171457, 0 ), # Chimaera Shot - Nature
          ( 201594, 1 ), # Stampede
          ( 118455, 5 ), # Beast Cleave
          ( 268877, 0 ), # Beast Cleave player buff
          ( 389448, 5 ), # Kill Cleave
          ( 186254, 5 ), # Bestial Wrath
          ( 392054, 5 ), # Piercing Fangs
          ( 257622, 2 ), # Trick Shots buff
          ( 260395, 2 ), # Lethal Shots buff
          ( 342076, 2 ), # Streamline buff
          ( 191043, 2 ), # Legacy of the Windrunners
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
          ( 328275, 0 ), ( 328757, 0 ), # Wild Spirits (Covenenat)
          ( 339928, 2 ), ( 339929, 2 ), # Brutal Projectiles (Conduit)
          ( 341223, 3 ), # Strength of the Pack (Conduit)
          ( 339061, 0 ), # Empowered Release (Conduit)
          ( 363760, 1 ), # Killing Frenzy (T28 BM 4pc)
          ( 363805, 3 ), # Mad Bombardier (T28 SV 4pc)
          ( 394388, 3 ), # Bestial Barrage (T29 SV 4pc)
          ( 378016, 0 ), # Latent Poison Injectors (talent dd action)
          ( 386875, 2 ), # Bombardment
          ( 164273, 2 ), # Lone Wolf buff
          ( 361736, 5 ), # Coordinated Assault (pet buff)
          ( 219199, 1 ), # Dire Beast (summon)
          ( 426703, 5), # Dire Beast Kill Command
          ( 459834, 3), # Sulfur-Lined Pockets (Explosive Shot buff)
          # Hero Talents
          ( 444354, 0), # Shadow Lash
          ( 444269, 0), # Shadow Surge
          ( 442419, 0), # Shadow Hounds
        ),

        # Rogue:
        (
            ( 115192, 0 ),          # Subterfuge Buff
            ( 121474, 0 ),          # Shadow Blades off hand
            ( 113780, 0, False ),   # Deadly Poison damage is not directly activatable
            ( 89775, 0, False ),    # Hemorrhage damage is not directy activatable
            ( 86392, 0, False ),    # Main Gauche false positive for activatable
            ( 168908, 0 ),          # Sinister Calling: Hemorrhage
            ( 168952, 0 ),          # Sinister Calling: Crimson Tempest
            ( 168971, 0 ),          # Sinister Calling: Garrote
            ( 168963, 0 ),          # Sinister Calling: Rupture
            ( 115189, 0 ),          # Anticipation buff
            ( 157562, 0 ),          # Crimson Poison (Enhanced Crimson Tempest perk)
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
            ( 323558, 0 ), ( 323559, 0 ), ( 323560, 0 ), ( 354838, 0 ), # Echoing Reprimand buffs
            ( 324074, 0 ), ( 341277, 0 ), # Serrated Bone Spike secondary instant damage spells
            ( 328548, 0 ), ( 328549, 0 ), # Serrated Bone Spike combo point spells
            ( 340582, 0 ), ( 340583, 0 ), ( 340584, 0 ), # Guile Charm legendary buffs
            ( 340580, 0 ),          # Guile Charm hidden buff
            ( 340600, 0 ), ( 340601, 0 ), ( 340603, 0 ), # Finality legendary buffs
            ( 341111, 0 ),          # Akaari's Soul Fragment legendary debuff
            ( 345121, 0 ),          # Akaari's Soul Fragment Shadowstrike clone
            ( 340587, 0 ),          # Concealed Blunderbuss legendary buff
            ( 340573, 0 ),          # Greenskin's Wickers legendary buff
            ( 340431, 0 ),          # Doomblade legendary debuff
            ( 343173, 0 ),          # Premeditation buff
            ( 319190, 0 ),          # Shadow Vault shadow damage spell
            ( 345316, 0 ), ( 345390, 0 ), # Flagellation damage spells
            ( 185422, 0 ),          # Shadow Dance buff spell
            ( 350964, 0 ),          # Subtlety-specific Deathly Shadows legendary buff
            ( 364234, 0 ), ( 364235, 0 ), ( 364556, 0 ), # T28 Outlaw 4pc Spells
            ( 364668, 0 ),          # T28 Assassination debuff
            ( 385897, 0 ),          # Hidden Opportunity Ambush
            ( 386270, 0 ),          # Audacity buff
            ( 395424, 0 ),          # Improved Adrenaline Rush energize
            ( 385907, 0 ),          # Take 'em By Surprise buff
            ( 385794, 0 ), ( 385802, 0 ), ( 385806, 0 ), # Vicious Venoms Ambush/Mutilate variant spells
            ( 360826, 0 ), ( 360830, 0 ), # Deathmark Cloned Bleeds
            ( 394324, 0 ), ( 394325, 0 ), ( 394326, 0 ), ( 394327, 0 ), ( 394328, 0 ), # Deathmark Cloned Poisons
            ( 385754, 0 ), ( 385747, 0 ), # Indiscriminate Carnage bleed jump target spells
            ( 394031, 0 ),          # Replicating Shadows fake Rupture Shadow tick spell
            ( 394757, 0 ),          # Flagellation talent damage spell
            ( 394758, 0 ),          # Flagellation talent persist buff
            ( 385948, 0 ), ( 385949, 0 ), ( 385951, 0 ), # Finality talent buffs
            ( 385960, 0 ), ( 386081, 0 ), # Lingering Shadow talent background spells
            ( 393724, 0 ), ( 393725, 0 ), # T29 Assassination Set Bonus Spells
            ( 393727, 0 ), ( 393728, 0 ), ( 394879, 0 ), ( 394888, 0 ), # T29 Outlaw Set Bonus Spells
            ( 393729, 0 ), ( 393730, 0 ), # T29 Subtlety Set Bonus Spells
            ( 409604, 0 ), ( 409605, 0 ), # T30 Outlaw Set Bonus Spells
            ( 409483, 0 ),          # T30 Assassination Set Bonus Spells
            ( 413890, 0 ),          # Nightstalker background spell
            ( 424081, 0 ), ( 424066, 0 ), ( 424080, 0 ), # Underhanded Upper Hand background spells
            ( 421979, 0 ),          # Caustic Spatter damage spell
            ( 426595, 0 ),          # Shadow Techniques delayed CP energize
            ( 423193, 0 ),          # Exsanguinate residual damage spell
            ( 424491, 0 ), ( 424492, 0 ), ( 424493, 0 ), # T31 Subtlety clone damage spells
            ( 429951, 0 ),          # Deft Maneuvers alternative Blade Flurry instant attack spell
            ( 381628, 0 ),          # Dragonflight Internal Bleeding talent spell

            # The War Within
            ( 452538, 0 ),          # Fatebound coin (tails) spell
            ( 452917, 0 ),          # Fatebound coin (tails) buff
            ( 452923, 0 ),          # Fatebound coin (heads) buff
            ( 452562, 0 ),          # Lucky Coin buff
            ( 457236, 0 ),          # Singular Focus damage spell
            ( 459002, 0 ),          # Outlaw 11.0 Set Bonus damage spell
            ( 467059, 0 ),          # Outlaw Crackshot Dispatch clone damage spell
        ),

        # Priest:
        (   (  63619, 5 ), 			# Shadowfiend "Shadowcrawl"
            ( 323707, 0 ),          # Mindgames Healing reversal Damage spell
            ( 323706, 0 ),          # Mindgames Damage reversal Healing spell
            (  32409, 0 ),          # Shadow Word: Death Self-Damage
            ( 375904, 0 ),          # Mindgames Damage reversal healing spell
            # Shadow Priest
            ( 193473, 5 ),			# Void Tendril "Mind Flay"
            ( 194249, 3, False ),   # Voidform extra data
            ( 346111, 0 ),          # Shadow Weaving Mastery spell
            ( 346112, 0 ),          # Shadow Weaving Mastery Pet Proc spell
            ( 373442, 5 ),          # Inescapable Torment
            ( 375981, 0 ),          # Shadowy Insight Buff
            ( 373204, 0 ),          # Mind Devourer Talent buff
            ( 393684, 0 ),          # DF Season 1 2pc
            ( 394961, 0 ),          # Gathering Shadows DF Season 1 2pc buff
            ( 393685, 0 ),          # DF Season 1 4pc
            ( 394963, 0 ),          # Dark Reveries DF Season 1 4pc buff
            ( 377355, 0 ),          # Idol of C'thun Duration
            ( 377358, 5 ),          # Idol of C'thun Insanity values
            ( 394976, 5 ),          # Idol of C'thun Void Lasher Mind Sear
            ( 394979, 5 ),          # Idol of C'thun Void Lasher Mind Sear Tick
            ( 393919, 0 ),          # Screams of the Void extra data
            ( 375408, 0 ),          # Screams of the Void extra data
            ( 390964, 0 ),          # Halo Damage Spell
            ( 390971, 0 ),          # Halo Heal Spell
            ( 390845, 0 ),          # Divine Star Damage Spell
            ( 390981, 0 ),          # Divine Star Heal Spell
            ( 396895, 5 ),          # Idol of Yogg-Saron Void Spike Cleave
            ( 373281, 0 ),          # Idol of N'Zoth Echoing Void debuff
            ( 373213, 0 ),          # Insidious Ire Buff
            ( 407468, 0 ),          # Mind Spike Insanity Buff
            ( 391232, 0 ),          # Maddening Touch Insanity
            ( 409502, 0 ),          # DF Season 2 4pc
            ( 148859, 0 ),          # Shadowy Apparitions Travel Spell
            # Holy Priest
            ( 196809, 5 ),          # Healing Light (Divine Image legendary pet spell)
            ( 196810, 5 ),          # Dazzling Light (Divine Image legendary pet spell)
            ( 196810, 5 ),          # Searing Light (Divine Image legendary pet spell)
            ( 196811, 5 ),          # Searing Light (Divine Image talent pet spell)
            ( 196812, 5 ),          # Light Eruption (Divine Image legendary pet spell)
            ( 196813, 5 ),          # Blessed Light (Divine Image legendary pet spell)
            ( 196816, 5 ),          # Tranquil Light (Divine Image legendary pet spell)
            ( 325315, 0 ),          # Ascended Blast heal
            ( 394729, 0 ),          # Prayer Focus T29 2-set
            ( 393682, 0 ),          # Priest Holy Class Set 2-set
            ( 393683, 0 ),          # Priest Holy Class Set 4-set
            ( 394745, 0 ),          # Priest Holy Class Set 4-set buff
            ( 234946, 0 ),          # Trail of Light heal
            ( 368276, 0 ),          # Binding Heals heal
            ( 391156, 0 ),          # Holy Mending heal
            ( 391359, 0 ),          # Empowered Renew heal
            # Discipline Priest
            (  94472, 0 ), 			# Atonement Crit
            ( 280398, 1, False ),   # Sins of the Many buff
            ( 393679, 0 ),          # Priest Discipline Class Set 2-set
            ( 394609, 0 ),          # Light Weaving 2-set buff
            ( 393681, 0 ),          # Priest Discipline Class Set 4-set
            ( 394624, 0 ),          # Shield of Absolution 4-set buff
            # Oracle Hero Talents
            ( 438733, 0 ), ( 443126, 0 ), # Premonition of Piety
            ( 438734, 0 ),                # Premonition of Insight
            ( 438854, 0 ),                # Premonition of Solace
            ( 443056, 0 ), ( 450796, 0 ), # Premonition
            ( 438855, 0 ), ( 440725, 0 ), # Clairvoyance
            ( 440762, 0 ),                # Grand Reveal
            # Voidweaver Hero Talents
            ( 449887, 0 ),                # Voidheart
            ( 450983, 0 ), ( 450404, 0 ), # Void Blast
            ( 447445, 0 ), ( 449485, 0 ), # Entropic Rift
            ( 451210, 0 ),                # No Escape
            ( 451847, 0 ),                # Devour Matter
            ( 450140, 0 ), ( 450150, 0 ), # Void Empowerment
            ( 451963, 0 ), ( 451964, 0 ), # Void Leech
            ( 451571, 0 ),                # Embrace the Shadow
            # Archon Hero Talents
            ( 453983, 0 ),                # Perfected Form
            ( 453936, 0 ), ( 453943, 0 ), # Incessant Screams
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
          ( 317791, 5 ), ( 317792, 5), # Magus of the Dead's (army of the damned talent) Frostbolt and Shadow Bolt spells
          ( 324165, 0 ), # Night Fae's Death's Due Strength Buff
          ( 220890, 5 ), # Dancing Rune Weapon's RP generation spell from Heart Strike
          ( 228645, 5 ), # Dancing Rune Weapon's Heart Strike
          ( 334895, 5 ), # Frenzied Monstrosity Buff that appears on the main ghoul pet (different from the player buff)
          ( 377588, 0 ), ( 377589, 0 ), # Ghoulish Frenzy buffs
          ( 390271, 0 ), # Coil of Devastation debuff
          ( 193486, 0 ), # Runic Empowerment energize spell
          ( 377656, 0 ), # Heartrend Buff
          ( 377633, 0 ), # Leeching Strike
          ( 377642, 0 ), # Shattering Bone
          ( 364197, 0 ), ( 366008, 0 ), ( 368938, 0 ), # T28 Endless Rune Waltz Blood Set Bonus
          ( 363885, 0 ), ( 364173, 0 ), ( 363887, 0 ), ( 367954, 0 ), # T28 Harvest Time Unholy Set Bonus
          ( 364384, 0 ), # T28 Arctic Assault Frost Set Bonus
          ( 368690, 0 ), # T28 Remnant's Despair (DK ring) buff
          ( 408368, 0 ), # T30 Wrath of the Frostwyrm Frost Set buff
          ( 410790, 0 ), # T30 Wrath of the Frostwyrm FWF triggered from pillar
          ( 196780, 0 ), # Outbreak Intermediate AoE spell
          ( 196782, 0 ), # Outbreak Intermediate AoE spell
          ( 281327, 0 ), # Obliteration Rune Generation Spell
          ( 47466, 0 ), # DK Pet Stun
          ( 166441, 0 ), # Fallen Crusader
          ( 50401, 0 ),  # Razorice
          ( 327087, 0 ), # Rune of the Apocalypse
          ( 62157, 0 ),  # Stoneskin Gargoyle
          ( 326913, 0 ), # Hysteria
          ( 326801, 0 ), # Sanguination
          ( 326864, 0 ), # Spellwarding
          ( 326982, 0 ), # Unending Thirst
          ( 425721, 0 ), # T31 Blood 2pc buff
          ( 377445, 0 ), # Unholy Aura debuff
          # The War Within
          ( 290577, 0 ), # Abomiantion Disease Cloud
          ( 439539, 0 ), # Icy Death Torrent Damage
          ( 458264, 0 ), ( 458233, 0 ), # Decomposition
          ( 460501, 0 ), # Bloodied blade heart strike
          ( 463730, 0 ), # Coagulating Blood for Death Strike
          # Rider of the Apocalypse
          ( 444505, 0 ), # Mograines Might Buff
          ( 444826, 0 ), # Trollbanes Chains of Ice Main
          ( 444828, 0 ), # Trollbanes Chains of Ice Debuff
          ( 444474, 0 ), # Mograine's Death and Decay
          ( 444741, 0 ), # Rider AMS Buff
          ( 444740, 0 ), # Rider AMS Data
          ( 444763, 0 ), # Apocalyptic Conquest
          ( 444248, 0 ), # Summon Mograine
          ( 444252, 0 ), # Summon Nazgrim
          ( 444254, 0 ), # Summon Trollbane
          ( 444251, 0 ), # Summon Whitemane
          ( 454393, 0 ), # Summon Mograine 2
          ( 454392, 0 ), # Summon Nazgrim 2
          ( 454390, 0 ), # Summon Trollbane 2
          ( 454389, 0 ), # Summon Whitemane 2
          ( 448229, 0 ), # Soul Reaper
          ( 445513, 0 ), # Whitemane Death Coil
          ( 445504, 0 ), # Mograine Heart Strike
          ( 445507, 0 ), # Trollbane Obliterate
          ( 445508, 0 ), # Nazgrim Scourge Strike Phys
          ( 445509, 0 ), # Nazgrim Scourge Strike Shadow
          # San'layn
          ( 434144, 0 ), # Infliction in Sorrow Damage
          ( 434246, 0 ), # Blood Eruption
          ( 434574, 0 ), # Blood Cleave
          ( 445669, 0 ), # Vampiric Strike Range increase
          # Deathbringer
          ( 439594, 0 ), # Reapers Mark
          ( 443404, 0 ), # Wave of Souls debuff
          ( 442664, 0 ), # Wave of Souls area dummy
          ( 440005, 0 ), # Blood Fever damage
          # Tier TWW1
          ( 457506, 0 ), # Blood TWW1 set, Piledriver
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
          ( 279515, 0 ),                                # Roiling Storm
          ( 275394, 0 ),                                # Lightning Conduit
          ( 273466, 0 ),                                # Strength of Earth
          ( 279556, 0 ),                                # Rumbling Tremors damage spell
          ( 286976, 0 ),                                # Tectonic Thunder Azerite Trait buff
          ( 327164, 0 ),                                # Primordial Wave buff
          ( 336732, 0 ), ( 336733, 0 ),                 # Legendary: Elemental Equilibrium school buffs
          ( 336737, 0 ),                                # Runeforged Legendary: Chains of Devastation
          ( 354648, 0 ),                                # Runeforged Legendary: Splintered Elements
          ( 222251, 0 ),                                # Improved Stormbringer damage
          ( 381725, 0 ), ( 298765, 0 ),                 # Mountains Will Fall Earth Shock and Earthquake Overload
          ( 390287, 0 ),                                # Improved Stormbringer damage spell
          ( 289439, 0 ),                                # Frost Shock Maelstrom generator
          ( 463351, 0 ),                                # Tempest Overload
          ( 451137, 0 ), ( 450511, 0 ), ( 451031, 0 ),  # Call of the Ancestors spells
          ( 447419, 0 ),                                # Call of the Ancestor Lava Burst
          ( 447427, 0 ),                                # Call of the Ancestor Elemental Blast
          ( 447425, 0 ),                                # Call of the Ancestor Chain Lightning
        ),

        # Mage:
        (
          ( 48107, 0, False ), ( 48108, 0, False ), # Heating Up and Pyroblast! buffs
          ( 88084, 5 ), ( 88082, 5 ), ( 59638, 5 ), # Mirror Image spells.
          ( 80354, 0 ),                             # Temporal Displacement
          ( 131581, 0 ),                            # Waterbolt
          ( 7268, 0, False ),                       # Arcane missiles trigger
          ( 115757, 0, False ),                     # Frost Nova false positive for activatable
          ( 148022, 0 ),                            # Icicle
          ( 155152, 5 ),                            # Prismatic Crystal nuke
          ( 157978, 0 ), ( 157979, 0 ),             # Unstable magic aoe
          ( 244813, 0 ),                            # Second Living Bomb DoT
          ( 187677, 0 ),                            # Aegwynn's Ascendance AOE
          ( 194432, 0 ),                            # Felo'melorn - Aftershocks
          ( 225119, 5 ),                            # Arcane Familiar attack, Arcane Assault
          ( 210833, 0 ),                            # Touch of the Magi
          ( 228358, 0 ),                            # Winter's Chill
          ( 240689, 0 ),                            # Aluneth - Time and Space
          ( 248147, 0 ),                            # Erupting Infernal Core
          ( 248177, 0 ),                            # Rage of the Frost Wyrm
          ( 222305, 0 ),                            # Sorcerous Fireball
          ( 222320, 0 ),                            # Sorcerous Frostbolt
          ( 222321, 0 ),                            # Sorcerous Arcane Blast
          ( 205473, 0 ),                            # Icicles buff
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
          ( 363685, 0 ),                            # Arcane Lucidity ready buff
          ( 203277, 0 ),                            # Flame Accelerant
          ( 384035, 0 ), ( 384038, 0 ),             # Firefall
          ( 382113, 0 ), ( 382114, 0 ),             # Cold Front
          ( 365265, 0 ),                            # Arcane Surge energize
          ( 384859, 0 ), ( 384860, 0 ),             # Orb Barrage
          ( 383783, 0 ),                            # Nether Precision
          ( 383882, 0 ),                            # Sun King's Blessing
          ( 408763, 0 ),                            # Frost T30 2pc
          ( 408673, 0 ), ( 408674, 0 ),             # Fire T30 4pc
          ( 414381, 0 ),                            # Concentrated Power AE
          ( 418735, 0 ),                            # Splintering Ray
          ( 419800, 0 ),                            # Intensifying Flame
          ( 424120, 0 ),                            # Glacial Blast
          ( 461508, 0 ),                            # Energy Reconstitution AE
          ( 438671, 0 ), ( 438672, 0 ), ( 438674, 0 ), # Excess Fire LB
          ( 460475, 0 ), ( 460476, 0 ),             # Pyromaniac Pyroblast and Flamestrike
          ( 464515, 0 ),                            # Arcane Echo ICD
          ( 449559, 0 ), ( 449560, 0 ), ( 449562, 0 ), ( 449569, 0 ), # Meteorite (Glorious Incandescence)
          ( 450499, 0 ),                            # Arcane Barrage (Arcane Phoenix)
          ( 450461, 0 ),                            # Pyroblast (Arcane Phoenix)
          ( 450462, 0 ),                            # Flamestrike (Arcane Phoenix)
          ( 453326, 0 ),                            # Arcane Surge (Arcane Phoenix)
          ( 450421, 0 ),                            # Greater Pyroblast (Arcane Phoenix)
          ( 455137, 0 ),                            # Blessing of the Phoenix missile speed
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
          ( 337142, 2 ),    # Grim Inquisitor's Dread Calling Buff
          ( 342997, 2 ),    # Grim Inquisitor's Dread Calling Buff 2
          ( 339986, 3 ),    # Hidden Combusting Engine Debuff
          ( 324540, 0 ),    # Malefic Rapture damage
          ( 364322, 0 ),    # T28 - Calamitous Crescendo Buff
          ( 364348, 0 ),    # T28 - Impending Ruin Buff
          ( 364349, 0 ),    # T28 - Ritual of Ruin Buff
          ( 364198, 0 ),    # T28 - Malicious Imp-Pact Summon
          ( 364261, 0 ),    # T28 - Malicious Imp Doombolt
          ( 364263, 0 ),    # T28 - Return Soul
          ( 367679, 0 ),    # T28 - Summon Blasphemy
          ( 367680, 0 ),    # T28 - Blasphemy
          ( 367819, 0 ),    # T28 - Blasphemous Existence
          ( 367831, 0 ),    # T28 - Deliberate Corruption
          ( 387079, 0 ),    # Tormented Crescendo buff
          ( 387310, 0 ),    # Haunted Soul buff
          ( 390097, 0 ),    # Darkglare - Grim Reach
          ( 387392, 0 ),    # Dread Calling Buff
          ( 387393, 0 ),    # Dread Calling Buff 2
          ( 267218, 0 ),    # Nether Portal aura
          ( 387496, 0 ),    # Antoran Armaments buff
          ( 387502, 0 ),    # Soul Cleave
          ( 387552, 0 ),    # Infernal Command pet aura
          ( 390193, 0 ),    # Demonic Servitude aura
          ( 387096, 0 ),    # Pyrogenics aura
          ( 387109, 0 ),    # Conflagration of Chaos aura 1
          ( 387110, 0 ),    # Conflagration of Chaos aura 2
          ( 387158, 0 ),    # Impending Ruin talent aura
          ( 387356, 0 ),    # Crashing Chaos aura
          ( 387409, 0 ),    # Madness Chaos Bolt aura
          ( 387413, 0 ),    # Madness Rain of Fire aura
          ( 387414, 0 ),    # Madness Shadowburn aura
          ( 405681, 0 ),    # Immutable Hatred Damage Proc
          ( 409890, 0 ),    # T30 - Channel Demonfire
          ( 417282, 3 ),    # Crashing Chaos Buff 10.1.5
          ( 421970, 0 ),    # Ner'zhul's Volition Buff 10.2
          ( 423874, 0 ),    # T31 - Flame Rift
          ( 427285, 0 ),    # T31 - Dimensional Cinder
          ( 438973, 0 ),    # Diabolist - Felseeker
          ( 434404, 0 ),    # Diabolist - Felseeker
          ( 438823, 0 ),    # Diabolic Bolt (pet spell)
        ),

        # Monk:
        (
          # General
          ( 138311, 0 ), # Energy Sphere energy refund
          ( 163272, 0 ), # Chi Sphere chi refund
          ( 365080, 0 ), # Windwalking Movement Buff
          ( 388199, 0 ), # Jadefire Debuff
          ( 388203, 0 ), # Jadefire Reset
          ( 388207, 0 ), # Jadefire Damage
          ( 388814, 0 ), # Fortifying Brew Increases Dodge and Armor
          ( 389541, 0 ), # White Tiger Statue - Claw of the White Tiger
          ( 389684, 0 ), # Close to Heart Leech Buff
          ( 389685, 0 ), # Generous Pour Avoidance Buff
          ( 392883, 0 ), # Vivacious Vivification buff
          ( 414143, 0 ), # Yu'lon's Grace buff
          ( 450380, 0 ), # Chi Wave Buff

          # Brewmaster
          ( 195630, 1 ), # Brewmaster Mastery Buff
          ( 115129, 1 ), # Expel Harm Damage
          ( 124503, 1 ), # Gift of the Ox Orb Left
          ( 124506, 1 ), # Gift of the Ox Orb Right
          ( 178173, 1 ), # Gift of the Ox Explosion
          ( 124275, 1 ), # Light Stagger
          ( 124274, 1 ), # Medium Stagger
          ( 124273, 1 ), # Heavy Stagger
          ( 205523, 1 ), # Blackout Kick Brewmaster version
          ( 216521, 1 ), # Celestial Fortune Heal
          ( 227679, 1 ), # Face Palm
          ( 227291, 1 ), # Niuzao pet Stomp
          ( 325092, 1 ), # Purified Chi
          ( 383701, 1 ), # Gai Plin's Imperial Brew Heal
          ( 383733, 1 ), # Training of Niuzao Mastery % Buff
          ( 386959, 1 ), # Charred Passions Damage
          ( 395267, 1 ), # Call to Arms Invoke Niuzao
          ( 387179, 1 ), # Weapons of Order (Debuff)

          # Mistweaver
          ( 228649, 2 ), # Teachings of the Monastery - Blackout Proc
          ( 343820, 2 ), # Invoke Chi-Ji, the Red Crane - Enveloping Mist cast reduction
          ( 388609, 2 ), # Zen Pulse Echoing Reverberation Damage
          ( 388668, 2 ), # Zen Pulse Echoing Reverberation Heal

          # Windwalker
          ( 115057, 3 ), # Flying Serpent Kick Movement spell
          ( 116768, 3 ), # Combo Breaker: Blackout Kick
          ( 121283, 3 ), # Chi Sphere from Power Strikes
          ( 125174, 3 ), # Touch of Karma redirect buff
          ( 129914, 3 ), # Combat Wisdom Buff
          ( 195651, 3 ), # Crosswinds Artifact trait trigger spell
          ( 196061, 3 ), # Crosswinds Artifact trait damage spell
          ( 196741, 3 ), # Hit Combo Buff
          ( 196742, 3 ), # Whirling Dragon Punch Buff
          ( 220358, 3 ), # Cyclone Strikes info
          ( 228287, 3 ), # Spinning Crane Kick's Mark of the Crane debuff
          ( 240672, 3 ), # Master of Combinations Artifact trait buff
          ( 261682, 3 ), # Chi Burst Chi generation cap
          ( 285594, 3 ), # Good Karma Healing Spell
          ( 290461, 3 ), # Reverse Harm Damage
          ( 335913, 3 ), # Empowered Tiger Lightning Damage spell
          ( 388201, 3 ), # Jadefire WW Damage
          ( 396167, 3 ), # Fury of Xuen Stacking Buff
          ( 396168, 3 ), # Fury of Xuen Haste Buff
          ( 393048, 3 ), # Skyreach Debuff
          ( 393050, 3 ), # Skyreach Exxhaustion Debuff
          ( 393565, 3 ), # Thunderfist buff
          ( 395413, 3 ), # Fae Exposure Healing Buff
          ( 395414, 3 ), # Fae Exposure Damage Debuff
          ( 451968, 3 ), # Combat Wisdom Expel Harm
          ( 452117, 3 ), # Flurry of Xuen Driver
          ( 461404, 3 ), # WW Chi Burst Cast

          # Covenant
          ( 325217, 0 ), # Necrolord Bonedust Brew damage
          ( 325218, 0 ), # Necrolord Bonedust Brew heal
          ( 328296, 0 ), # Necrolord Bonedust Chi Refund
          ( 342330, 0 ), # Necrolord Bonedust Brew Grounding Breath Buff
          ( 394514, 0 ), # Necrolord Bonedust Brew Attenuation Buff

          # Shadowland Legendaries
          ( 337342, 3 ), # Jade Ignition Damage
          ( 337482, 3 ), # Pressure Point Buff

          # Tier 28
          ( 366793, 1 ), # BrM 4-piece Keg of the Heavens Heal
          ( 363911, 3 ), # WW 4-piece Primordial Potential
          ( 363924, 3 ), # WW 4-piece Primordial Power

          # Tier 29
          ( 394951, 3 ), # WW 4-piece Versatility buff

          # Tier 30
          ( 411376, 3 ), # WW 4-piece Shadowflame Vulnerability buff

          # Tier 31
          ( 425298, 1 ), # BrM 2-piece Charred Dreams Healing
          ( 425299, 1 ), # BrM 2-piece Charred Dreams Damage
          ( 425965, 1 ), # BrM 4-piece Celestial Brew Guard

          # Tier 33
          ( 457271, 1 ), # BrM 4-piece Flow of Battle

          # Shado-Pan
          ( 451021, 0 ), # Flurry Charge (Buff)

          # Conduit of the Celestials
          ( 443616, 0 ), # Heart of the Jade Serpent (Buff)
          ( 443574, 0 ), # Ox Stance
          ( 443576, 0 ), # Serpent Stance
          ( 443575, 0 ), # Tiger Stance
          ( 443592, 0 ), # Unity Within
          ( 443611, 0 ), # Flight of the Red Crane

          # Master of Harmony
          ( 451299, 0 ), # Mantra of Tenacity Chi Cocoon

        ),

        # Druid:
        ( (  93402, 1, True ), # Sunfire
          ( 106996, 1, True ), # Astral Storm
          ( 112071, 1, True ), # Celestial Alignment
          ( 122114, 1, True ), # Chosen of Elune
          ( 122283, 0, True ),
          ( 110807, 0, True ),
          ( 112997, 0, True ),
          ( 150017, 5 ),       # Rake for Treants
          ( 124991, 0 ), ( 124988, 0 ), # Nature's Vigil
          ( 155627, 2 ),       # Lunar Inspiration
          ( 155625, 2 ),       # Lunar Inspiration Moonfire
          ( 145152, 2 ),       # Bloodtalons buff
          ( 135597, 3 ),       # Tooth and Claw absorb buff
          ( 155784, 3 ),       # Primal Tenacity buff
          ( 137542, 0 ),       # Displacer Beast buff
          ( 155777, 4 ),       # Germination Rejuvenation
          ( 157228, 1 ),       # Owlkin Frenzy
          ( 202461, 1 ),       # Stellar Drift mobility buff
          ( 202771, 1 ),       # Half Moon artifact power
          ( 202768, 1 ),       # Full Moon artifact power
          ( 203001, 1 ),       # Goldrinn's Fang, Spirit of Goldrinn artifact power
          ( 203958, 3 ),       # Brambles (talent) damage spell
          ( 210721, 2 ),       # Shredded Wounds (Fangs of Ashamane artifact passive)
          ( 210713, 2 ),       # Ashamane's Rake (Fangs of Ashamane artifact trait spell)
          ( 210705, 2 ),       # Ashamane's Rip (Fangs of Ashamane artifact trait spell)
          ( 210649, 2 ),       # Feral Instinct (Fangs of Ashamane artifact trait)
          ( 213557, 2 ),       # Protection of Ashamane ICD (Fangs of Ashamane artifact trait)
          ( 211547, 1 ), ( 281871, 1 ), # Fury of Elune
          ( 106829, 0 ), ( 48629, 0 ), # Bear/Cat form swipe/thrash replacement
          ( 106899, 0 ), ( 106840, 0 ), # Bear/Cat form stampeding roar replacement
          ( 209406, 1 ),       # Oneth's Intuition buff
          ( 209407, 1 ),       # Oneth's Overconfidence buff
          ( 213666, 1 ),       # Echoing Stars artifact spell
          ( 69369,  2 ),       # Predatory Swiftness buff
          ( 227034, 3 ),       # Nature's Guardian heal
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
          ( 345048, 3 ),       # Ursoc's Fury Remembered absorb buff
          ( 340060, 0 ),       # Lycara's Fleeting Glimpse coming soon buff
          # 9.1 Legendary
          ( 354805, 0 ), ( 354802, 0 ), ( 354815, 0 ), ( 354345, 0 ), # Kindred Affinity (kyrian)

          # Shadowlands Covenant
          ( 326446, 0, True ), # Kyrian Empower Bond on DPS
          ( 326462, 0, True ), # Kyrian Empower Bond on Tank
          ( 326647, 0, True ), # Kyrian Empower Bond on Healer
          ( 338142, 0, True ), # Kyrian Lone Empowerment

          ( 340720, 1 ), ( 340708, 1 ), ( 340706, 1 ), ( 340719, 1 ), # balance potency conduits
          ( 340682, 2 ), ( 340688, 2 ), ( 340694, 2 ), ( 340705, 2 ), # feral potency conduits
          ( 340552, 3 ), ( 340609, 3 ), # guardian potency conduits
          ( 341378, 0 ), ( 341447, 0 ), ( 341446, 0 ), ( 341383, 0 ), # convenant potency conduits

          ( 365640, 1 ), # tier 28 balance 2pc foe damage spell
          ( 367907, 1 ), # tier 28 balance 2pc foe energize spell
          ( 365476, 1 ), ( 365478, 1 ), # tier 28 balance 2pc ground effect spell

          # Dragonflight
          ( 340546, 0 ), # Tireless Pursuit buff
          ( 378989, 0 ), ( 378990, 0 ), ( 378991, 0 ), ( 378992, 0 ), # Lycara's Teachings buffs
          ( 395336, 0 ), ( 378987, 0 ), ( 400202, 0 ), ( 400204, 0 ), # Protector of the Pack
          # Balance
          ( 188046, 1 ), # Denizen of the Dream pet attack
          ( 394049, 1 ), ( 394050, 1 ), # Balance of All Things
          ( 393942, 1 ), ( 393944, 1 ), # Starweaver
          ( 395022, 1 ), # Incarnation unknown spell
          ( 393961, 1 ), # Primordial Arcanic Pulsar counter buff
          ( 394103, 1 ), ( 394106, 1 ), ( 394108, 1 ), ( 394111, 1 ), # Sundered Firmament
          ( 395110, 1 ), # Parting Skies Sundered Firmament tracker buff
          ( 393869, 1 ), # Lunar Shrapnel damage
          ( 393957, 1 ), # Waning Twilight
          ( 424248, 1 ), # Balance T31 2pc buff
          # Feral
          ( 391710, 2 ), # Ferocious Frenzy damage
          ( 391786, 2 ), # Tear Open Wounds damage
          ( 393617, 2 ), # Primal Claws energize
          ( 405069, 2 ), ( 405191, 2 ), # Overflowing Power
          # Guardian
          ( 370602, 3 ), # Elune's Favored heal
          ( 372505, 3 ), # Ursoc's Fury absorb
          ( 372019, 3 ), # Vicious Cycle mangle buff
          ( 279555, 3 ), # Layered Mane buff
          ( 395942, 3 ), # 2t29 Mangle AoE
          ( 400360, 3 ), # Moonless Night
          ( 425441, 3 ), ( 425448, 3 ), # Blazing Thorns (4t31)

          # The War Within
          # Class
          # Balance
          # Feral
          # Guardian
          # Restoration
          # Hero talents
          ( 425217, 0 ), ( 425219, 0 ), # boundless moonlight
          ( 441585, 0 ), ( 441602, 0 ), # ravage
          ( 439891, 0 ), ( 439893, 0 ), # strategic infusion
        ),
        # Demon Hunter:
        (
          # General
          ( 225102, 0 ), # Fel Eruption damage
          ( 339229, 0 ), # Serrated Glaive conduit debuff
          ( 337849, 0 ), ( 345604, 0 ), ( 346664, 0 ), # Fel Bombardment legendary spells
          ( 347765, 0 ), # Fodder to the Flame Empowered Demon Soul buff
          ( 355894, 0 ), ( 356070, 0 ), # Blind Faith legendary spells
          ( 355892, 0 ), # Blazing Slaughter legendary buff

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
          ( 333105, 1 ), ( 333110, 1 ), # Sigil of the Illidari Legendary fake Fel Devastation spells
          ( 346502, 1 ), ( 346503, 1 ), # New Sigil of the Illidari Legendary fake Fel Devastation spells
          ( 337567, 1 ), # Chaotic Blades legendary buff
          ( 390197, 1 ), # Ragefire talent damage spell
          ( 390195, 1 ), # Chaos Theory talent buff
          ( 390145, 1 ), # Inner Demon talent buff
          ( 391374, 1 ), ( 391378, 1 ), ( 393054, 1 ), ( 393055, 1 ), # First Blood Chaos spells
          ( 393628, 1 ), ( 393629, 0 ), # T29 Set Bonus Spells
          ( 408754, 1 ), # T30 4pc Seething Potential damage buff
          ( 228532, 1 ), # Consume Lesser Soul heal
          ( 328953, 1 ), # Consume Demon Soul heal
          ( 328951, 1 ), # New Shattered Souls area trigger
          ( 428493, 1 ), # Chaotic Disposition Damage

          # Vengeance
          ( 203557, 2 ), # Felblade proc rate
          ( 209245, 2 ), # Fiery Brand damage reduction
          ( 213011, 2 ), # Charred Warblades heal
          ( 212818, 2 ), # Fiery Demise debuff
          ( 207760, 2 ), # Burning Alive spread radius
          ( 333386, 2 ), ( 333389, 2 ), # Sigil of the Illidari Legendary fake Eye Beam spells
          ( 346504, 2 ), ( 346505, 2 ), # New Sigil of the Illidari Legendary fake Eye Beam spells
          ( 336640, 2 ), # Charred Flesh
          ( 203981, 2 ), # Soul Fragments
          ( 409877, 2 ), # T30 4pc Recrimination buff
          ( 428595, 2 ), # Illuminated Sigils

          # Aldrachi Reaver
          ( 442294, 0 ), # Reaver's Glaive
          ( 442624, 0 ), # Reaver's Mark
          ( 442435, 0 ), # Glaive Flurry
          ( 442442, 0 ), # Rending Strike
          ( 444806, 0 ), # Art of the Glaive damage
          ( 444810, 0 ), # Art of the Glaive container

          # Fel-scarred
          ( 453314, 0 ), # Enduring Torment
          ( 451263, 0), ( 451266, 0 ), ( 452435, 0 ), ( 452443, 0 ), ( 452449, 0 ), ( 452452, 0 ), ( 452462, 0 ), ( 452463, 0 ), ( 452489, 0 ), ( 452491, 0 ), ( 452492, 0 ), ( 452493, 0 ), ( 452499, 0 ), ( 453323, 0 ), ( 451258, 0 ), # Demonsurge
       ),

       # Evoker:
       (
          # General
          ( 372470, 0 ), # Scarlet Adaptation buff
          ( 370901, 0 ), ( 370917, 0 ), # Leaping Flames buff
          ( 359115, 0 ), # Empower Triggered GCD
          ( 361519, 0 ), # Essence Burst
          # Devastation
          ( 386399, 1 ), ( 399370, 1 ), # Iridescence: Blue
          ( 375802, 1 ), # Burnout buff
          ( 376850, 1 ), # Power Swell buff
          ( 397870, 1 ), # Titanic Wrath
          ( 405651, 1 ), # Imminent Destruction Player Buff
          ( 409848, 1 ), # 4t30 buff
          # Preservation
          ( 369299, 2 ), # Preservation Essence Burst
          # Augmentation
          ( 392268, 3 ), # Augmentation Essence Burst
          ( 404908, 3 ), ( 413786, 3 ), # Fate Mirror Damage/Heal
          ( 409632, 3 ), # Breath of Eons Damage
          ( 360828, 3 ), # Blistering Scales
          ( 410265, 3 ), # Inferno's Blessing
          ( 424368, 3 ), # T31 4pc Buff Trembling Earth
          ( 409276, 3 ), # Motes of Possibility Buff
          # Flameshaper
          ( 444249, 0 ), # Firebreath copied by Travelling Flame
          ( 444089, 0 ), # Consume Flame Fire Breath Damage
          ( 444017, 0 ), # Enkindle Damage
          # Scalecommander
          ( 441248, 0 ), # Unrelenting Siege
          ( 436336, 0 ), # Mass Disintegrate
          ( 433874, 0 ), # Deepbreath
          ( 438653, 0 ), # Mass Eruption Child Damage
          ( 438588, 0 ), # Mass Eruption Buff
          ( 442204, 0 ), # Breath of Eons
          # Chronowarden
          ( 431583, 0 ), # Chrono Flame
          ( 431620, 0 ), # Upheaval Dot
          ( 431654, 0 ), # Primacy Buff
          ( 431991, 0 ), # Time Convergence Buff
          ( 432895, 0 ), # Thread of Fate Damage
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
        ( ),         # Evoker
    ]

    # Specialization categories, Spec0 | Spec1 | Spec2
    # Note, these are reset for MoP
    _spec_skill_categories = [
        (),
        (   71,   72,   73,   0 ), # Warrior
        (   65,   66,   70,   0 ), # Paladin
        (  254,  255,  256,   0 ), # Hunter
        (  259,  260,  261,   0 ), # Rogue
        (  256,  257,  258,   0 ), # Priest
        (  250,  251,  252,   0 ), # Death Knight
        (  262,  263,  264,   0 ), # Shaman
        (   62,   63,   64,   0 ), # Mage
        (  265,  266,  267,   0 ), # Warlock
        (  268,  270,  269,   0 ), # Monk
        (  102,  103,  104, 105 ), # Druid
        (  577,  581,    0,   0 ), # Demon Hunter
        ( 1467, 1468, 1473,   0 ), # Evoker
    ]

    _race_categories = [
        (),
        ( 754 ),   # Human           0x00000001
        ( 125 ),   # Orc             0x00000002
        ( 101 ),   # Dwarf           0x00000004
        ( 126 ),   # Night-elf       0x00000008
        ( 220 ),   # Undead          0x00000010
        ( 124 ),   # Tauren          0x00000020
        ( 753 ),   # Gnome           0x00000040
        ( 733 ),   # Troll           0x00000080
        ( 790 ),   # Goblin          0x00000100? not defined yet
        ( 756 ),   # Blood elf       0x00000200
        ( 760 ),   # Draenei         0x00000400
        ( 2597 ),  # Dark Iron Dwarf 0x00000800
        ( 2775 ),  # Vulpera         0x00001000
        ( 2598 ),  # Mag'har Orc     0x00002000
        ( 2774 ),  # Mechagnome      0x00004000
        ( 2808 ),  # Dracthyr (H)    0x00008000
        (),        # Dracthyr (A)    0x00010000
        ( 2895 ),  # Earthen (H)     0x00020000
        (),        # Earthen (A)     0x00040000
        (),
        (),
        ( 789 ),   # Worgen          0x00200000
        (),        # Gilnean         0x00400000
        (),        # Pandaren (N)    0x00800000
        (),        # Pandaren (A)    0x01000000
        ( 899 ),   # Pandaren (H)    0x02000000
        ( 2419 ),  # Nightborne      0x04000000
        ( 2420 ),  # Highmountain    0x08000000
        ( 2423 ),  # Void Elf        0x10000000
        ( 2421 ),  # Lightforged     0x20000000
        ( 2721 ),  # Zandalari Troll 0x40000000
        ( 2723 ),  # Kul Tiran       0x80000000
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
        16,     # SPELL_EFFECT_QUEST_COMPLETE
        18,     # SPELL_EFFECT_RESURRECT
        25,     # SPELL_EFFECT_WEAPONS
        36,     # Learn spell
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
        ( 143, 'misc_value_1' ),
        ( 218, 'misc_value_2' ),
        ( 219, 'misc_value_2' ),
        ( 507, 'misc_value_1' ),
        ( 537, 'misc_value_1' ),
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
        345482, # Manifest Aethershunt (Shadowlands Conduit upgrade Maw item)
        345487, # Spatial Realignment Apparatus (Shadowlands Maw additional socket item)
        282965, # Shadow Priest Testing Spell (DNT)
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
        'evoker' : 224,
    }

    def initialize(self):
        if not super().initialize():
            return False

        self._data_store.link('SpellEffect',        'id_parent', 'SpellName', 'add_effect'    )
        self._data_store.link('SpellMisc',          'id_parent', 'SpellName', 'misc'          )
        self._data_store.link('SpellLevels',        'id_parent', 'SpellName', 'level'         )
        self._data_store.link('SpellCooldowns',     'id_parent', 'SpellName', 'cooldown'      )
        self._data_store.link('SpellScaling',       'id_spell',  'SpellName', 'scaling'       )
        self._data_store.link('AzeritePower',       'id_spell',  'SpellName', 'azerite_power' )

        if self._options.build >= dbc.WowVersion(8, 2, 0, 30080):
            self._data_store.link('AzeriteEssencePower', 'id_power', 'AzeriteEssence', 'powers')
            self._data_store.link('AzeriteEssencePower', 'id_spell_major_base', 'SpellName', 'azerite_essence')
            self._data_store.link('AzeriteEssencePower', 'id_spell_minor_base', 'SpellName', 'azerite_essence')
            self._data_store.link('AzeriteEssencePower', 'id_spell_major_upgrade', 'SpellName', 'azerite_essence')
            self._data_store.link('AzeriteEssencePower', 'id_spell_minor_upgrade', 'SpellName', 'azerite_essence')

        return True

    def class_mask_by_skill(self, skill):
        for i in range(0, len(constants.CLASS_SKILL_CATEGORIES)):
            if constants.CLASS_SKILL_CATEGORIES[i] == skill:
                return util.class_mask(class_id=i)

        return 0

    def class_mask_by_spec_skill(self, spec_skill):
        for i in range(0, len(self._spec_skill_categories)):
            if spec_skill in self._spec_skill_categories[i]:
                return util.class_mask(class_id=i)

        return 0

    def class_mask_by_pet_skill(self, pet_skill):
        for i in range(0, len(self._pet_skill_categories)):
            if pet_skill in self._pet_skill_categories[i]:
                return util.class_mask(class_id=i)

        return 0

    def race_mask_by_skill(self, skill):
        return util.race_mask(skill=skill)

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
        for c in spell.children('SpellCategories'):
            if c.id_mechanic in SpellDataGenerator._mechanic_blacklist:
                logging.debug("Spell id %u (%s) matches mechanic blacklist %u", spell.id, spell_name, c.id_mechanic)
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
        spell = self.db('SpellName')[spell_id]
        enabled_effects = [ True ] * ( spell.max_effect_index + 1 )

        if spell.id == 0:
            return None

        if state and not self.spell_state(spell, enabled_effects):
            return None

        if spell.id in constants.REMOVE_CLASS_AND_RACE_MASKS:
            mask_class = 0
            mask_race = 0

        filter_list[spell.id] = { 'mask_class': mask_class, 'mask_race': mask_race, 'effect_list': enabled_effects }

        if spell.id in constants.IGNORE_CLASS_AND_RACE_MASKS:
            mask_class = 0
            mask_race = 0

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

        spell_text = self.db('Spell')[spell_id]
        spell_refs = re.findall(SpellDataGenerator._spell_ref_rx, spell_text.desc or '')
        spell_refs += re.findall(SpellDataGenerator._spell_ref_rx, spell_text.tt or '')

        for link in spell.child_refs('SpellXDescriptionVariables'):
            data = link.ref('id_desc_var')
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
        for talent in TalentSet(self._options).get():
            # These may now be pet talents
            if talent.class_id > 0:
                mask_class = util.class_mask(class_id=talent.class_id)
                self.process_spell(talent.id_spell, ids, mask_class, 0, False)

        # Get all perks
        for _, perk_data in self.db('MinorTalent').items():
            if self._options.build < 25600:
                spell_id = perk_data.id_spell
            else:
                spell_id = perk_data.id_parent
            if spell_id == 0:
                continue

            spec_data = self.db('ChrSpecialization')[perk_data.id_parent]
            if spec_data.id == 0:
                continue

            self.process_spell(spell_id, ids, util.class_mask(class_id=spec_data.class_id), 0, False)

        # Get base skills from SkillLineAbility
        for ability in self.db('SkillLineAbility').values():
            mask_class_category = 0
            mask_race_category  = 0

            skill_id = ability.id_skill

            if skill_id in SpellDataGenerator._skill_category_blacklist:
                continue

            # Guess class based on skill category identifier
            mask_class_category = util.class_mask(skill=skill_id)
            if mask_class_category == 0:
                mask_class_category = self.class_mask_by_spec_skill(skill_id)

            if mask_class_category == 0:
                mask_class_category = self.class_mask_by_pet_skill(skill_id)

            # Guess race based on skill category identifier
            mask_race_category = self.race_mask_by_skill(skill_id)

            # Make sure there's a class or a race for an ability we are using
            if not ability.mask_class and not ability.mask_race and not mask_class_category and not mask_race_category:
                continue

            spell = ability.ref('id_spell')
            if not spell.id:
                continue

            self.process_spell(spell.id, ids, ability.mask_class or mask_class_category, ability.mask_race or mask_race_category)

        # Get specialization skills from SpecializationSpells and masteries from ChrSpecializations
        for spec_spell in self.db('SpecializationSpells').values():
            # Guess class based on specialization category identifier
            spec_data = spec_spell.ref('spec_id')
            if spec_data.id == 0:
                continue

            if spec_data.name == 'Initial':
                continue

            spell = spec_spell.ref('spell_id')
            if not spell.id:
                continue

            mask_class = 0
            if spec_data.class_id > 0:
                mask_class = util.class_mask(class_id=spec_data.class_id)
            # Hunter pet classes have a class id of 0, tag them as "hunter spells" like before
            else:
                mask_class = util.class_mask(class_id=Class.HUNTER)

            self.process_spell(spell.id, ids, mask_class, 0, False)

        for entry in MasterySpellSet(self._options).get():
            self.process_spell(entry.id_mastery_1, ids, util.class_mask(class_id=entry.class_id), 0, False)


        # Get spells relating to item enchants, so we can populate a (nice?) list
        for enchant_data in self.db('SpellItemEnchantment').values():
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
        for ability in self.db('SkillLineAbility').values():
            if ability.id_skill not in self._profession_enchant_categories:
                continue

            spell = ability.ref('id_spell')
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
                    item = effect.ref('item_type')
                    if not item.id or item.gem_props == 0:
                        continue

                    for item_effect in [c.ref('id_item_effect') for c in item.children('ItemXItemEffect')]:
                        enchant_spell = item_effect.ref('id_spell')
                        for enchant_effect in enchant_spell._effects:
                            if not enchant_effect or (enchant_effect.type != 53 and enchant_effect.type != 6):
                                continue

                            enchant_spell_id = enchant_spell.id
                            break

                        if enchant_spell.id > 0:
                            break
                elif effect.type == 53:
                    enchant_spell_id = spell.id

                if enchant_spell_id > 0:
                    break

            # Valid enchant, process it
            if enchant_spell_id > 0:
                self.process_spell(enchant_spell_id, ids, 0, 0)

        # Rest of the Item enchants relevant to us, such as Shoulder / Head enchants
        for item in self.db('ItemSparse').values():
            blacklist_item = False

            classdata = self.db('Item')[item.id]
            class_ = classdata.classs
            subclass = classdata.subclass
            if item.id in ItemDataGenerator._item_blacklist:
                continue

            name = item.name
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
                for item_effect in [c.ref('id_item_effect') for c in item.children('ItemXItemEffect')]:
                    spell = item_effect.ref('id_spell')
                    for effect in spell._effects:
                        if not effect or effect.type != 53:
                            continue

                        self.process_spell(spell.id, ids, 0, 0)
            # Grab relevant spells from consumables as well
            elif class_ == 0:
                for item_effect in [c.ref('id_item_effect') for c in item.children('ItemXItemEffect')]:
                    spell = item_effect.ref('id_spell')
                    if not spell.id:
                        continue

                    # Potions and Elixirs need to apply attributes, rating or
                    # armor
                    if classdata.has_value('subclass', [1, 2, 3]) and spell.has_effect('sub_type', [13, 22, 29, 99, 189, 465, 43]):
                        self.process_spell(spell.id, ids, 0, 0)
                    # Food needs to have a periodically triggering effect
                    # (presumed to be always a stat giving effect)
                    elif classdata.has_value('subclass', 5) and spell.has_effect('sub_type', 23):
                        self.process_spell(spell.id, ids, 0, 0)
                    # Permanent enchants
                    elif classdata.has_value('subclass', 6):
                        self.process_spell(spell.id, ids, 0, 0)

                    # Finally, check consumable whitelist
                    map_ = constants.CONSUMABLE_ITEM_WHITELIST.get(classdata.subclass, {})
                    item = item_effect.child_ref('ItemXItemEffect').parent_record()
                    if item.id in map_:
                        self.process_spell(spell.id, ids, 0, 0)

            # Hunter scopes and whatnot
            elif class_ == 7:
                if classdata.has_value('subclass', 3):
                    for item_effect in [c.ref('id_item_effect') for c in item.children('ItemXItemEffect')]:
                        spell = item_effect.ref('id_spell')
                        for effect in spell._effects:
                            if not effect:
                                continue

                            if effect.type == 53:
                                self.process_spell(spell.id, ids, 0, 0)

        # Relevant set bonuses
        for set_spell_data in self.db('ItemSetSpell').values():
            if not SetBonusListGenerator.is_extract_set_bonus(set_spell_data.id_parent)[0]:
                continue

            mask_class = 0
            spec_data = set_spell_data.ref('id_spec')
            if spec_data.id > 0:
                mask_class = DataGenerator._class_masks[spec_data.class_id]

            self.process_spell(set_spell_data.id_spell, ids, mask_class, 0)

        # Glyph effects, need to do trickery here to get actual effect from spellbook data
        for ability in self.db('SkillLineAbility').values():
            if ability.id_skill != 810 or not ability.mask_class:
                continue

            use_glyph_spell = ability.ref('id_spell')
            if not use_glyph_spell.id:
                continue

            # Find the on-use for glyph then, misc value will contain the correct GlyphProperties.dbc id
            for effect in use_glyph_spell._effects:
                if not effect or effect.type != 74: # Use glyph
                    continue

                # Filter some erroneous glyph data out
                glyph_data = self.db('GlyphProperties')[effect.misc_value_1]
                spell_id = glyph_data.id_spell
                if not glyph_data.id or not spell_id:
                    continue

                self.process_spell(spell_id, ids, ability.mask_class, 0)

        # Item enchantments that use a spell
        for data in self.db('SpellItemEnchantment').values():
            for attr_id in range(1, 4):
                attr_type = getattr(data, 'type_%d' % attr_id)
                if attr_type == 1 or attr_type == 3 or attr_type == 7:
                    sid = getattr(data, 'id_property_%d' % attr_id)
                    self.process_spell(sid, ids, 0, 0)

        # Items with a spell identifier as "stats"
        for item in self.db('ItemSparse').values():
            # Allow neck, finger, trinkets, weapons, 2hweapons to bypass ilevel checking
            ilevel = item.ilevel
            if item.inv_type not in [ 2, 11, 12, 13, 15, 17, 21, 22, 26 ] and \
               (ilevel < self._options.min_ilevel or ilevel > self._options.max_ilevel):
                continue

            for item_effect in [c.ref('id_item_effect') for c in item.children('ItemXItemEffect')]:
                spell = item_effect.ref('id_spell')
                if spell.id == 0:
                    continue

                self.process_spell(spell.id, ids, 0, 0)

        for data in self.db('AzeritePowerSetMember').values():
            power = data.ref('id_power')
            if power.id != data.id_power:
                continue

            spell = power.ref('id_spell')
            if spell.id != power.id_spell:
                continue

            self.process_spell(spell.id, ids, 0, 0, False)
            # for spelldatadump readability, we no longer assign a class to azerite
            # if spell.id in ids:
            #    mask_class = self._class_masks[data.class_id] or 0
            #    ids[spell.id]['mask_class'] |= mask_class

        # Azerite esssence spells
        for data in self.db('AzeriteItemMilestonePower').values():
            power = data.ref('id_power')
            if power.id != data.id_power:
                continue

            spell = power.ref('id_spell')
            if spell.id != power.id_spell:
                continue

            self.process_spell(spell.id, ids, 0, 0, False)

        for data in self.db('AzeriteEssencePower').values():
            self.process_spell(data.id_spell_major_base, ids, 0, 0, False)
            self.process_spell(data.id_spell_major_upgrade, ids, 0, 0, False)
            self.process_spell(data.id_spell_minor_base, ids, 0, 0, False)
            self.process_spell(data.id_spell_minor_upgrade, ids, 0, 0, False)

        # Get dynamic item effect spells
        for data in self.db('ItemBonus').values():
            if data.type != 23:
                continue

            effect = self.db('ItemEffect')[data.val_1]
            if effect.id == 0:
                continue

            self.process_spell(effect.id_spell, ids, 0, 0, False)

        # Soulbind trees abilities
        for spell_id in SoulbindAbilitySet(self._options).ids():
            self.process_spell(spell_id, ids, 0, 0)

        # Covenant abilities
        for spell_id in CovenantAbilitySet(self._options).ids():
            self.process_spell(spell_id, ids, 0, 0)

        # Souldbind conduits
        for spell_id in ConduitSet(self._options).ids():
            self.process_spell(spell_id, ids, 0, 0)

        # Renown rewards
        for spell_id in RenownRewardSet(self._options).ids():
            self.process_spell(spell_id, ids, 0, 0)

        # Explicitly add Shadowlands legendaries
        for entry in self.db('RuneforgeLegendaryAbility').values():
            self.process_spell(entry.id_spell, ids, 0, 0)

        # Dragonflight player traits
        for data in TraitSet(self._options).get().values():
            class_mask = util.class_mask(class_id=data['class_'])

            self.process_spell(data['spell'].id, ids, class_mask, 0, False)

            if data['definition'].id_replace_spell > 0:
                self.process_spell(data['definition'].id_replace_spell, ids, class_mask, 0, False)

            if data['definition'].id_override_spell > 0:
                self.process_spell(data['definition'].id_override_spell, ids, class_mask, 0, False)

        # Temporary item enchants
        for item, spell, enchant_id, rank in TemporaryEnchantItemSet(self._options).get():
            enchant = self.db('SpellItemEnchantment')[enchant_id]
            enchant_spells = []
            for index in range(1, 4):
                type_ = getattr(enchant, 'type_{}'.format(index))
                prop_ = getattr(enchant, 'id_property_{}'.format(index))
                if type_ in [1, 3, 7] and prop_:
                    enchant_spell = self.db('SpellName')[prop_]
                    if enchant_spell.id != 0:
                        enchant_spells.append(self.db('SpellName')[prop_])

            for s in [spell] + enchant_spells:
                self.process_spell(s.id, ids, 0, 0)

        # Last, get the explicitly defined spells in _spell_id_list on a class basis and the
        # generic spells from SpellDataGenerator._spell_id_list[0]
        for generic_spell_id in SpellDataGenerator._spell_id_list[0]:
            if generic_spell_id in ids.keys():
                spell = self.db('SpellName')[generic_spell_id]
                logging.debug('Whitelisted spell id %u (%s) already in the list of spells to be extracted',
                    generic_spell_id, spell.name)
            self.process_spell(generic_spell_id, ids, 0, 0)

        for cls in range(1, len(SpellDataGenerator._spell_id_list)):
            for spell_tuple in SpellDataGenerator._spell_id_list[cls]:
                if len(spell_tuple) == 2 and spell_tuple[0] in ids.keys():
                    spell = self.db('SpellName')[spell_tuple[0]]
                    logging.debug('Whitelisted spell id %u (%s) already in the list of spells to be extracted',
                        spell_tuple[0], spell.name)
                self.process_spell(spell_tuple[0], ids, self._class_masks[cls], 0)

        for spell_id, spell_data in self.db('SpellName').items():
            for pattern in SpellDataGenerator._spell_name_whitelist:
                if pattern.match(spell_data.name):
                    self.process_spell(spell_id, ids, 0, 0)

        # After normal spells have been fetched, go through all spell ids,
        # and get all the relevant aura_ids for selected spells
        more_ids = { }
        for spell_id, spell_data in ids.items():
            spell = self.db('SpellName')[spell_id]
            for power in spell.children('SpellPower'):
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

        # Spells with description that is entirely in the format $@spelldesc###### are assumed to be associated
        # with spell ###### and should be included.
        for spell_id, spell_data in self.db('Spell').items():
            if spell_data.desc:
                r = re.match(r"\$@spell(?:aura|desc)([0-9]{1,6})", spell_data.desc)
                if r and (id := int(r.group(1))) in ids:
                    self.process_spell(spell_id, ids, ids[id]['mask_class'], ids[id]['mask_race'])

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

        for label in constants.SPELL_LABEL_WHITELIST:
            included_labels.add(label)

        labels = []
        for label in self.db('SpellLabel').values():
            if label.id_parent not in id_keys:
                continue

            if label.label in constants.SPELL_LABEL_BLACKLIST:
                continue

            label_tuple = (label.id_parent, label.label, label)
            if label_tuple not in labels:
                labels.append(label_tuple)
                spelllabel_index[label.id_parent] += 1

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
            spell = self.db('SpellName')[id]

            # Unused hotfix IDs: 7,
            # MAX hotfix id: 39
            hotfix = HotfixDataRecord()
            power_count = 0

            if not spell.id and id > 0:
                sys.stderr.write('Spell id %d not found\n') % id
                continue

            spelldata_array_position[spell.id] = index

            for power in spell.children('SpellPower'):
                power_count += 1
                powers.append( power )

            misc = spell.get_link('misc')

            #if index % 20 == 0:
            #  self._out.write('//{ Name                                ,     Id,             Hotfix,PrjSp,  Sch, Class,  Race,Sca,MSL,SpLv,MxL,MinRange,MaxRange,Cooldown,  GCD,Chg, ChrgCd, Cat,  Duration,  RCost, RPG,Stac, PCh,PCr, ProcFlags,EqpCl, EqpInvType,EqpSubclass,CastMn,CastMx,Div,       Scaling,SLv, RplcId, {      Attr1,      Attr2,      Attr3,      Attr4,      Attr5,      Attr6,      Attr7,      Attr8,      Attr9,     Attr10,     Attr11,     Attr12 }, {     Flags1,     Flags2,     Flags3,     Flags4 }, Family, Description, Tooltip, Description Variable, Icon, ActiveIcon, Effect1, Effect2, Effect3 },\n')

            fields = spell.field('name', 'id')
            hotfix.add(spell, ('name', 0))

            fields += misc.field('school', 'proj_speed', 'proj_delay', 'proj_min_duration')
            hotfix.add(misc, ('proj_speed', 3), ('school', 4), ('proj_delay', 50), ('proj_min_duration', 51))

            # Hack in the combined class from the id_tuples dict
            fields += [ u'%#.16x' % ids.get(id, { 'mask_class' : 0, 'mask_race': 0 })['mask_race'] ]
            fields += [ u'%#.8x' % ids.get(id, { 'mask_class' : 0, 'mask_race': 0 })['mask_class'] ]

            scaling_entry = spell.get_link('scaling')
            fields += scaling_entry.field('max_scaling_level')
            hotfix.add(scaling_entry, ('max_scaling_level', 8))

            level_entry = spell.get_link('level')

            # Simulationcraft does not really support the concept of "Learn"
            # and "Required" level, so grab the highest of the two for level
            # check purposes.
            req_level = max(level_entry.base_level, level_entry.spell_level)
            fields += [level_entry.field_format('base_level')[0] % req_level]

            fields += level_entry.field('max_level', 'req_max_level')
            hotfix.add(level_entry, ('base_level', 9), ('max_level', 10), ('req_max_level', 46), ('spell_level', 49))

            range_entry = misc.ref('id_range')
            fields += range_entry.field('min_range_1', 'max_range_1')
            hotfix.add(range_entry, ('min_range_1', 11), ('max_range_1', 12))

            cooldown_entry = spell.get_link('cooldown')
            fields += cooldown_entry.field('cooldown_duration', 'gcd_cooldown', 'category_cooldown')
            hotfix.add(cooldown_entry, ('cooldown_duration', 13), ('gcd_cooldown', 14), ('category_cooldown', 15))

            category = spell.child('SpellCategories')
            category_data = category.ref('id_charge_category')

            fields += category_data.field('charges', 'charge_cooldown')
            hotfix.add(category_data, ('charges', 16), ('charge_cooldown', 17))
            if category.id_charge_category > 0: # Note, some spells have both cooldown and charge categories
                fields += category.field('id_charge_category')
                hotfix.add(category, ('id_charge_category', 18))
            else:
                fields += category.field('id_cooldown_category')
                hotfix.add(category, ('id_cooldown_category', 18))
            fields += category.field('dmg_class')
            hotfix.add(category, ('dmg_class', 47))

            fields += spell.child('SpellTargetRestrictions').field('max_affected_targets')
            hotfix.add(spell.child('SpellTargetRestrictions'), ('max_affected_targets', 48))

            duration_entry = misc.ref('id_duration')
            fields += duration_entry.field('duration_1')
            hotfix.add(duration_entry, ('duration_1', 19))

            aura_opt_entry = spell.child('SpellAuraOptions')
            fields += aura_opt_entry.field('stack_amount', 'proc_chance', 'proc_charges')
            fields.append(f'{(aura_opt_entry.proc_flags_1 | aura_opt_entry.proc_flags_2 << 32):#018x}')
            fields += aura_opt_entry.field('internal_cooldown')
            hotfix.add(aura_opt_entry,
                    ('stack_amount', 20), ('proc_chance', 21), ('proc_charges', 22),
                    ('proc_flags_1', 23), ('proc_flags_2', 52), ('internal_cooldown', 24))

            ppm_entry = aura_opt_entry.ref('id_ppm')
            fields += ppm_entry.field('ppm')
            hotfix.add(ppm_entry, ('ppm', 25))

            equipped_item_entry = spell.child_ref('SpellEquippedItems')
            fields += equipped_item_entry.field('item_class', 'mask_inv_type', 'mask_sub_class')
            hotfix.add(equipped_item_entry,
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

            class_opt_entry = spell.child_ref('SpellClassOptions', field='id_spell')
            fields += [ '{ %s }' % ', '.join(class_opt_entry.field('flags_1', 'flags_2', 'flags_3', 'flags_4')) ]
            fields += class_opt_entry.field('family')
            hotfix.add(class_opt_entry,
                (('flags_1', 'flags_2', 'flags_3', 'flags_4'), 36), ('family', 37))

            shapeshif_entry = spell.child_ref('SpellShapeshift')
            fields += shapeshif_entry.field('flags_1')
            hotfix.add(shapeshif_entry, ('flags_1', 38))

            fields += category.field('id_mechanic')
            hotfix.add(category, ('id_mechanic', 39))

            power = spell.get_link('azerite_power')
            fields += power.field('id')

            # 8.2.0 Azerite essence stuff
            if self._options.build >= dbc.WowVersion(8, 2, 0, 30080):
                essences = [x.field('id_essence')[0] for x in spell.get_links('azerite_essence')]
                if len(essences) == 0:
                    fields += self.db('AzeriteEssence')[0].field('id')
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

            # MAX hotfix id 29
            hotfix = HotfixDataRecord()

            fields = effect.field('id', 'id_parent', 'index', 'type', 'sub_type')
            hotfix.add(effect, ('index', 3), ('type', 4), ('sub_type', 5))

            fields += effect.field('id_scaling_class')
            hotfix.add(effect, ('id_scaling_class', 28))

            fields += [f'{effect.attribute:#010x}']
            hotfix.add(effect, ('attribute', 29))

            fields += effect.field('coefficient', 'delta', 'bonus', 'sp_coefficient',
                    'ap_coefficient', 'amplitude')
            hotfix.add(effect, ('coefficient', 6), ('delta', 7), ('bonus', 8),
                    ('sp_coefficient', 9), ('ap_coefficient', 10), ('amplitude', 11))

            radius_entry = effect.ref('id_radius_1')
            fields += radius_entry.field('radius_1')
            hotfix.add(radius_entry, ('radius_1', 12))

            radius_max_entry = effect.ref('id_radius_2')
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

            fields += effect.field('id_mechanic')
            hotfix.add(effect, ('id_mechanic', 22))

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
                header = 'Spell labels',
                type = 'spelllabel_data_t',
                array = 'spelllabel',
                length = len(labels))

        for _, _, label in sorted(labels, key=lambda l: (l[0], l[1])):
            self.output_record(label.field('id_parent', 'label'))

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

        for chrspec, spell_data in sorted(data, key = lambda e: (e[0].class_id, e[0].index, e[0].id, e[1].spell_id)):
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
    # "set_bonus_type_e" enumeration in sc_enum.hpp or very bad
    # things will happen.
    # ====================================================================
    # NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
    set_bonus_map = [
        # Legion Dungeon, March of the Legion
        {
            'name'   : 'march_of_the_legion',
            'bonuses': [ 1293 ],
            'tier'   : 'T19_MOTL'
        },
        # Legion Dungeon, Journey Through Time
        {
            'name'   : 'journey_through_time',
            'bonuses': [ 1294 ],
            'tier'   : 'T19_JTT'
        },
        # Legion Dungeon, Cloth
        {
            'name'   : 'tier19p_cloth',
            'bonuses': [ 1295 ],
            'tier'   : 'T19_C'
        },
        # Legion Dungeon, Leather
        {
            'name'   : 'tier19p_leather',
            'bonuses': [ 1296 ],
            'tier'   : 'T19_L'
        },
        # Legion Dungeon, Mail
        {
            'name'   : 'tier19p_mail',
            'bonuses': [ 1297 ],
            'tier'   : 'T19_M'
        },
        # Legion Dungeon, Plate
        {
            'name'   : 'tier19p_plate',
            'bonuses': [ 1298 ],
            'tier'   : 'T19_P'
        },
        {
            'name'   : 'waycrests_legacy',
            'bonuses': [ 1439 ],
            'tier'   : 'T21_WL'
        },
        {
            'name'   : 'gift_of_the_loa',
            'bonuses': [ 1442 ],
            'tier'   : 'T23_GOTL'
        },
        {
            'name'   : 'keepsakes',
            'bonuses': [ 1443 ],
            'tier'   : 'T23_KS'
        },
        {
            'name'   : 'titanic_empowerment',
            'bonuses': [ 1452 ],
            'tier'   : 'T24_TE'
        },
        {
            'name'   : 'hack_and_gore',
            'bonuses': [ 1457 ],
            'tier'   : 'T26_HG'
        },
        {
            'name'   : 'tier28',
            'bonuses': [ 1496, 1497, 1498, 1499, 1500, 1501, 1502, 1503, 1504, 1505, 1506, 1507],
            'tier'   : 'T28'
        },
        {
            'name'   : 'ripped_secrets',
            'bonuses': [ 1508 ],
            'tier'   : 'T28_RS'
        },
        {
            'name'   : 'tier29',
            'bonuses': [ 1526, 1527, 1528, 1529, 1530, 1531, 1532, 1533, 1534, 1535, 1536, 1537, 1538 ],
            'tier'   : 'T29'
        },
        {
            'name'   : 'playful_spirits_fur',
            'bonuses': [ 1509 ],
            'tier'   : 'T29_PSF'
        },
        {
            'name'   : 'horizon_striders_garments',
            'bonuses': [ 1510 ],
            'tier'   : 'T29_HSG'
        },
        {
            'name'   : 'azureweave_vestments',
            'bonuses': [ 1516 ],
            'tier'   : 'T29_AV'
        },
        {
            'name'   : 'woven_chronocloth',
            'bonuses': [ 1515 ],
            'tier'   : 'T29_WC'
        },
        {
            'name'   : 'raging_tempests',
            'bonuses': [ 1521, 1523, 1524, 1525 ],
            'tier'   : 'T29_RT'
        },
        {
            'name'   : 'tier30',
            'bonuses': [ 1540, 1541, 1542, 1543, 1544, 1545, 1546, 1547, 1548, 1549, 1550, 1551, 1552 ],
            'tier'   : 'T30'
        },
        {
            'name'   : 'might_of_the_drogbar',
            'bonuses': [ 1539 ],
            'tier'   : 'T30_MOTD'
        },
        {
            'name'   : 'tier31',
            'bonuses': [ 1557, 1558, 1559, 1560, 1561, 1562, 1563, 1564, 1565, 1566, 1567, 1568, 1569 ],
            'tier'   : 'T31'
        },
        {
            'name'   : 'dragonflight_season_4',
            'bonuses': [ 1594, 1595, 1596, 1597, 1598, 1599, 1600, 1601, 1602, 1603, 1604, 1605, 1606 ],
            'tier'   : 'DF4'
        },
        {
            'name'   : 'embrace_of_the_cinderbee',
            'bonuses': [ 1611 ],
            'tier'   : 'TWW_ECB'
        },
        {
            'name'   : 'fury_of_the_storm_rook',
            'bonuses': [ 1612 ],
            'tier'   : 'TWW_FSR'
        },
        {
            'name'   : 'kyevezzas_cruel_implements',
            'bonuses': [ 1613 ],
            'tier'   : 'TWW_KCI'
        },
        {
            'name'   : 'thewarwithin_season_1',
            'bonuses': [ 1684, 1685, 1686, 1687, 1688, 1689, 1690, 1691, 1692, 1693, 1694, 1695, 1696 ],
            'tier'   : 'TWW1'
        },
        {
            'name'   : 'woven_dusk',
            'bonuses': [ 1697 ],
            'tier'   : 'TWW_WDusk'
        },
        {
            'name'   : 'woven_dawn',
            'bonuses': [ 1683 ],
            'tier'   : 'TWW_WDawn'
        },
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
            '{: <43}', '{: <27}', '{: <10}', '{: <6}', '{: <5}', '{: <3}', '{: <3}', '{: <4}', '{: <7}', '{}'
        )

        _data_specifiers = (
            '{: <44}', '{: <27}', '{: >10}', '{: >6}', '{: >5}', '{: >3}', '{: >3}', '{: >4}', '{: >7}', '{}'
        )

        _hdr_format = ', '.join(_hdr_specifiers)
        _hdr = _hdr_format.format('SetBonusName', 'OptName', 'Tier', 'EnumID', 'SetID', 'Bns', 'Cls', 'Spec', 'SpellID', 'ItemIDs')

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
                '"%s"' % map_entry['tier'].replace('"', '\\"'),
                entry['index'],
                set_id,
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
                                 'min_item_level', 'max_item_level', 'req_skill',
                                 'req_skill_value')
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
            fields += [ '{ %s }' % ', '.join( rpp.field('epic_points_1', 'epic_points_2', 'epic_points_3', 'epic_points_4', 'epic_points_5')) ]
            fields += [ '{ %s }' % ', '.join( rpp.field('rare_points_1', 'rare_points_2', 'rare_points_3', 'rare_points_4', 'rare_points_5')) ]
            fields += [ '{ %s }' % ', '.join( rpp.field('uncm_points_1', 'uncm_points_2', 'uncm_points_3', 'uncm_points_4', 'uncm_points_5')) ]

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
            fields = entry.field('id', 'id_enchant', 'color')

            # it's possible to have gem items with different descriptors sharing the same gem propery.
            # as we are mainly interested in parsing the 'color' of standard gems, only output 1:1 matches of gem
            # property to descriptor.
            descs = entry.child_refs('ItemSparse')
            fields += descs[0].field('id_name_desc') if len(descs) == 1 else ['0']

            self.output_record(fields, comment = entry.ref('id_enchant').id and entry.ref('id_enchant').desc or '')

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
        for item in super().filter():
            ids.extend([c.ref('id_item_effect') for c in item.children('ItemXItemEffect')])

        return list(set(ids))

    def generate(self, data = None):
        item_effect_id_index = []

        self.output_header(
                header = 'Item effects',
                type = 'item_effect_t',
                array = 'item_effect',
                length = len(data))

        for index, entry in enumerate(sorted(data, key = lambda e: (e.child_ref('ItemXItemEffect').parent_record().id, e.index, e.id))):
            item_effect_id_index.append((entry.id, index))
            item = entry.child_ref('ItemXItemEffect').parent_record()

            fields = entry.field('id', 'id_spell')
            fields += item.field('id')
            fields += entry.field('index', 'trigger_type', 'cooldown_group', 'cooldown_duration', 'cooldown_group_duration')

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

        self._out.write('#define {} "{}"\n'.format(
            self.format_str('SIMC_WOW_VERSION').upper(),
            '{}{}{}'.format(self._options.build.expansion(), self._options.build.patch(),
                self._options.build.minor())))

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
        data.sort(key = lambda v: isinstance(v[0], int) and (v[0], 0, v[1].id) or (v[0].class_id, v[0].id, v[1].id))

        self.output_header(
                header = 'Active class spells',
                type = 'active_class_spell_t',
                array = 'active_spells',
                length = len(data))

        for spec_data, spell, replace_spell in data:
            fields = []
            if isinstance(spec_data, int):
                fields += ['{:2d}'.format(spec_data), '{:4d}'.format(0)]
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

        name_re = re.compile(r'^(?:Pet[\s]+\-[\s]+|)(.+)', re.I)
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
            fields += entry.field('id_spell', 'mask_inv_type', 'id_covenant', 'name')
            self.output_record(fields)

        self.output_footer()

class RankSpellGenerator(DataGenerator):
    def filter(self):
        return RankSpellSet(self._options).get()

    def generate(self, data = None):
        data.sort(key = lambda v: (v[2].name, self.db('Spell')[v[2].id].rank, v[0], v[1], v[2].id))

        self.output_header(
                header = 'Rank class spells',
                type = 'rank_class_spell_t',
                array = 'rank_spells',
                length = len(data))

        for class_id, spec_id, spell, replace_spell in data:
            fields = []
            fields += ['{:2d}'.format(class_id), '{:4d}'.format(spec_id)]
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
            fields += garr_talent.parent_record().field('id')
            fields += garr_talent.ref('id_spell').field('name')

            self.output_record(fields)

        self.output_footer()

class CovenantAbilityGenerator(DataGenerator):
    def filter(self):
        return CovenantAbilitySet(self._options).get()

    def generate(self, data=None):
        data.sort(key = lambda v: v.ref('id_spell').name)

        self.output_header(
                header = 'Covenant abilities',
                type = 'covenant_ability_entry_t',
                array = 'covenant_ability',
                length = len(data))

        for entry in data:
            class_id = util.class_id(family = entry.ref('id_spell').child_ref('SpellClassOptions').family)

            fields = ['{:2d}'.format(class_id > -1 and class_id or 0)]
            fields += entry.ref('id_covenant_preview').field('id_covenant')
            fields += entry.field('ability_type')
            fields += entry.ref('id_spell').field('id', 'name')

            self.output_record(fields)

        self.output_footer()

class RenownRewardGenerator(DataGenerator):
    def filter(self):
        return RenownRewardSet(self._options).get()

    def generate(self, data=None):
        data.sort(key = lambda v: (v.id_covenant, v.level, v.id_spell))

        self.output_header(
                header = 'Renown reward abilities',
                type = 'renown_reward_entry_t',
                array = 'renown_reward_ability',
                length = len(data))

        for entry in data:
            fields = entry.field('id_covenant', 'level')
            fields += entry.ref('id_spell').field('id', 'name')

            self.output_record(fields, comment = entry.ref('id_covenant').name)

        self.output_footer()

class TemporaryEnchantItemGenerator(DataGenerator):
    def filter(self):
        return TemporaryEnchantItemSet(self._options).get()

    def generate(self, data=None):

        self.output_header(
                header = 'Temporary item enchants',
                type = 'temporary_enchant_entry_t',
                array = 'temporary_enchant',
                length = len(data))

        for item, spell, enchant_id, rank in sorted(data, key = lambda v: (v[0].name, v[3], v[2])):
            fields = ['{:5d}'.format(enchant_id)]
            fields += ['{:2d}'.format(rank)]
            fields += spell.field('id')
            fields += ['{:30s}'.format('"{}"'.format(util.tokenize(item.name)))]
            self.output_record(fields)

        self.output_footer()

class SoulbindConduitEnhancedSocketGenerator(DataGenerator):
    def generate(self, data=None):
        data = self.db('SoulbindConduitEnhancedSocket').values()

        self.output_header(
                header = 'Enhanced conduit renown levels',
                type = 'enhanced_conduit_entry_t',
                array = 'enhanced_conduit',
                length = len(data))

        soulbind_lookup = {}
        for soulbind in self.db('Soulbind').values():
            soulbind_lookup[soulbind.id_garr_talent_tree] = soulbind

        field_data = []
        for entry in data:
            slot = entry.ref('id_garr_talent')
            soulbind = soulbind_lookup[slot.id_garr_talent_tree]
            player_cond = entry.ref('id_player_cond')
            for i in range(1, 5):
                # We are only interested in Renown currency requirements.
                if getattr(player_cond, 'id_currency_{}'.format(i)) == 1822:
                    # Add 1 to convert from currency count to renown level
                    renown = getattr(player_cond, 'currency_count_{}'.format(i)) + 1
                    fields = ['{:2d}'.format(soulbind.id)]
                    fields += slot.field('tier', 'ui_order')
                    fields.append('{:3d}'.format(renown))
                    fields += soulbind.field('name')
                    field_data.append(fields)
                    break

        field_data.sort(key = lambda f: [int(v) for v in f[:-1]])
        for f in field_data:
            self.output_record(f[:-1], comment = f[-1].strip('"'))

        self.output_footer()

class TraitGenerator(DataGenerator):
    def filter(self):
        return TraitSet(self._options).get()

    def _generate_table(self, data, subtrees):
        for entry in data:
            fields = []

            fields.append(f'{entry["tree"]}')
            fields.append(f'{entry["class_"]:2d}')
            fields += entry['entry'].field('id')
            fields += entry['node'].field('id')
            fields += entry['entry'].field('max_ranks')
            fields.append(f'{entry["req_points"]:2d}')
            fields += entry['definition'].field('id')
            fields += entry['spell'].field('id')
            fields += entry['definition'].field('id_replace_spell')
            fields += entry['definition'].field('id_override_spell')
            fields += [f'{entry["row"]:2d}', f'{entry["col"]:2d}']
            fields.append(f'{entry["selection_index"]:3d}')

            _name = entry['spell'].name
            if override := entry['definition'].override_name:
                _name = override
                if override.startswith('$@spellname') and override[11:].isnumeric():
                    _name = self.db('SpellName')[int(override[11:])].name

            fields.append("{:>40s}".format(f'"{_name}"'))
            fields.append(f'{{ {", ".join(["{:4d}".format(x) for x in sorted(entry["specs"]) + [0] * (constants.MAX_SPECIALIZATION - len(entry["specs"]))])} }}')
            fields.append(f'{{ {", ".join(["{:4d}".format(x) for x in sorted(entry["starter"]) + [0] * (constants.MAX_SPECIALIZATION - len(entry["starter"]))])} }}')

            _subtree = entry['entry'].id_trait_sub_tree if entry['tree'] == 4 else entry['node'].id_trait_sub_tree
            if _subtree != 0:
                subtrees.add(_subtree)

            fields.append(f'{_subtree:2d}')
            fields += entry['node'].field('type')

            self.output_record(fields)

    def generate(self, data=None):
        sorted_data = sorted(
            data.values(),
            key=lambda v: (v['tree'], v['class_'], v['entry'].id)
        )

        subtrees = set()

        self.output_header(
                header='Player trait definitions',
                type='trait_data_t',
                array='trait_data',
                length=len(data))

        self._generate_table(sorted_data, subtrees)

        self.output_footer()

        # Definition effects
        definition_effects = [
            e for entry in sorted_data
                for e in entry['definition'].child_refs('TraitDefinitionEffectPoints')
        ]

        definition_effects.sort(key=lambda v: (v.id_parent, v.effect_index))

        self.output_header(
                header='Player trait definition effect entries',
                type='trait_definition_effect_entry_t',
                array='trait_definition_effect',
                length=len(definition_effects)
        )

        for e in definition_effects:
            self.output_record(e.field('id_trait_definition', 'effect_index', 'operation', 'id_curve'))

        self.output_footer()

        # Collect spell IDs for indices
        trait_spell_index = list((v['spell'].id, index) for index, v in enumerate(sorted_data))

        self.output_id_index(
            index = [ index for _, index in sorted(trait_spell_index) ],
            array = 'trait_spell')

        # Hero trees
        self.output_header(
            header='Hero trees',
            type='std::pair<unsigned, std::string>',
            array='trait_sub_tree',
            length=len(subtrees)
        )

        for e in sorted(subtrees):
            self.output_record([str(e), '"{}"'.format(self.db('TraitSubTree')[e].name)])

        self.output_footer()

        """
        print(
            f'cls={entry["class_"]} specs={entry["specs"]} starter={entry["starter"]} '
            f'groups=[{", ".join([str(x.id) for x in sorted(entry["groups"], key=lambda v:v.id)])}] '
            f'tree={entry["tree"]} node_id={entry["node"].id} node_type={entry["node"].type} '
            f'entry={entry["entry"].id} '
            f'replace={entry["definition"].id_replace_spell} '
            f'override={entry["definition"].id_override_spell} '
            f'spell={entry["spell"].name} ({entry["spell"].id}) ({util.tokenize(entry["spell"].name)})'
        )
        """

class PermanentEnchantItemGenerator(DataGenerator):
    def filter(self):
        return PermanentEnchantItemSet(self._options).get()

    def generate(self, data=None):
        self.output_header(
            header = 'Permanent item enchants',
            type = 'permanent_enchant_entry_t',
            array = 'permanent_enchant',
            length = len(data))

        for spell_name, rank, _, _, _, _, enchant, sei in sorted(data, key=lambda v: (v[0], v[1], v[3], v[4], v[5])):
            fields = enchant.field('id')
            fields += ['{:2d}'.format(rank)]
            fields += sei.field('item_class', 'mask_inv_type', 'mask_sub_class')
            fields += ['{:35s}'.format('"{}"'.format(spell_name))]
            self.output_record(fields)

        self.output_footer()

class ExpectedStatGenerator(DataGenerator):
    def filter(self):
        return [ v for v in self.db('ExpectedStat').values() if v.id_expansion == -2 ]

    def generate(self, data = None):
        self.output_header(
            header = 'Expected stat for levels 1-{}'.format(data[-1].id_parent),
            type = 'expected_stat_t',
            array = 'expected_stat',
            length = len(data))

        for es in sorted(data, key = lambda e: e.id_parent):
            fields = es.field('id_parent', 'creature_auto_attack_dps', 'creature_armor',
                              'player_primary_stat', 'player_secondary_stat',
                              'armor_constant', 'creature_spell_damage')
            self.output_record(fields)

        self.output_footer()

class ExpectedStatModGenerator(DataGenerator):
    def filter(self):
        return ExpectedStatModSet(self._options).get()

    def generate(self, data = None):
        self.output_header(
            header = 'Expected stat mods',
            type = 'expected_stat_mod_t',
            array = 'expected_stat_mod',
            length = len(data))

        for esm in sorted(data, key = lambda e: (e[1], e[0].id)):
            fields = esm[0].field('id', 'mod_creature_auto_attack_dps', 'mod_creature_armor',
                                  'mod_player_primary_stat', 'mod_player_secondary_stat',
                                  'mod_armor_constant', 'mod_creature_spell_damage')
            fields += [str(esm[1])]

            self.output_record(fields)

        self.output_footer()

class EmbellishmentGenerator(DataGenerator):
    def filter(self):
        return EmbellishmentSet(self._options).get()

    def generate(self, data = None):
        self.output_header(
            header = 'Embellishment data',
            type = 'embellishment_data_t',
            array = 'embellishment',
            length = len(data))

        for emb in data:
            fields = [str(emb[0]), str(emb[1]), str(emb[2])]
            self.output_record(fields)

        self.output_footer()

class CharacterLoadoutGenerator(DataGenerator):
    def filter(self):
        return CharacterLoadoutSet(self._options).get()

    def generate(self, data = None):
        # assume target mythic ilevel is highest heroic dungeon find minium ilevel + 5 * 13
        _ilevels = [e.heroic_lfg_dungeon_min_gear for e in self.db('MythicPlusSeason').values()]
        _ilevels.sort(reverse=True)
        self._out.write('static constexpr int {}MYTHIC_TARGET_ITEM_LEVEL = {};\n\n'.format(
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            _ilevels[0] + 5 * 13))

        self.output_header(
            header = 'Character Loadout data',
            type = 'character_loadout_data_t',
            array = 'character_loadout',
            length = len(data))

        spec_idx = defaultdict(list)

        for _, loadout in data:
            spec_idx[loadout.id_class].append(loadout.id)

        for k, v in spec_idx.items():
            spec_idx[k] = sorted(list(set(v)))

        for cli, loadout in data:
            fields = loadout.field('id', 'id_class')
            fields += [str(spec_idx[loadout.id_class].index(loadout.id))]
            fields += cli.field('id_item')
            self.output_record(fields)

        self.output_footer()

class TraitLoadoutGenerator(DataGenerator):
    def filter(self):
        return TraitLoadoutSet(self._options).get()

    def generate(self, data = None):
        self.output_header(
            header = 'Trait Loadout data',
            type = 'trait_loadout_data_t',
            array = 'trait_loadout',
            length = len(data))

        for entry in sorted(data, key = lambda e: (e.ref('id_trait_tree_loadout').id_spec, e.order_index)):
            fields = entry.ref('id_trait_tree_loadout').field('id_spec')

            if entry.id_trait_node_entry != 0:
                node_entry = entry
            else:
                node_entries = entry.ref('id_trait_node').child_refs('TraitNodeXTraitNodeEntry')
                if len(node_entries) == 1:
                    node_entry = node_entries[0]
                else:
                    node_entry = sorted(node_entries, key = lambda e: e.index)[0]

            fields += node_entry.field('id_trait_node_entry')
            fields += entry.field('num_points', 'order_index')

            self.output_record(fields)

        self.output_footer()
