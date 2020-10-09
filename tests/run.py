#!/usr/bin/env python3

import sys
import argparse
import random
try:
    import simc_support
    simc_support_available = True
except ImportError:
    simc_support_available = False

from helper import Test, TestGroup, run, find_profiles
from talent_options import talent_combinations

FIGHT_STYLES = ('Patchwerk', 'DungeonSlice', 'HeavyMovement')


def test_talents(klass: str, path: str):
    for fight_style in FIGHT_STYLES:
        grp = TestGroup('{}/{}/talents'.format(profile, fight_style),
                        fight_style=fight_style, profile=path)
        tests.append(grp)
        for talents in talent_combinations(klass):
            Test(talents, group=grp, args=[('talents', talents)])


def test_covenants(klass: str, path: str):
    if not simc_support_available:
        print("Error importing simc-support library. Skipping covenant tests.")
        return

    from simc_support.game_data import WowSpec
    from simc_support.game_data import Covenant
    from simc_support.game_data import Conduit
    spec = WowSpec.get_wow_spec_from_combined_simc_name(klass)
    conduits = Conduit.get_conduits_for_spec(spec)
    for fight_style in FIGHT_STYLES:
        grp = TestGroup('{}/{}/covenants'.format(profile, fight_style),
                        fight_style=fight_style, profile=path)
        tests.append(grp)

        # Test covenants
        for covenant in Covenant.COVENANTS:
            Test(covenant.full_name, group=grp,
                 args=[('covenant', covenant.simc_name), ('level', 60)])
            # Add conduits specific to selected covenant
            for conduit in conduits:
                if len(conduit.covenants) == 1 and covenant == conduit.covenants[0]:
                    rank = random.choice(conduit.ranks) + 1
                    soulbind_argument = '{}:{}'.format(conduit.id, rank)
                    Test('{} - {} ({}) Rank {}'.format(covenant.full_name, conduit.full_name, conduit.id, rank), group=grp, args=[
                        ('covenant', covenant.simc_name), ('level', 60), ('soulbind', soulbind_argument)])

        # test conduits available for all 4 covenants independent from covenant selection.
        for conduit in conduits:
            if len(conduit.covenants) == 4:
                rank = random.choice(conduit.ranks) + 1
                soulbind_argument = '{}:{}'.format(conduit.id, rank)
                Test('{} ({}) Rank {}'.format(conduit.full_name, conduit.id, rank), group=grp, args=[
                    ('level', 60), ('soulbind', soulbind_argument)])


def test_trinkets(klass: str, path: str):
    if not simc_support_available:
        print("Error importing simc-support library. Skipping trinket tests.")
        return
    
    from simc_support.game_data import WowSpec
    from simc_support.game_data import Trinket
    spec = WowSpec.get_wow_spec_from_combined_simc_name(klass)
    trinkets = Trinket.get_trinkets_for_spec(spec)
    fight_style = args.trinkets_fight_style
    grp = TestGroup('{}/{}/trinkets'.format(profile, fight_style),
                    fight_style=fight_style, profile=path)
    tests.append(grp)
    for trinket in trinkets:
        Test('{} ({})'.format(trinket.name, trinket.item_id), group=grp, args=[
            ('trinket1', '"{}",id={},ilevel={}'.format(trinket.name, trinket.item_id, trinket.min_itemlevel))])

def test_legendaries(klass: str, path: str):
    if not simc_support_available:
        print("Error importing simc-support library. Skipping legendary tests.")
        return

    from simc_support.game_data import WowSpec
    from simc_support.game_data import Legendary
    spec = WowSpec.get_wow_spec_from_combined_simc_name(klass)
    legendaries = Legendary.get_legendaries_for_spec(spec)
    for fight_style in FIGHT_STYLES:
        grp = TestGroup('{}/{}/legendaries'.format(profile, fight_style),
                        fight_style=fight_style, profile=path)
        tests.append(grp)
        for legendary in legendaries:
            Test('{} ({} / {})'.format(legendary.full_name, legendary.id, legendary.bonus_id), group=grp, args=[
                ('trinket1', '"{}",bonus_id={}'.format(legendary.full_name, legendary.bonus_id))])

def test_soulbinds(klass: str, path: str):
    if not simc_support_available:
        print("Error importing simc-support library. Skipping soulbind tests.")
        return

    from simc_support.game_data import Covenant
    from simc_support.game_data import SoulBind
    fight_style = args.soulbind_fight_style
    for covenant in Covenant.COVENANTS:
        grp = TestGroup('{}/{}/soulbinds'.format(profile, fight_style),
                        fight_style=fight_style, profile=path)
        tests.append(grp)
        for soulbind in SoulBind.SOULBINDS:
            if covenant == soulbind.covenant:
                for soulbind_talent in soulbind.soul_bind_talents:
                    if soulbind_talent.spell_id != 0:
                        Test('{} - {} - {} ({})'.format(covenant.full_name, soulbind.full_name, soulbind_talent.full_name, soulbind_talent.spell_id), group=grp, args=[
                            ('covenant', covenant.simc_name), ('level', 60), ('soulbind', '{},{}'.format(soulbind.simc_name, soulbind_talent.spell_id))])

available_tests = {
    "talent": (test_talents, None),
    "covenant": (test_covenants, "simc-support"),
    "trinket": (test_trinkets, "simc-support"),
    "legendary": (test_legendaries, "simc-support"),
    "soulbind": (test_soulbinds, "simc-support")
}

formatted_available_tests = ", ".join(("'{}'{}".format(
    key, "" if data[1] is None else " (requires {})".format(data[1])) for key, data in available_tests.items()))

parser = argparse.ArgumentParser(description='Run simc tests.')
parser.add_argument('specialization', metavar='spec', type=str,
                    help='Simc specialization in the form of CLASS_SPEC, eg. Priest_Shadow')
parser.add_argument('-tests', nargs='+', default=["talent"], required=False,
                    help='Tests to run. Available tests: {}'.format(formatted_available_tests))
parser.add_argument('--trinkets-fight-style', default='DungeonSlice', type=str,
                    help='Fight style used for trinket simulations.')
parser.add_argument('--soulbind-fight-style', default='DungeonSlice', type=str,
                    help='Fight style used for soulbind simulations.')
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
