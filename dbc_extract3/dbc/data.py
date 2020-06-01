import struct, sys, math, types

import dbc.fmt
import dbc.db

_FORMATDB = None

class RawDBCRecord:
    __slots__ = ('_id', '_d', '_dbcp', '_flags', '_key')

    _key_block = False
    _id_block = False

    _id_format = '%u'
    _key_format = '%u'

    _id_index = 0

    # Parenting, can either use the key block, or if defined by the format
    # files, a specific field (_parent_key) of the data
    _parent_db = None
    _parent_key_db = None

    @classmethod
    def has_key_block(cls, state):
        cls._key_block = state

    @classmethod
    def has_id_block(cls, state):
        cls._id_block = state

    @classmethod
    def id_format(cls, format):
        cls._id_format = format

    @classmethod
    def key_format(cls, format):
        cls._key_format = format

    @classmethod
    def id_index(cls, index):
        cls._id_index = index

    @classmethod
    def parent(cls, parent, key = None, n_elements = 1):
        # Key block, set the parent key database
        if key == None:
            cls._parent_key_db = parent
        else:
            if cls._parent_db == None:
                cls._parent_db = [None,] * len(cls._cd)

            if n_elements == 1:
                field_idx = cls._cd.get(key, -1)
                if field_idx == -1:
                    raise ValueError(
                        'Unknown db2 field "{}" for "()" when parenting to "{}"').format(
                            key, cls.__name__, parent)

                cls._parent_db[field_idx] = parent
            else:
                for i in range(1, n_elements + 1):
                    key_str = '{}_{}'.format(key, i)
                    field_idx = cls._cd.get(key_str)
                    if field_idx == -1:
                        raise ValueError(
                            'Unknown db2 field "{}" for "()" when parenting to "{}"').format(
                                key_str, cls.__name__, parent)

                    cls._parent_db[field_idx] = parent

    def dbc_name(self):
        return self.__class__.__name__.replace('_', '-')

    def is_hotfixed(self):
        return self._flags != 0

    # Get the child objects if defined by the data format.
    # Currently allows indexing of any data table
    #
    # TODO: Add checks for linkage
    def children(self, child_db):
        if not dbc.db.datastore():
            return []

        db = dbc.db.datastore().get(child_db)
        return db.records_for_parent(self._id)

    def child_refs(self, child_db):
        if not dbc.db.datastore():
            return []

        db = dbc.db.datastore().get(child_db)
        return db.records_for_reference(self.dbc_name(), self._id)

    # Get the parent object if defined by the data format
    def parent_record(self, field = None):
        # Key block handling
        if not field:
            if self._parent_key_db:
                db = dbc.db.datastore().get(self._parent_key_db)
                return db[self._key]
            else:
                raise ValueError('No key-block based parent database defined for "{}"',
                    self.__name__)
        else:
            if not self._parent_db:
                raise ValueError('No parent reference defined for field "{}"'.format(field))

            if field not in self._cd:
                raise ValueError('Invalid field name "{}" for parent reference'.format(field))

            field_idx = self._cd[field]
            if self._parent_db[field_idx] == None:
                raise ValueError('No parent reference for field "{}"'.format(field))

            db = dbc.db.datastore().get(self._parent_db[field_idx])

            return db[self._d[field_idx]]

    def __init__(self, parser, dbc_id, data, key = 0):
        self._dbcp = parser

        self._id = dbc_id
        self._d = data
        self._key = key
        self._flags = 0

        if not self._d:
            self._d = (0,) * len(self._fi)

    def __getattr__(self, name):
        if name == 'id' and self._id_block and self._id > -1:
            return self._id
        # Always return the parent id, even as 0 if the block does not exist in the db2 file
        elif name == 'id_parent':
            return self._key
        else:
            raise AttributeError

    def __str__(self):
        s = []
        if self._id_block:
            s.append('id=%u' % self._id)

        for i in range(0, len(self._d)):
            if not self._id_block and self._id_index == i:
                s.append('id=%d' % self._d[i])
            else:
                s.append('f%d=%d' % (i + 1, self._d[i]))

        if self._key_block:
            s.append('id_parent=%u' % self._key)

        return ' '.join(s)

