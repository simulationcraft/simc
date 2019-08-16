call py -3 casc_extract.py -m batch --cdn --ptr -o wow_ptr
cd wow_ptr
dir /b /a:D /O:-D>hi.txt
set /p wowdir=<hi.txt
del hi.txt
cd ..
cd ..
set curr=%cd%
cd dbc_extract3
robocopy "C:\Program Files (x86)\World of Warcraft\_ptr_\Cache\ADB\enUS" "%cd%\cache\live" DBCache.bin
robocopy "C:\Program Files (x86)\Warcraft\_ptr_\Cache\ADB\enUS" "%cd%\cache\live" DBCache.bin
robocopy "C:\World of Warcraft\_ptr_\Cache\ADB\enUS" "%cd%\cache\live" DBCache.bin
call generate.bat ptr %wowdir% %curr%\casc_extract\wow_ptr
pause