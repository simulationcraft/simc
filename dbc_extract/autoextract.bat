@echo off

setlocal enabledelayedexpansion

set PTR=
if not %1 == ptr goto start
set PTR= ptr
shift

:start
set INPATH=%~f1\Data\enUS
set OUTPATH=%~f2
set CACHEPATH=%~f3

if exist "%INPATH%" goto next
echo Error: Unable to find WoW path! %INPATH%
echo.
goto usage

:next
if exist "%OUTPATH%" goto okay
echo Error: Unable to find output path!
echo.
goto usage

:okay
set FILES=locale-enUS.MPQ
for %%f in ("%INPATH%\wow-update-enUS*.MPQ") do (
set FILES=!FILES! %%~nf.MPQ
)
set BUILD=%FILES:~-9,5%

echo cd "%INPATH%" > tmp.mopaq
echo op %FILES% >> tmp.mopaq
echo e locale-enUS.MPQ DBFilesClient\* "%OUTPATH%\%BUILD%" /fp >> tmp.mopaq

mkdir "%OUTPATH%\%BUILD%"
MPQEditor.exe /console tmp.mopaq
del tmp.mopaq

if exist "%CACHEPATH%" copy "%CACHEPATH%\Item-sparse.adb" "%OUTPATH%\%BUILD%\DBFilesClient\"

echo Generating...
generate.bat%PTR% %BUILD% "%OUTPATH%"

goto end

:usage
echo Usage: autoextract.bat [ptr] wowpath outpath [cachepath]

:end
endlocal