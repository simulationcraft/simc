#!/usr/bin/env python3

import sys

from helper import Test, TestGroup, run, find_profiles
from talent_options import talent_combinations

FIGHT_STYLES = ( 'Patchwerk', 'DungeonSlice', 'HeavyMovement' )
COVENANTS = ( 'Kyrian', 'Venthyr', 'Night_Fae', 'Necrolord' )

if len(sys.argv) < 2:
    sys.exit(1)

klass = sys.argv[1]

print(' '.join(klass.split('_')))

tests = []
for profile, path in find_profiles(klass):
    for fight_style in FIGHT_STYLES:
        grp = TestGroup('{}/{}/talents'.format(profile, fight_style),
                        fight_style=fight_style, profile=path)
        tests.append(grp)
        for talents in talent_combinations(klass):
            Test('{}'.format(talents), group=grp,
                 args=[ 'talents={}'.format(talents) ] )

    for fight_style in FIGHT_STYLES:
        grp = TestGroup('{}/{}/covenants'.format(profile, fight_style),
                        fight_style=fight_style, profile=path)
        tests.append(grp)
        for covenant in COVENANTS:
            Test('{}'.format(covenant), group=grp,
                 args=[ 'covenant={}'.format(covenant.lower()), 'level=60' ] )

sys.exit( run(tests) )
