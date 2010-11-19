import sys, struct
import parser, db

class DBCAligner:
    def __init__(self, options, dbc_1, dbc_2):
        self._dbc_base = parser.RawDBCParser(options, dbc_1)
        self._dbc_new = parser.RawDBCParser(options, dbc_2)

        if not self._dbc_base.open_dbc() or not self._dbc_new.open_dbc():
            sys.exit(1)

        # Read the records
        self._db_base = db.DBCDB()
        self._db_new  = db.DBCDB()

        raw_record = self._dbc_base.next_record()
        while raw_record != None:
            record = struct.unpack('I' * self._dbc_base._fields, raw_record)
            self._db_base[record[0]] = record
            raw_record = self._dbc_base.next_record()

        raw_record = self._dbc_new.next_record()
        while raw_record != None:
            record = struct.unpack('I' * self._dbc_new._fields, raw_record)
            self._db_new[record[0]] = record
            raw_record = self._dbc_new.next_record()

        self._records = set(sorted(self._db_base.keys())) & set(sorted(self._db_new.keys()))

    def align(self):
        field_matches = [ ]
        fields = range(0, self._dbc_base._fields)
    
        field_diff = self._dbc_new._fields - self._dbc_base._fields
        match_keys = 0
        max_keys = len(self._records)
        added_fields = [ ]
        
        for id in self._records:
            r_base = self._db_base[id]
            if field_diff > 0:
                r_base += (0, ) * field_diff

            r_new  = self._db_new[id]

            if r_new == r_base:
                continue
            else:
                for i in xrange(0, len(r_base)):
                    if r_base[i] == r_new[i]:
                        continue

                    added_fields.append(i)


