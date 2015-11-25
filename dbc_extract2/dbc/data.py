import struct, sys, math

import dbc.fmt

_FORMATDB = None

# Yank string out of datastream
def _getstring(record, offset):
    start = offset
    end = offset

    # Skip zeros
    while True:
        b = record[start]
        if b == 0:
            start += 1
            end += 1
        else:
            break

    # Forwards until \x00 found
    while True:
        b = record[end]
        if b == 0:
            break
        end += 1

    return (start, end)

class DBCRecord(object):
    __d = None

    # Default value if database is accessed with a missing key (id)
    @classmethod
    def default(cls, *args):
        if not cls.__d:
            cls.__d = cls(None, None, 0, 0)

        # Ugly++ but it will have to do
        for i in range(0, len(args), 2):
            setattr(cls.__d, args[i], args[i + 1])

        return cls.__d

    @classmethod
    def data_size(cls):
        return cls._parser.size

    def __init__(self, parser, data, dbc_id, record_offset):
        self._dbcp = parser
        # Store data if we are in debug mode
        if self._dbcp and self._dbcp._options.debug == True:
            self._dbc_id = dbc_id
            self._record = data
            self._record_offset = record_offset

        # Decode data based on parser
        if data:
            self._d = self._parser.unpack_from(data)
        # No data given, all zero fields (used by default construction)
        else:
            self._d = (0,) * len(self._fi)

        # Separate dbc id given, prepend it to the parsed data tuple
        if dbc_id > 0:
            self._d = (dbc_id,) + self._d

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

    # Customize data access, this gets only called on fields that do not exist in the object. If the
    # format of the field is 'S', the value is an offset to the stringblock giving the string
    def __getattr__(self, name):
        try:
            field_idx = self._cd[name]
        except:
            raise AttributeError

        if self._fo[field_idx] == 'S' and self._d[field_idx] > 0:
            return self._dbcp.get_string_block(self._d[field_idx])
        else:
            return self._d[field_idx]

    # Output field data based on formatting
    def field(self, *args):
        f = [ ]
        for field in args:
            try:
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
                    tmpstr = self._dbcp.get_string_block(self._d[field_idx])
                    tmpstr = tmpstr.replace("\\", "\\\\")
                    tmpstr = tmpstr.replace("\"", "\\\"")
                    tmpstr = tmpstr.replace("\n", "\\n")
                    tmpstr = tmpstr.replace("\r", "\\r")
                    tmpstr = '"' + tmpstr + '"'
                f.append(self._ff[field_idx] % tmpstr)
            else:
                f.append(self._ff[field_idx] % self._d[field_idx])

        return f

    def __str__(self):
        s = ''

        for i in range(0, len(self._fi)):
            field = self._fi[i]
            fmt = self._ff[i]
            type_ = self._fo[i]
            if not field:
                continue

            if type_ == 'S' and self._d[i] > 0:
                s += '%s=\"%s\" ' % (field, repr(self._dbcp.get_string_block(self._d[i])))
            elif type_ == 'f':
                s += '%s=%f ' % (field, self._d[i])
            elif type_ in 'ihb':
                s += '%s=%d ' % (field, self._d[i])
            else:
                s += '%s=%u ' % (field, self._d[i])

        if self._dbcp and self._dbcp._options.debug == True:
            s += 'bytes=['
            for b in range(0, len(self._record)):
                s += '%.02x' % self._record[b]
                if (b + 1) % 4 == 0 and b < len(self._record) - 1:
                    s += ' '

            s += ']'
        return s

    def csv(self, delim = ',', header = False):
        s = ''
        for i in range(0, len(self._fi)):
            field = self._fi[i]
            fmt = self._ff[i]
            type_ = self._fo[i]
            if not field:
                continue

            if type_ == 'S':
                if self._d[i] > 0:
                    s += '\"%s\"%c' % (field, repr(self._dbcp.get_string_block(self._d[i])), delim)
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

class Item_sparse(DBCRecord):
    _p1 = None
    _p2 = None

    def __init__(self, dbc_parser, record, dbc_id, record_offset):
        # Create dummy parsers, splits out name/desc, and eliminates extra padding from the whole
        # data format
        if not Item_sparse._p1:
            p1_idx = self._fi.index('name')
            p2_idx = p1_idx + 1

            has_padding = 'x' in self._fo[-1] and len(self._fo[-1]) or False

            Item_sparse._p1 = struct.Struct(self._parser.format[:p1_idx - 1])
            if has_padding:
                Item_sparse._p2 = struct.Struct(self._parser.format[p2_idx:-has_padding])
            else:
                Item_sparse._p2 = struct.Struct(self._parser.format[p2_idx:])

        self._dbcp = dbc_parser
        self.spells = []
        self.journal = JournalEncounterItem.default()

        # if we are not given data, bail out early. This occurs with default objects.
        if not record:
            self._d = (0,) * len(self._fi)
            return

        # Unpack first set of data, note that field indexing is +1 because 'id' is
        # the first field, and not included in the dbc record data
        d = self._p1.unpack_from(record)
        self._d = (dbc_id,) + d

        # Name as an inline string, beginning immediately after the first set of data
        start_offset, end_offset = _getstring(record, self._p1.size)
        # Insert as a field to the string start offset in the file. Specialized DBC file parser will
        # handle string extraction when the time comes
        self._d += (record_offset + start_offset,)
        # Padded with 4 bytes, fifth byte may be non-zero if desc is available
        end_offset += 4

        # Check if there's a description
        p2_offset = 0
        if record[end_offset] != 0:
            start_offset, end_offset = _getstring(record, end_offset)
            self._d += (record_offset + start_offset,)
        else:
            self._d += (0,)

        # Second set of data starts immediately after the strings (there is always a single byte of
        # padding for strings), even if no desc is available. Thus, choices:
        # name | 1 byte | 4 bytes
        # name | 4 bytes | desc | 1 byte
        d = self._p2.unpack_from(record, end_offset + 1)
        self._d += d

