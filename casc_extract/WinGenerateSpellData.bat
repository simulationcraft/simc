call python casc_extract.py -m batch --cdn --beta -o wow
cd wow
dir /b /a:D /O:-D>hi.txt
set /p wowdir=<hi.txt
set /p oldname=<hi.txt
set wowdir=%wowdir:~4,-15%
ren %oldname% %wowdir%
cd ..
cd ..
cd dbc_extract
generate.bat %wowdir% E:\simulationcraft\casc_extract\wow\
pause