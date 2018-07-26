import collections

# Constants

# For some client data file hotfix entries, the key block id (parent id) is
# repeated in the actual record data. For those files, the key block id is
# actually not concatenated to the hotfix entry.
KEY_FIELD_HOTFIX_RECORD = {
    'ItemBonus'             : 'id_node',
    'SkillLineAbility'      : 'id_skill',
    'SpecializationSpells'  : 'spec_id',
    'ItemModifiedAppearance': 'id_item'
}

def use_hotfix_key_field(wdb_name):
    return KEY_FIELD_HOTFIX_RECORD.get(wdb_name, None)

HeaderFieldInfo = collections.namedtuple('HeaderFieldInfo', [ 'attr', 'format' ])

DBCRecordInfo = collections.namedtuple('DBCRecordInfo', [ 'dbc_id', 'record_id', 'record_offset', 'record_size', 'parent_id' ])

