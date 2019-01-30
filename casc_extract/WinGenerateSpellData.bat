call py -3 casc_extract.py -m batch --cdn -o wow
cd wow
dir /b /a:D /O:-D>hi.txt
set /p wowdir=<hi.txt
del hi.txt
cd ..
cd ..
set curr=%cd%
cd dbc_extract3
robocopy "C:\Program Files (x86)\Warcraft\Cache\ADB\enUS" "%cd%\cache\live" DBCache.bin
robocopy "C:\World of Warcraft\Cache\ADB\enUS" "%cd%\cache\live" DBCache.bin
call generate.bat %wowdir% "%curr%\casc_extract\wow"
pause