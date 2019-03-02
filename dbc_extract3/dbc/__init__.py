import collections, re

# Simple World of Warcraft versioning structure
class WowVersion:
    def __init__(self, *args):
        # Presume a string
        if len(args) == 1:
            mobj = re.match('^([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)$', args[0])
            if not mobj:
                raise ValueError('Invalid World of Warcraft version string {}, expected format <expansion>.<patch>.<minor>.<build>'.format(args[0]))

            self.__expansion = int(mobj.group(1))
            self.__patch = int(mobj.group(2))
            self.__minor = int(mobj.group(3))
            self.__build = int(mobj.group(4))
        # 4 numbers (expansion, patch, minor, build)
        elif len(args) == 4:
            self.__expansion = int(args[0])
            self.__patch = int(args[1])
            self.__minor = int(args[2])
            self.__build = int(args[3])

    def expansion(self):
        return self.__expansion

    def patch(self):
        return self.__patch

    def minor(self):
        return self.__minor

    def build(self):
        return self.__build

    def patch_level(self):
        val = self.expansion() << 27
        val |= self.patch() << 22
        val |= self.minor() << 17
        return val

    def version(self):
        val = self.expansion() << 27
        val |= self.patch() << 22
        val |= self.minor() << 17
        val |= self.build()
        return val

    def __le__(self, other):
        return not self.__gt__(other)

    def __ge__(self, other):
        return not self.__lt__(other)

    def __eq__(self, other):
        return not self.__gt__(other) and not self.__lt__(other)

    def __ne__(self, other):
        return not self.__eq__(other)

    def __cmp(self, val):
        build_only = False
        other_obj = None

        if isinstance(val, str):
            other_obj = WowVersion(val)
        # Build-only comparison
        elif isinstance(val, int):
            build_only = True
        else:
            other_obj = val

        return build_only, other_obj

    def __gt__(self, other):
        build_only, other_obj = self.__cmp(other)

        if build_only:
            return self.build() > other
        else:
            return self.version() > other.version()

    def __lt__(self, other):
        build_only, other_obj = self.__cmp(other)

        if build_only:
            return self.build() < other
        else:
            return self.version() < other.version()

    def __str__(self):
        return '{}.{}.{}.{}'.format(self.expansion(), self.patch(), self.minor(), self.build())

# Constants

# For some client data file hotfix entries, the key block id (parent id) is
# repeated in the actual record data. For those files, the key block id is
# actually not concatenated to the hotfix entry.
KEY_FIELD_HOTFIX_RECORD = {
    'ItemBonus'             : 'id_node',
    'SkillLineAbility'      : 'id_skill',
    'SpecializationSpells'  : 'spec_id',
    'ItemModifiedAppearance': 'id_item',
    'JournalEncounterItem'  : 'id_encounter'
}

def use_hotfix_key_field(wdb_name):
    return KEY_FIELD_HOTFIX_RECORD.get(wdb_name, None)

HeaderFieldInfo = collections.namedtuple('HeaderFieldInfo', [ 'attr', 'format' ])

DBCRecordInfo = collections.namedtuple('DBCRecordInfo',
    [ 'dbc_id', 'record_offset', 'record_size', 'parent_id', 'section_id' ])

EMPTY_RECORD = DBCRecordInfo(-1, 0, 0, 0, -1)