class SpellEffect(DBCRecord):
    def __init__(self, dbc_parser, record, dbc_id, record_offset):
        DBCRecord.__init__(self, dbc_parser, record, dbc_id, record_offset)

        self.scaling = None

class Spell(DBCRecord):
    def __init__(self, dbc_parser, record, dbc_id, record_offset):
        DBCRecord.__init__(self, dbc_parser, record, dbc_id, record_offset)

        self._effects = [ None ]
        self.max_effect_index = 0
        self._powers  = []
        self._misc    = []
        self._categories = []
        self._scaling = SpellScaling.default()
        self._levels = SpellLevels.default()
        self._cooldowns = SpellCooldowns.default()
        self._auraoptions = SpellAuraOptions.default()
        self._equippeditems = SpellEquippedItems.default()
        self._classopts = SpellClassOptions.default()

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

        setattr(self, 'effect_%d' % ( spell_effect.index + 1 ), spell_effect)

    def add_power(self, spell_power):
        self._powers.append(spell_power)

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

class ItemSet(DBCRecord):
    def __init__(self, dbc_parser, record, dbc_id, record_offset):
        DBCRecord.__init__(self, dbc_parser, record, dbc_id, record_offset)

        self.bonus = []

def proxy_str(self):
    s = ''
    data_idx = 1
    format_idx = 0
    if self._dbcp._id_offset > 0:
        fmt = '%%-%us' % int(math.log10(self._dbcp._last_id) + 1)
        s += 'id=%s ' % (fmt % self._d[0])
    else:
        data_idx = 1
        format_idx = 1
        fmt = '%%-%us' % int(math.log10(self._dbcp._last_id) + 1)
        s += 'id=%s ' % (fmt % self._d[0])

    for idx in range(0, len(self._parser.format) - format_idx):
        fmt = self._parser.format[format_idx + idx]
        if fmt == ord('I'):
            s += '%08x' % (self._d[data_idx + idx])
        elif fmt == ord('H'):
            s += '%04x' % (self._d[data_idx + idx])
        elif fmt == ord('B'):
            s += '%02x' % (self._d[data_idx + idx])
        s += ' '

    if getattr(self, '_record', None):
        s += 'bytes=['
        for b in range(0, len(self._record)):
            s += '%.02x' % self._record[b]
            if (b + 1) % 4 == 0 and b < len(self._record) - 1:
                s += ' '

        s += ']'

    return s

def proxy_class(file_name, fields, record_size):
        class_name = '%s' % file_name.split('.')[0].replace('-', '_')

        f = fields
        rs = record_size
        b4 = b2 = b1 = 0
        while f > 0:
            if rs >= (4 + f) or (rs == 4 and f == 1):
                b4 += 1
                rs -= 4
            elif rs >= (2 + f) or (rs == 2 and f == 1):
                b2 += 1
                rs -= 2
            else:
                b1 += 1
                rs -= 1

            f -= 1

        #print(fields, record_size, b4, b2, b1, record_size - b4 * 4 - b2 * 2 - b1)

        # Override the __str__ method of the proxy class so that it prints out
        # something better for DBC data exploration
        objdict = DBCRecord.__dict__.copy()
        objdict['__str__'] = proxy_str

        setattr(dbc.data, class_name, type('%s' % class_name.encode('ascii'), (DBCRecord,), dict(objdict)))

        cls = getattr(dbc.data, class_name)

        # Set class-specific parser
        setattr(cls, '_parser', struct.Struct('I' * b4 + 'H' * b2 + 'B' * b1))
        setattr(cls, '_cd', {})

        return cls

def initialize_data_model(options, obj):
    global _FORMATDB
    _FORMATDB = dbc.fmt.DBFormat(options)

    for dbc_file_name, data_fo in _FORMATDB.data.items():
        class_name = '%s' % dbc_file_name.split('.')[0].replace('-', '_')

        # Add class to the data model if it does not exist. Inherit automatically from DBCRecord
        if class_name not in dir(obj):
            setattr(obj, class_name, type('%s' % class_name.encode('ascii'), (DBCRecord,), dict(DBCRecord.__dict__)))

        cls = getattr(obj, class_name)

        # Set class-specific parser
        setattr(cls, '_parser', _FORMATDB.parser(dbc_file_name))

        # Setup data field names (_fi), data field types (_fo), and data field formats for output
        # (_ff)
        if data_fo['data-fields'][0] == 'id':
            setattr(cls, '_fi', tuple(data_fo['data-fields']))
            setattr(cls, '_fo', tuple(data_fo['data-format']))
            setattr(cls, '_ff', tuple(data_fo['cpp']))
        else:
            setattr(cls, '_fi', ('id',) + tuple(data_fo['data-fields']))
            setattr(cls, '_fo', ('I',) + tuple(data_fo['data-format']))
            setattr(cls, '_ff', ('%u',) + tuple(data_fo['cpp']))

        # Setup index lookup table for fields to speedup __getattr__ access
        setattr(cls, '_cd', {})
        for fidx in range(0, len(cls._fi)):
            if not cls._fi:
                continue

            cls._cd[cls._fi[fidx]] = fidx

