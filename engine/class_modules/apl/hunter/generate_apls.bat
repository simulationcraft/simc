@echo off

echo Generating MM
py "..\ConvertAPL.py" -i "mm.txt" -o "..\apl_hunter.cpp" -s marksmanship
echo Generating BM
py "..\ConvertAPL.py" -i "bm.txt" -o "..\apl_hunter.cpp" -s beast_mastery
echo Generating SV
py "..\ConvertAPL.py" -i "sv.txt" -o "..\apl_hunter.cpp" -s survival

echo Done
pause >nul
