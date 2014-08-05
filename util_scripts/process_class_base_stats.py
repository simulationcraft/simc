#!/usr/bin/python

import re, sys
import numpy as np

# this is the ordering of the classes in the data file \base_stats\class_stats.txt
WOWCLASSES = ["No Class", 'Death Knight', 'Druid', 'Hunter', 'Mage', 'Monk', 'Paladin', 'Priest', 'Rogue', 'Shaman', 'Warlock', 'Warrior']
# this mask corresponds to order in sc_util.cpp::translate_class_id
# e.g. CLASSNUMMASK[2] tells us where to find the data for a Paladin, which is class number 2 in translate_class_id
CLASSNUMMASK = [ 0, 11, 6, 3, 8, 7, 1, 9, 4, 10, 5, 2 ]

def import_data( filename ):
    # Imports the data as a long 1-D array
    with open(filename) as f:
        str = f.read()
        str = re.sub("\n", " ", str )
        return str.split( ' ', )

def output_stat( filename, stats ):
    # Takes the 3-D array and constructs the text output line by line
    with open(filename,"w") as f:
        f.write( """//  Base Stats (Str Agi Sta Int Spi)
static stat_data_t __gt_class_stats_by_level[][100] = \n{\n""")
        for classnum in CLASSNUMMASK:            
            f.write("  { // %s\n"%(WOWCLASSES[classnum]) )
            f.write("    //    Str     Agi     Sta     Int     Spi\n")
            for level in range(100):
                stats_str = "    {"
                for stat in range(5):
                    stats_str += " {:6.0f}".format(float(stats[stat,level,classnum])) 
                    if stat < 4:
                        stats_str += ","
                    else:
                        stats_str += " "
                stats_str += "},"
                if (level +1)% 5 == 0:
                    stats_str += " // " + str(level+1)
                stats_str += "\n"
                f.write( stats_str )
            f.write("  },\n")
        f.write( "};")
        
def main():
    stats = import_data( "base_stats/class_stats.txt")
    matrix = np.array(stats).reshape(5,100,-1)
    # tricksy: since the data includes level numbers, so matrix[:,:,0] is a useless array of 1-100's.
    # We can just overwrite this with zeros for our 'No Class' entry
    matrix[:,:,0] = np.zeros(100) 
    output_stat("base_stats/class_out.txt", matrix )
    # For debugging - this list should match the order in sc_util.cpp::translate_class_id
    print("Class Order (for double-checking)")
    for classnum in CLASSNUMMASK:
        print(WOWCLASSES[classnum])
    print("done")
    

if __name__ == "__main__":
    main()