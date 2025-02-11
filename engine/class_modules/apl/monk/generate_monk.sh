#!/usr/bin/env bash

echo "Converting Windwalker"
python3 '../ConvertAPL.py' -i windwalker.simc -o '../apl_monk.cpp' -s windwalker
python3 '../ConvertAPL.py' -i windwalker_ptr.simc -o '../apl_monk.cpp' -s windwalker_ptr

echo Done!
read -rsp $'Press enter to continue...\n'
