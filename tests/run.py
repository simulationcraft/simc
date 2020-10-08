#!/usr/bin/env python3

import sys
import argparse
import random

from helper import Test, TestGroup, run, find_profiles
from talent_options import talent_combinations

FIGHT_STYLES = ('Patchwerk', 'DungeonSlice', 'HeavyMovement')
COVENANTS = ('Kyrian', 'Venthyr', 'Night_Fae', 'Necrolord')


def test_talents(klass: str, path: str):
    for fight_style in FIGHT_STYLES:
        grp = TestGroup('{}/{}/talents'.format(profile, fight_style),
                        fight_style=fight_style, profile=path)
        tests.append(grp)
        for talents in talent_combinations(klass):
            Test(talents, group=grp, args=[('talents', talents)])


def test_covenants(klass: str, path: str):
    try:
        from simc_support.game_data.WowSpec import get_wow_spec_from_combined_simc_name
        from simc_support.game_data.Conduit import get_conduits_for_spec
    except ImportError:
        print("Error importing simc-support library for covenant test. Skipping test.")
        return

    spec = get_wow_spec_from_combined_simc_name(klass)
    conduits = get_conduits_for_spec(spec)
    for fight_style in FIGHT_STYLES:
        grp = TestGroup('{}/{}/covenants'.format(profile, fight_style),
                        fight_style=fight_style, profile=path)
        tests.append(grp)

        # Test covenants
        for covenant in COVENANTS:
            Test(covenant, group=grp,
                 args=[('covenant', covenant.lower()), ('level', 60)])
            # Add conduits specific to selected covenant
            for conduit in conduits:
                if len(conduit.covenants) == 1 and covenant.lower() == conduit.covenants[0].simc_name:
                    rank = random.choice(conduit.ranks) + 1
                    soulbind_argument = '{}:{}'.format(conduit.id, rank)
                    Test('{} - {} ({}) Rank {}'.format(covenant, conduit.full_name, conduit.id, rank), group=grp, args=[
                        ('covenant', covenant.lower()), ('level', 60), ('soulbind', soulbind_argument)])

        # test conduits available for all 4 covenants independent from covenant selection.
        for conduit in conduits:
            if len(conduit.covenants) == 4:
                rank = random.choice(conduit.ranks) + 1
                soulbind_argument = '{}:{}'.format(conduit.id, rank)
                Test('{} ({}) Rank {}'.format(conduit.full_name, conduit.id, rank), group=grp, args=[
                    ('level', 60), ('soulbind', soulbind_argument)])


def test_trinkets(klass: str, path: str):
    try:
        from simc_support.game_data.WowSpec import get_wow_spec_from_combined_simc_name
        from simc_support.game_data.Trinket import get_trinkets_for_spec
    except ImportError:
        print("Error importing simc-support library for trinket test. Skipping test.")
        return

    spec = get_wow_spec_from_combined_simc_name(klass)
    trinkets = get_trinkets_for_spec(spec)
    fight_style = args.trinkets_fight_style
    grp = TestGroup('{}/{}/trinkets'.format(profile, fight_style),
                    fight_style=fight_style, profile=path)
    tests.append(grp)
    for trinket in trinkets:
        Test('{} ({})'.format(trinket.name, trinket.item_id), group=grp, args=[
            ('trinket1', '{},id={},ilevel={}'.format(trinket.name, trinket.item_id, trinket.min_itemlevel))])


available_tests = {
    "talent": (test_talents, None),
    "covenant": (test_covenants, "simc-support"),
    "trinket": (test_trinkets, "simc-support")
}

formatted_available_tests = ", ".join(("'{}'{}".format(
    key, "" if data[1] is None else " (requires {})".format(data[1])) for key, data in available_tests.items()))

parser = argparse.ArgumentParser(description='Run simc tests')
parser.add_argument('specialization', metavar='spec', type=str,
                    help='Simc specialization in the form of CLASS_SPEC, eg. Priest_Shadow')
parser.add_argument('-tests', nargs='+', default=["talent"], required=False,
                    help='Tests to run. Available tests: {}'.format(formatted_available_tests))
parser.add_argument('--trinkets-fight-style', default='DungeonSlice', type=str,
                    help='Fight style used for trinket simulations.')
args = parser.parse_args()


klass = args.specialization

print(' '.join(klass.split('_')))

tests = []
profiles = list(find_profiles(klass))
if len(profiles) == 0:
    print("No profile found for {}".format(klass))

for profile, path in profiles:
    for test in args.tests:
        if test in available_tests:
            available_tests[test][0](klass, path)
        else:
            print("Could not find test {}".format(test))


sys.exit(run(tests))
