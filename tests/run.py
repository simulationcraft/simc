#!/usr/bin/env python3

import sys
import argparse
import random

try:
    from simc_support.game_data import WowSpec
    from simc_support.game_data import Trinket
    from simc_support.game_data import Season
except ImportError:
    raise Exception(
        "simc-support dependency missing. Please install using 'pip3 install -r requirements.txt'"
    )

from helper import Test, TestGroup, run, find_profiles

FIGHT_STYLES = ("Patchwerk", "DungeonSlice", "HeavyMovement",)
SEASON = Season.Season.SEASON_1


def test_trinkets(klass: str, path: str):
    spec = WowSpec.get_wow_spec_from_combined_simc_name(klass)
    trinkets = Trinket.get_trinkets_for_spec(spec)
    seasonal_trinkets = [t for t in trinkets if SEASON in t.seasons]
    fight_style = args.trinkets_fight_style
    grp = TestGroup(
        "{}/{}/trinkets".format(profile, fight_style),
        fight_style=fight_style,
        profile=path,
    )
    tests.append(grp)
    for trinket in seasonal_trinkets:
        Test(
            "{} ({})".format(trinket.name, trinket.item_id),
            group=grp,
            args=[
                (
                    "trinket1",
                    "{},id={},ilevel={}".format(
                        trinket.simc_name, trinket.item_id, trinket.min_itemlevel
                    ),
                )
            ],
        )


available_tests = {
    "trinket": test_trinkets,
}

parser = argparse.ArgumentParser(description="Run simc tests.")
parser.add_argument(
    "specialization",
    metavar="spec",
    type=str,
    help="Simc specialization in the form of CLASS_SPEC, eg. Priest_Shadow",
)
parser.add_argument(
    "-tests",
    nargs="+",
    default=["trinket"],
    required=False,
    help="Tests to run. Available tests: {}".format(
        [t for t in available_tests.keys()]
    ),
)
parser.add_argument(
    "--trinkets-fight-style",
    default="DungeonSlice",
    type=str,
    help="Fight style used for trinket simulations.",
)
# parser.add_argument('--soulbind-fight-style', default='DungeonSlice', type=str,
#                     help='Fight style used for soulbind simulations.')
parser.add_argument(
    "--max-profiles-to-use",
    default="0",
    type=int,
    help="Maximum number of profiles to use per spec. 0 means use all available profiles",
)
args = parser.parse_args()


klass = args.specialization

print(" ".join(klass.split("_")))

tests = []
profiles = list(find_profiles(klass))
if args.max_profiles_to_use != 0:
    profiles = profiles[: args.max_profiles_to_use]

if len(profiles) == 0:
    print("No profile found for {}".format(klass))

for profile, path in profiles:
    for test in args.tests:
        if test in available_tests:
            available_tests[test](klass, path)
        else:
            print("Could not find test {}".format(test))


sys.exit(run(tests))
