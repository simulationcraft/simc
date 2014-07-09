#!/usr/bin/python

import re, sys
import numpy as np

WOWCLASSES = ["No Class", 'Death Knight', 'Druid', 'Hunter', 'Mage', 'Monk', 'Paladin', 'Priest', 'Rogue', 'Shaman', 'Warlock', 'Warrior']

def import_data( filename ):
    with open(filename) as f:
        str = f.read()
        str = re.sub("\n", " ", str )
        return str.split( ' ', )

def output_stat( filename, stats ):
    with open(filename,"w") as f:
        f.write( """//      Str     Agi     Sta     Int     Spi
static stat_data_t __gt_class_stats_by_level[][100] = \n{\n""")
        for classnum in range(len(WOWCLASSES)):
            f.write("  { // %s\n"%(WOWCLASSES[classnum]) )
            f.write("    //    Str     Agi     Sta     Int     Spi\n")
            for level in range(100):
                stats_str = "    {"
                for stat in range(5):
                    stats_str += " {:6.1f}".format(float(stats[stat,level,classnum])) + ","
                stats_str += "},"
                if (level +1)% 5 == 0:
                    stats_str += " // " + str(level+1)
                stats_str += "\n"
                f.write( stats_str )
            f.write("  },\n")
        f.write( "};")
        
def main():
    stats = import_data( "base_stats/stats.txt")
    matrix = np.array(stats).reshape(5,100,-1)
    matrix[:,:,0] = np.zeros(100)
    output_stat("base_stats/out.txt", matrix )
    print("done")
    

if __name__ == "__main__":
    main()