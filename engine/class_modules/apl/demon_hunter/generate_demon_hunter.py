import os
import sys
sys.path.append(os.path.join(os.path.curdir, '..'))
from ConvertAPL import main

if __name__ == '__main__':
    print('Converting Havoc')
    main(['-i', 'havoc.simc', '-o', os.path.join(os.path.curdir, '..', 'apl_demon_hunter.cpp'), '-s', 'havoc'])
    # main(['-i', 'havoc_ptr.simc', '-o', os.path.join(os.path.curdir, '..', 'apl_demon_hunter.cpp'), '-s', 'havoc_ptr'])
    print('Converting Vengeance')
    main(['-i', 'vengeance.simc', '-o', os.path.join(os.path.curdir, '..', 'apl_demon_hunter.cpp'), '-s', 'vengeance'])
    # main(['-i', 'vengeance_ptr.simc', '-o', os.path.join(os.path.curdir, '..', 'apl_demon_hunter.cpp'), '-s', 'vengeance_ptr'])
    print('Done!')