class DBCRecord(RawDBCRecord):
    __l = None
    __d = None
    # Lazy initialized in add_link, Hotfixed field data
    __slots__ = ('_l', '_hotfix_data', '_child_data', '_parent', '_ref_cache' ) 

    # Default value if database is accessed with a missing key (id)
    @classmethod
    def default(cls, parser):
        if not cls.__d:
            cls.__d = cls(parser, 0, None, 0)

        return cls.__d

    @classmethod
    def link(cls, attr_name, attr_default):
        if not cls.__l:
            cls.__l = {}

        if attr_name in cls.__l:
            return

        cls.__l[attr_name] = attr_default

    # Get the reference object for a specific field
    def ref(self, field_name):
        field_idx = self._cd.get(field_name, -1)
        if field_idx == -1:
            return None

        if self._dbr[field_idx] == None:
            return None

        if not hasattr(self, '_ref_cache'):
            self._ref_cache = [None] * len(self._cd)

        if self._ref_cache[field_idx] == None:
            db = dbc.db.datastore().get(self._dbr[field_idx])
            self._ref_cache[field_idx] = db[self._d[field_idx]]

        return self._ref_cache[field_idx]

    # Field_name of -1 indicates new entry, otherwise, collect original value
    # to _hotfix_data so we can output it later on
    def add_hotfix(self, field_name, original_data):
        field_index = 0
        if field_name == -1:
            field_index = -1
        else:
            field_index = self._cd[field_name]

        if not hasattr(self, '_hotfix_data'):
            self._hotfix_data = []

        if field_index == -1:
            self._hotfix_data.append((-1, None))
        else:
            self._hotfix_data.append((field_index, getattr(original_data, field_name)))

    def add_link(self, name, value):
        if not hasattr(self, '_l'):
            self._l = {}

        if name not in self._l:
            self._l[name] = []

        if name in self.__l:
            self._l[name].append(value)
        else:
            v = getattr(self, name, None)
            if isinstance(v, types.FunctionType) or \
               isinstance(v, types.MethodType):
                v(value)
            else:
                raise AttributeError

    # Returns a tuple of ( <hotfix field flags>, [ <hotfix data: mapping id, field type, and the original value> ] )
    def get_hotfix_info(self, *args):
        hotfix_val = 0
        hotfix_data = []
        for field_name, map_index in args:
            # Note, theoretically this can keyerror, which is fine since it's
            # always a bug on the generator side (typoed field name)
            field_index = self._cd[field_name]

            if (1 << field_index) & self._flags == 0:
                continue

            # Only flag the same mapping index once, since we are not concerned
            # (currently) with hotfixed fields inside arrays of data
            if (1 << map_index) & hotfix_val:
                continue

            hotfix_val |= (1 << map_index)
            # Add field to hotfix data
            for index, orig_value in self._hotfix_data:
                if index == field_index:
                    hotfix_data.append((map_index, self._fo[field_index], orig_value, getattr(self, field_name)))
                    break

        return hotfix_val, hotfix_data

    def get_link(self, name, index = 0):
        if name not in self.__l:
            raise AttributeError

        if not hasattr(self, '_l'):
            return self.__l[name].default(self._dbcp)

        if name not in self._l:
            return self.__l[name].default(self._dbcp)

        v = self._l[name]
        if index >= len(v):
            return self.__l[name].default(self._dbcp)
        else:
            return v[index]

    def get_links(self, name):
        if name not in self.__l:
            raise AttributeError

        if not hasattr(self, '_l'):
            return []

        return self._l.get(name, [])

    def has_value(self, field, value):
        try:
            idx = self._cd[field]
        except:
            return False

        if type(value) in [list, tuple] and self._d[idx] in value:
            return True
        elif type(value) in [str, int, float] and self._d[idx] == value:
            return True

        return False

    # Customize data access, this gets only called on fields that do not exist in the object. If the
    # format of the field is 'S', the value is an offset to the stringblock giving the string
    def __getattr__(self, name):
        try:
            field_idx = self._cd[name]
        except:
            return super().__getattr__(name)

        if self._fo[field_idx] == 'S' and self._d[field_idx] > 0:
            return self._dbcp.get_string(self._d[field_idx], self._id, field_idx)
        else:
            return self._d[field_idx]

    # Output field data based on formatting
    def field(self, *args):
        f = [ ]
        for field in args:
            field_idx = 0
            try:
                if field == 'id':
                    f.append(self._id_format % self._id)
                    continue
                elif field == 'id_parent':
                    f.append(self._key_format % self._key)
                    continue
                else:
                    field_idx = self._cd[field]
            # If the field isnt found, insert none and skip it. Presumably a child class
            # will insert it later on in the process
            except KeyError:
                f.append(None)
                continue

            if self._fo[field_idx] == 'S':
                tmpstr = u""
                if self._d[field_idx] == 0:
                    tmpstr = u"0"
                else:
                    tmpstr = self._dbcp.get_string(self._d[field_idx], self._id, field_idx)
                    if len(tmpstr) > 0:
                        tmpstr = tmpstr.replace("\\", "\\\\")
                        tmpstr = tmpstr.replace("\"", "\\\"")
                        tmpstr = tmpstr.replace("\n", "\\n")
                        tmpstr = tmpstr.replace("\r", "\\r")
                        tmpstr = '"' + tmpstr + '"'
                    else:
                        tmpstr = u"0"
                f.append(self._ff[field_idx] % tmpstr)
            else:
                fmt = self._ff[field_idx]
                if field == 'id':
                    fmt = self._id_format
                f.append(fmt % self._d[field_idx])

        return f

    def field_names(self, delim = ", ", raw = False):
        fields = []
        if self._id_block and self.__output_field('id'):
            fields.append('id')

        for field in self._fi:
            if self.__output_field(field):
                fields.append(field)

        if self._key_block and self.__output_field('id_parent'):
            fields.append('id_parent')

        if raw:
            return fields
        else:
            return delim.join(fields)

    def __output_field(self, field):
        if len(self._dbcp.options.fields) == 0:
            return True

        return field in self._dbcp.options.fields

    def __str__(self):
        s = []

        if self._id_block and self.__output_field('id'):
            s.append('id=%u' % self._id)

        for i in range(0, min(len(self._d), len(self._fi))):
            field = self._fi[i]
            type_ = self._fo[i]
            if not field or 'x' in type_:
                continue

            if not self.__output_field(field):
                continue

            if type_ == 'S' and self._d[i] > 0:
                str_ = self._dbcp.get_string(self._d[i], self._id, i)
                s.append('%s=%s%s' % (field,
                    (str_ and len(str_)) and repr(str_) or '0',
                    (str_ and len(str_)) and ' ({})'.format(self._d[i]) or ''
                ))
            elif type_ == 'f':
                s.append('%s=%f' % (field, self._d[i]))
            elif type_ in 'ihb':
                s.append('%s=%d' % (field, self._d[i]))
            else:
                s.append('%s=%u' % (field, self._d[i]))

        if self._key_block and self.__output_field('id_parent'):
            s.append('id_parent=%u' % self._key)

        return ' '.join(s)

    def obj(self):
        d = {}
        if self._id_block and self.__output_field('id'):
            d['id'] = self._id

        for i in range(0, len(self._fi)):
            field = self._fi[i]
            type_ = self._fo[i]
            if not field:
                continue

            if not self.__output_field(field):
                continue

            if type_ == 'S':
                if self._d[i] > 0:
                    d[field] = self._dbcp.get_string(self._d[i], self._id, i)
                else:
                    d[field] = ""
            else:
                d[field] = self._d[i]

        if self._key_block and self.__output_field('id_parent'):
            d['id_parent'] = self._key

        return d

    def csv(self, delim = ',', header = False):
        s = ''
        if self._id_block and self.__output_field('id'):
            s += '%u%c' % (self._id, delim)

        for i in range(0, len(self._fi)):
            field = self._fi[i]
            fmt = self._ff[i]
            type_ = self._fo[i]
            if not field:
                continue

            if not self.__output_field(field):
                continue

            if type_ == 'S':
                if self._d[i] > 0:
                    s += '"%s"%c' % (self._dbcp.get_string(self._d[i], self._id, i).replace('"', '\\"'), delim)
                else:
                    s += '""%c' % delim
            elif type_ == 'f':
                s += '%f%c' % (self._d[i], delim)
            elif type_ in 'ihb':
                s += '%d%c' % (self._d[i], delim)
            else:
                s += '%u%c' % (self._d[i], delim)

        if self._key_block and self.__output_field('id_parent'):
            s += '%u%c' % (self._key, delim)

        if len(s) > 0:
            s = s[0:-1]
        return s

