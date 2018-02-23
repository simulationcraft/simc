import collections

# Constants

# Some WDB files are stored in "expanded form" in the hotfix cache file
# (DBCache.bin). In these cases, the normal WDC1-based file format is
# abandoned, seemingly replacing it with a simple full-length field format
# (e.g., fields are 4 bytes). In addition, the WDC1 id block seems to be
# tacked to the beginning of the record key block seems to be tacked to the
# end of the record.
EXPANDED_HOTFIX_RECORDS = [ 'SpellEffect' ]

# For some client data file hotfix entries, the key block id (parent id) is
# repeated in the actual record data. For those files, the key block id is
# actually not concatenated to the hotfix entry.
KEY_FIELD_HOTFIX_RECORD = {
    'ItemBonus'       : 'id_node',
    'SkillLineAbility': 'id_skill'
}

def use_hotfix_key_field(wdb_name):
    return KEY_FIELD_HOTFIX_RECORD.get(wdb_name, None)

DBCRecordInfo = collections.namedtuple('DBCRecordInfo', [ 'dbc_id', 'record_id', 'record_offset', 'record_size', 'parent_id' ])

class DBCRecordData(collections.namedtuple('DBCRecordData', [ 'dbc_id', 'parent_id', 'record_data' ])):
    __slots__ = ()

    def valid(self):
        return len(self.record_data) > 0
