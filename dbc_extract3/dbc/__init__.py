import collections

DBCRecordInfo = collections.namedtuple('DBCRecordInfo', [ 'dbc_id', 'record_id', 'record_offset', 'record_size' ])

class DBCRecordData(collections.namedtuple('DBCRecordData', [ 'dbc_id', 'parent_id', 'record_data' ])):
    __slots__ = ()

    def valid(self):
        return len(self.record_data) > 0
