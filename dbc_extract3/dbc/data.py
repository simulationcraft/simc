import struct, sys, math, types

import dbc.fmt

_FORMATDB = None

class RawDBCRecord:
    __slots__ = ( '_id', '_d' )

    _dbcp = None

    def __init__(self, parser, dbc_id, data):
        if not self.__class__._dbcp:
            self.__class__._dbcp = parser

        self._id = dbc_id
        self._d = data

        if not self._d:
            self._d = (0,) * len(self._fi)

    def __getattr__(self, name):
        if name == 'id' and self._id > -1:
            return self._id
        else:
            raise AttributeError

    def __str__(self):
        s = []
        if self._id > -1:
            s.append('id=%u' % self._id)

        for i in range(0, len(self._d)):
            s.append('f%d=%d' % (i + 1, self._d[i]))

        return ' '.join(s)

class DBCRecord(RawDBCRecord):
    __l = None
    __d = None
    __slots__ = ('_l',) # Lazy initialized in add_link

    # Default value if database is accessed with a missing key (id)
    @classmethod
    def default(cls, *args):
        if not cls.__d:
            cls.__d = cls(None, 0, None)

        # Ugly++ but it will have to do
        for i in range(0, len(args), 2):
            setattr(cls.__d, args[i], args[i + 1])

        return cls.__d

    @classmethod
    def link(cls, attr_name, attr_default):
        if not cls.__l:
            cls.__l = {}

        if attr_name in cls.__l:
            return

        cls.__l[attr_name] = attr_default

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

    def get_link(self, name, index = 0):
        if name not in self.__l:
            raise AttributeError

        if not hasattr(self, '_l'):
            return self.__l[name].default()

        if name not in self._l:
            return self.__l[name].default()

        v = self._l[name]
        if index >= len(v):
            return self.__l[name].default()
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

    def value(self, *args):
        v = [ ]
        for attr in args:
            idx = self._cd[attr]
            v.append(self._d[idx])

        return v

    def field_names(self, delim):
        fields = [ None ] * len(self._cd)
        for i in self._cd.keys():
            fields[self._cd[i]] = i
        if self._dbcp.id_block_offset > 0:
            fields = [ 'id', ] + fields
        return delim.join(fields)

    # Customize data access, this gets only called on fields that do not exist in the object. If the
    # format of the field is 'S', the value is an offset to the stringblock giving the string
    def __getattr__(self, name):
        try:
            field_idx = self._cd[name]
        except:
            #if name == 'id':
            #    return self._id
            #raise AttributeError
            return super().__getattr__(name)

        if self._fo[field_idx] == 'S' and self._d[field_idx] > 0:
            return self._dbcp.get_string(self._d[field_idx])
        else:
            return self._d[field_idx]

    # Output field data based on formatting
    def field(self, *args):
        f = [ ]
        for field in args:
            field_idx = 0
            try:
                if field == 'id' and self._id > -1:
                    f.append(self._dbcp.id_format() % self._id)
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
                    tmpstr = self._dbcp.get_string(self._d[field_idx])
                    tmpstr = tmpstr.replace("\\", "\\\\")
                    tmpstr = tmpstr.replace("\"", "\\\"")
                    tmpstr = tmpstr.replace("\n", "\\n")
                    tmpstr = tmpstr.replace("\r", "\\r")
                    tmpstr = '"' + tmpstr + '"'
                f.append(self._ff[field_idx] % tmpstr)
            else:
                fmt = self._ff[field_idx]
                if field == 'id':
                    fmt = self._dbcp.id_format()
                f.append(fmt % self._d[field_idx])

        return f

    def __str__(self):
        s = []

        if self._id > -1:
            s.append('id=%u' % self._id)

        for i in range(0, len(self._fi)):
            field = self._fi[i]
            fmt = self._ff[i]
            type_ = self._fo[i]
            if not field:
                continue

            if type_ == 'S' and self._d[i] > 0:
                s.append('%s=\"%s\"' % (field, repr(self._dbcp.get_string(self._d[i]))))
            elif type_ == 'f':
                s.append('%s=%f' % (field, self._d[i]))
            elif type_ in 'ihb':
                s.append('%s=%d' % (field, self._d[i]))
            else:
                s.append('%s=%u' % (field, self._d[i]))
        return ' '.join(s)

    def csv(self, delim = ',', header = False):
        s = ''
        if self._dbcp.id_block_offset > 0:
            s += '%u%c' % (self._id, delim)

        for i in range(0, len(self._fi)):
            field = self._fi[i]
            fmt = self._ff[i]
            type_ = self._fo[i]
            if not field:
                continue

            if type_ == 'S':
                if self._d[i] > 0:
                    s += '\"%s\"%c' % (repr(self._dbcp.get_string(self._d[i])), delim)
                else:
                    s += '""%c' % delim
            elif type_ == 'f':
                s += '%f%c' % (self._d[i], delim)
            elif type_ in 'ihb':
                s += '%d%c' % (self._d[i], delim)
            else:
                s += '%u%c' % (self._d[i], delim)

        if len(s) > 0:
            s = s[0:-1]
        return s

