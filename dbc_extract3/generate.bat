@echo off
chcp 65001
setlocal

set OUTPATH="%~dp0..\engine\dbc\generated"
set RUNFILE="%~dp0\dbc_extract.py"
set CACHEFILE="%~dp0\cache\live\DBCache.bin"
set BATCHFILE="%~dp0\live.conf"
set FORMATFILE=

set PTR=
set PTREXT=
if not %1 == ptr goto next
set PTR= --prefix=ptr
set PTREXT=_ptr
set CACHEFILE="%~dp0\cache\ptr\DBCache.bin"
set BATCHFILE="%~dp0\ptr.conf"
shift

:next
set BUILD=%1
set INPATH="%~f2\%BUILD%\DBFilesClient"
set GTINPATH="%~f2\%BUILD%\GameTables"
if not %1 == ptr set FORMATFILE="-f%~dp0\formats\26788.json"

if exist %INPATH% goto okay
echo Error: Unable to find input files! %INPATH%
echo.
goto usage
:okay

py -3 %RUNFILE% -p %GTINPATH% -b %BUILD% %FORMATFILE%                      %PTR% -t scale  -o %OUTPATH%\sc_scale_data%PTREXT%.inc
py -3 %RUNFILE% -p %INPATH%   -b %BUILD% %FORMATFILE% --hotfix=%CACHEFILE% %PTR% -t output %BATCHFILE%

echo Done!

goto end

:usage
echo Usage: generate.bat [ptr] <build> <inpath>

:end
endlocal