class SpellName(DBCRecord):
    __slots__ = ( '_effects', 'max_effect_index' )

    def __init__(self, dbc_parser, dbc_id, data, key_id):
        super().__init__(dbc_parser, dbc_id, data, key_id)

        self._effects = []
        self.max_effect_index = -1

    def has_effect(self, field, value):
        for effect in self._effects:
            if not effect:
                continue

            if effect.has_value(field, value):
                return True

        return False

    def add_effect(self, spell_effect):
        if spell_effect.index > self.max_effect_index:
            self.max_effect_index = spell_effect.index
            for i in range(0, (self.max_effect_index + 1) - len(self._effects)):
                self._effects.append( None )

        self._effects[ spell_effect.index ] = spell_effect

    def __getattr__(self, name):
        # Hack to get effect default values if spell has no effect_x, as those fields 
        # are the only ones that may be missing in Spellxxx. Just in case raise an 
        # attributeerror if something else is being accessed
        if 'effect_' in name:
            return SpellEffect.default(self._dbcp)

        return DBCRecord.__getattr__(self, name)

    def __str__(self):
        s = DBCRecord.__str__(self)

        for i in range(len(self._effects)):
            if not self._effects[i]:
                continue
            s += 'effect_%d={ %s} ' % (i + 1, str(self._effects[i]))

        return s

