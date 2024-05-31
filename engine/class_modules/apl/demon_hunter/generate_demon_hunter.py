import os
import sys
sys.path.append(os.path.join(os.path.curdir, '..'))
from ConvertAPL import main

if __name__ == '__main__':
    print('Converting Havoc')
    main(['-i', 'havoc.simc', '-o', os.path.join(os.path.curdir, '..', 'apl_demon_hunter.cpp'), '-s', 'havoc'])
    print('Converting Vengeance')
    main(['-i', 'vengeance.simc', '-o', os.path.join(os.path.curdir, '..', 'apl_demon_hunter.cpp'), '-s', 'vengeance'])
    main(['-i', 'vengeance_aldrachi_reaver.simc', '-o', os.path.join(os.path.curdir, '..', 'apl_demon_hunter.cpp'), '-s', 'vengeance_aldrachi_reaver'])
    main(['-i', 'vengeance_felscarred.simc', '-o', os.path.join(os.path.curdir, '..', 'apl_demon_hunter.cpp'), '-s', 'vengeance_felscarred'])
    print('Done!')
