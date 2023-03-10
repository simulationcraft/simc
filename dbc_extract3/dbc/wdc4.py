import logging, struct, collections

from dbc import HeaderFieldInfo

from dbc import wdc1
from dbc.wdc2 import WDC2Parser, WDC2Section
from dbc.wdc3 import WDC3Parser, WDC3Section

class WDC4Section(WDC3Section):
    pass

class WDC4Parser(WDC3Parser):
    SECTION_OBJECT = WDC4Section

    def is_magic(self): return self.magic == b'WDC4'
