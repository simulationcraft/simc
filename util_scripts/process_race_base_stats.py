#!/usr/bin/python

import re, sys
import numpy as np

# this is the ordering of the classes in the data file \base_stats\race_mods.txt
# note that I've added a column of zeroes for 'No Race', just in case the human modifiers were to change
WOWRACES = ['None', 'Human', 'Dwarf', 'Night Elf', 'Orc', 'Tauren', 'Undead', 'Gnome', 'Troll', 'Blood Elf', 'Draenei', 'Goblin', 'Worgen', 'Pandaren']
# this is the order of the classes in our own data structure (consistent with blizzard's dbc?)
RACENAMES = ['No Race', 'Human', 'Orc', 'Dwarf', 'Night Elf', 'Undead', 'Tauren', 'Gnome', 'Troll', 'Goblin', 'Blood Elf', 'Draenei', 'Fel Orc', 'Naga', 'Broken', 'Skeleton', 'Vrykul', 'Tuskarr', 'Forest Troll', 'Tanuka', 'Skeleton', 'Ice Troll', 'Worgen', 'Gilnean', 'Pandaren', 'Pandaren - Alliance', 'Pandaren - Horde']
# this mask converts between the two; e.g. RACENUMMASK[2] tells us which column of the matrix contains the Orc data
RACENUMMASK=[ 0, 1, 4, 2, 3, 6, 5, 7, 8, 11, 9, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 13, 13, 13]


def import_data( filename ):
    # Imports the data as a long 1-D array
    with open(filename) as f:
        str = f.read()
        str = re.sub("\n", " ", str )
        return str.split( ' ', )

def output_stat( filename, stats ):
    # Takes the 2-D array and constructs the text output line by line
    with open(filename,"w") as f:
        f.write( """//  Racial Base Stat Modifiers (Str Agi Sta Int Spi)
static stat_data_t __gt_race_stats[] = \n{\n""")
        f.write("  // Str Agi Sta Int Spi\n")
        for raceindex in range(len(RACENUMMASK)):
            racenum = RACENUMMASK[raceindex]
            stats_str = "  { "
            for stat in range(5):
                stats_str += " {:2.0f}".format(float(stats[racenum,stat])) 
                if stat < 4:
                    stats_str += ","
                else:
                    stats_str += " "
            stats_str += "},"
            stats_str += " // %s\n"%(RACENAMES[raceindex])
            f.write( stats_str )
        f.write( "};")
        
def main():
    stats = import_data( "base_stats/race_mods.txt")
    matrix = np.array(stats).reshape(5,-1).transpose()
    output_stat("base_stats/race_out.txt", matrix )
    # For debugging - this list should match the order in sc_util.cpp::translate_class_id
    print("Race Order (for double-checking)")
    for raceindex in range(len(RACENUMMASK)):
        if RACENUMMASK[raceindex]>0:
            print(WOWRACES[RACENUMMASK[raceindex]])
    print("done")

if __name__ == "__main__":
    main()