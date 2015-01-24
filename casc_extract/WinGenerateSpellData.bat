call python casc_extract.py -m batch --cdn -o wow
cd wow
dir /b /a:D /O:-D>hi.txt
set /p wowdir=<hi.txt
set /p oldname=<hi.txt
set wowdir=%wowdir:~4,-17%
ren %oldname% %wowdir%
del hi.txt
cd ..
cd ..
cd dbc_extract
call generate.bat %wowdir% E:\simulationcraft\casc_extract\wow\
pause