@echo off

setlocal enabledelayedexpansion
set LANG=enUS

set PTR=
if not %1 == ptr goto start
set PTR= ptr
shift

:start
set INPATH_A=%~f1\Data
set INPATH_B=%~f1\Data\%LANG%
set OUTPATH=%~f2
set CACHEPATH=%~f3

if exist "%INPATH_A%" goto pathb
echo Error: Unable to find WoW path! %INPATH_A%
echo.
goto usage

:pathb
if exist "%INPATH_B%" goto next
echo Error: Unable to find WoW path! %INPATH_B%
echo.
goto usage

:next
if exist "%OUTPATH%" goto okay
echo Error: Unable to find output path!
echo.
goto usage

:okay
set FILES=misc.MPQ
for %%f in ("%INPATH_A%\wow-update-base*.MPQ") do (
set FILES=!FILES! %%~nf.MPQ
)
set BUILD=%FILES:~-9,5%

echo cd "%INPATH_A%" > tmp1.mopaq
echo op %FILES% >> tmp1.mopaq
echo e misc.MPQ DBFilesClient\* "%OUTPATH%\%BUILD%" /fp >> tmp1.mopaq

set FILES=locale-%LANG%.MPQ
for %%f in ("%INPATH_B%\wow-update-%LANG%*.MPQ") do (
set FILES=!FILES! %%~nf.MPQ
)
set BUILD=%FILES:~-9,5%

echo cd "%INPATH_B%" > tmp2.mopaq
echo op %FILES% >> tmp2.mopaq
echo e locale-%LANG%.MPQ DBFilesClient\* "%OUTPATH%\%BUILD%" /fp >> tmp2.mopaq

mkdir "%OUTPATH%\%BUILD%"
MPQEditor.exe /console tmp1.mopaq
del tmp1.mopaq
MPQEditor.exe /console tmp2.mopaq
del tmp2.mopaq

if exist "%CACHEPATH%" copy "%CACHEPATH%\Item-sparse.adb" "%OUTPATH%\%BUILD%\DBFilesClient\"

echo Generating...
generate.bat%PTR% %BUILD% "%OUTPATH%"

goto end

:usage
echo Usage: autoextract.bat [ptr] wowpath outpath [cachepath]

:end
endlocal