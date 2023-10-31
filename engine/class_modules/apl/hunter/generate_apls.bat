@echo off

echo Generating MM
py "..\ConvertAPL.py" -i "mm.txt" -o "..\apl_hunter.cpp" -s marksmanship
echo Generating MM PTR
py "..\ConvertAPL.py" -i "mm_ptr.txt" -o "..\apl_hunter.cpp" -s marksmanship_ptr
echo Generating BM
py "..\ConvertAPL.py" -i "bm.txt" -o "..\apl_hunter.cpp" -s beast_mastery
echo Generating BM PTR
py "..\ConvertAPL.py" -i "bm_ptr.txt" -o "..\apl_hunter.cpp" -s beast_mastery_ptr
echo Generating SV
py "..\ConvertAPL.py" -i "sv.txt" -o "..\apl_hunter.cpp" -s survival
echo Generating SV PTR
py "..\ConvertAPL.py" -i "sv_ptr.txt" -o "..\apl_hunter.cpp" -s survival_ptr

echo Done
pause >nul
