@echo off

echo Generating Havoc
py "..\ConvertAPL.py" -i havoc.simc -o "..\apl_demon_hunter.cpp" -s havoc
@REM py "..\ConvertAPL.py" -i havoc_ptr.simc -o "..\apl_demon_hunter.cpp" -s havoc_ptr

echo Generating Vengeance
py "..\ConvertAPL.py" -i vengeance.simc -o "..\apl_demon_hunter.cpp" -s vengeance
@REM py "..\ConvertAPL.py" -i vengeance_ptr.simc -o "..\apl_demon_hunter.cpp" -s vengeance_ptr

echo Done!
pause >nul