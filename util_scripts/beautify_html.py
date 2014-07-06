#!/usr/bin/python

import sys
from bs4 import BeautifulSoup

def transform( input_file, output_file ):
    
    with open( input_file, "r" ) as f:
        c = f.read()
        soup = BeautifulSoup(c)
        with open( output_file, 'w' ) as o:
            o.write( soup.prettify() )

def main():
    if len(sys.argv) <= 2:
        print( "% input_file output_file"%(sys.argv[0]))
        return
    transform( sys.argv[ 1 ], sys.argv[ 2 ] )
    
if __name__ == "__main__":
    main()
    sys.exit()
    