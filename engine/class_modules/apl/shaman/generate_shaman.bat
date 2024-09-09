@echo off

echo Generating Elemental
py "..\ConvertAPL.py" -i elemental.simc -o "..\apl_shaman.cpp" -s elemental
py "..\ConvertAPL.py" -i elemental_ptr.simc -o "..\apl_shaman.cpp" -s elemental_ptr

echo Done!
pause >nul