class SpellCooldowns(DBCRecord):
    def __getattribute__(self, name):
        if name == 'cooldown_duration':
            if self.category_cooldown > 0:
                return self.category_cooldown
            elif self.cooldown > 0:
                return self.cooldown
            else:
                return 0
        else:
            return DBCRecord.__getattribute__(self, name)

    # Need to override get_hotfix_info too so we can get some information
    # out of the record when the special 'cooldown_duration' field is specified
    def get_hotfix_info(self, *args):
        hotfix_val = 0
        hotfix_data = []
        for field_name, map_index in args:
            if field_name == 'cooldown_duration':
                if (1 << self._cd['cooldown']) & self._flags:
                    field_index = self._cd['cooldown']
                    hotfix_val |= (1 << map_index)
                    for index, orig_value in self._hotfix_data:
                        if index == field_index:
                            hotfix_data.append((map_index, self._fo[field_index], orig_value, self._d[field_index]))
                            break
                elif (1 << self._cd['category_cooldown']) & self._flags:
                    field_index = self._cd['category_cooldown']
                    hotfix_val |= (1 << map_index)
                    for index, orig_value in self._hotfix_data:
                        if index == field_index:
                            hotfix_data.append((map_index, self._fo[field_index], orig_value, self._d[field_index]))
                            break
            else:
                field_index = self._cd[field_name]
                if (1 << field_index) & self._flags == 0:
                    continue

                hotfix_val |= (1 << map_index)

                for index, orig_value in self._hotfix_data:
                    if index == field_index:
                        hotfix_data.append((map_index, self._fo[field_index], orig_value, getattr(self, field_name)))
                        break

        return hotfix_val, hotfix_data


    def field(self, *args):
        f = DBCRecord.field(self, *args)

        if 'cooldown_duration' in args:
            if self.cooldown > 0:
                f[args.index('cooldown_duration')] = u'%7u' % self.cooldown
            elif self.category_cooldown > 0:
                f[args.index('cooldown_duration')] = u'%7u' % self.category_cooldown
            else:
                f[args.index('cooldown_duration')] = u'%7u' % 0

        return f