class Spell(DBCRecord):
    __slots__ = ( '_effects', 'max_effect_index' )

    def __init__(self, dbc_parser, dbc_id, data):
        DBCRecord.__init__(self, dbc_parser, dbc_id, data)

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
            return SpellEffect.default()

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

def initialize_data_model(options, obj):
    global _FORMATDB
    _FORMATDB = dbc.fmt.DBFormat(options)

    for dbc_file_name, data_fo in _FORMATDB.data.items():
        class_name = '%s' % dbc_file_name.split('.')[0].replace('-', '_')

        # Add class to the data model if it does not exist. Inherit automatically from DBCRecord
        if class_name not in dir(obj):
            new_class = types.new_class(class_name, bases = (DBCRecord,), kwds = { 'metaclass': Meta })
            setattr(obj, class_name, new_class)

        cls = getattr(obj, class_name)

        # Setup data field names (_fi), data field types (_fo), and data field formats for output
        # (_ff)
        setattr(cls, '_fi', tuple(data_fo['data-fields']))
        setattr(cls, '_fo', tuple(data_fo['data-format']))
        setattr(cls, '_ff', tuple(data_fo['cpp']))

        # Setup index lookup table for fields to speedup __getattr__ access
        setattr(cls, '_cd', {})
        for fidx in range(0, len(cls._fi)):
            if not cls._fi:
                continue

            cls._cd[cls._fi[fidx]] = fidx

    if 'Spell' in dir(obj):
        dbc.data.Spell.link('level', dbc.data.SpellLevels)
        dbc.data.Spell.link('power', dbc.data.SpellPower)
        dbc.data.Spell.link('categories', dbc.data.SpellCategories)
        dbc.data.Spell.link('cooldown', dbc.data.SpellCooldowns)
        dbc.data.Spell.link('aura_option', dbc.data.SpellAuraOptions)
        dbc.data.Spell.link('equipped_item', dbc.data.SpellEquippedItems)
        dbc.data.Spell.link('class_option', dbc.data.SpellClassOptions)
        dbc.data.Spell.link('shapeshift', dbc.data.SpellShapeshift)
        dbc.data.Spell.link('scaling', dbc.data.SpellScaling)

    if 'SpellEffect' in dir(obj):
        dbc.data.SpellEffect.link('scaling', dbc.data.SpellEffectScaling)

    if 'SpellItemEnchantment' in dir(obj):
        dbc.data.SpellItemEnchantment.link('spells', dbc.data.Spell)
        dbc.data.SpellItemEnchantment.link('gem_property', dbc.data.GemProperties)

    if 'Item_sparse' in dir(obj):
        dbc.data.Item_sparse.link('spells', dbc.data.ItemEffect)
        dbc.data.Item_sparse.link('journal', dbc.data.JournalEncounterItem)

    if 'ItemSet' in dir(obj):
        dbc.data.ItemSet.link('bonus', dbc.data.ItemSetSpell)

    if 'GemProperties' in dir(obj):
        dbc.data.GemProperties.link('item', dbc.data.Item_sparse)

