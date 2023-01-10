import os
import sys
sys.path.append(os.path.join(os.path.curdir, '..'))
from ConvertAPL import main

if __name__ == '__main__':
    print('Converting Affliction')
    main(['-i', 'affliction.simc', '-o', os.path.join(os.path.curdir, '..', 'apl_warlock.cpp'), '-s', 'affliction'])
    print('Converting Demonology')
    main(['-i', 'demonology.simc', '-o', os.path.join(os.path.curdir, '..', 'apl_warlock.cpp'), '-s', 'demonology'])
    print('Converting Destruction')
    main(['-i', 'destruction.simc', '-o', os.path.join(os.path.curdir, '..', 'apl_warlock.cpp'), '-s', 'destruction'])
    print('Done!')