class Meta(type):
    def __new__(mcl, name, bases, namespace):
        namespace['__slots__'] = ()
        return super(Meta, mcl).__new__(mcl, name, bases, namespace)

def requires_data_model(options):
    return options.type in ['csv', 'view', 'output', 'generator', 'class_flags', 'json']

def initialize_data_model(options):
    if not requires_data_model(options):
        return

    # Initialize data store
    dbc.db.datastore(options)

    global _FORMATDB
    _FORMATDB = dbc.fmt.DBFormat(options)

    this_module = sys.modules[__name__]

    for dbc_file_name, data_fo in _FORMATDB.data.items():
        class_name = '%s' % dbc_file_name.split('.')[0].replace('-', '_')

        # Add class to the data model if it does not exist. Inherit automatically from DBCRecord
        if class_name not in dir(this_module):
            new_class = types.new_class(class_name, bases = (DBCRecord,), kwds = { 'metaclass': Meta })
            setattr(this_module, class_name, new_class)

        cls = getattr(this_module, class_name)

        # Setup data field names (_fi), data field types (_fo), and data field formats for output
        # (_ff)
        setattr(cls, '_fi', tuple(_FORMATDB.fields(dbc_file_name, True)))
        setattr(cls, '_fo', tuple(_FORMATDB.types(dbc_file_name, True)))
        setattr(cls, '_ff', tuple(_FORMATDB.formats(dbc_file_name, True)))
        setattr(cls, '_dbr', tuple(_FORMATDB.refs(dbc_file_name, True)))

        # Setup index lookup table for fields to speedup __getattr__ access
        setattr(cls, '_cd', {})
        for fidx in range(0, len(cls._fi)):
            if not cls._fi:
                continue

            cls._cd[cls._fi[fidx]] = fidx

        for parent in _FORMATDB.parent_dbs(dbc_file_name):
            cls.parent(parent['db'], parent['key'], parent['elements'])

    if 'SpellName' in dir(this_module):
        SpellName.link('level', SpellLevels)
        SpellName.link('power', SpellPower)
        SpellName.link('categories', SpellCategories)
        SpellName.link('cooldown', SpellCooldowns)
        SpellName.link('aura_option', SpellAuraOptions)
        SpellName.link('equipped_item', SpellEquippedItems)
        SpellName.link('class_option', SpellClassOptions)
        SpellName.link('shapeshift', SpellShapeshift)
        SpellName.link('scaling', SpellScaling)
        SpellName.link('artifact_power', ArtifactPowerRank)
        SpellName.link('azerite_power', AzeritePower)
        SpellName.link('label', SpellLabel)
        SpellName.link('misc', SpellMisc)
        SpellName.link('text', Spell)

        if options.build >= 25600:
            SpellName.link('desc_var_link', SpellXDescriptionVariables)

        if options.build >= dbc.WowVersion(8, 2, 0, 30080):
            SpellName.link('azerite_essence', AzeriteEssencePower)

    if 'SpellEffect' in dir(this_module) and options.build < 25600:
        SpellEffect.link('scaling', SpellEffectScaling)

    if 'SpellItemEnchantment' in dir(this_module):
        SpellItemEnchantment.link('spells', SpellName)
        SpellItemEnchantment.link('gem_property', GemProperties)

    if 'Item_sparse' in dir(this_module):
        Item_sparse.link('spells', ItemEffect)
        Item_sparse.link('journal', JournalEncounterItem)

    if 'ItemSparse' in dir(this_module):
        ItemSparse.link('spells', ItemEffect)
        ItemSparse.link('journal', JournalEncounterItem)

    if 'ItemSet' in dir(this_module):
        ItemSet.link('bonus', ItemSetSpell)

    if 'GemProperties' in dir(this_module):
        if 'Item_sparse' in dir(this_module):
            GemProperties.link('item', Item_sparse)
        elif 'ItemSparse' in dir(this_module):
            GemProperties.link('item', ItemSparse)

    if 'AzeriteEssence' in dir(this_module) and 'AzeriteEssencePower' in dir(this_module):
        AzeriteEssence.link('powers', AzeriteEssencePower)

