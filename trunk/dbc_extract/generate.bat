@echo off

setlocal

set INEXT=DBFilesClient
set OUTPATH=%~dp0..\engine
set RUNFILE=%~dp0\dbc_extract.py


set PTR=
set PTREXT=
if not %1 == ptr goto next
set PTR= --prefix=ptr
set PTREXT=_ptr
shift

:next
set BUILD=%1
set INPATH=%~f2\%BUILD%\%INEXT%

if exist %INPATH% goto okay
echo Error: Unable to find input files!
echo.
goto usage
:okay

python.exe %RUNFILE% -p %INPATH% -b %BUILD% %PTR%  -t talent                  > %OUTPATH%\sc_talent_data%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% --itemcache=%INPATH% -b %BUILD% %PTR%  -t spell > %OUTPATH%\sc_spell_data%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% -b %BUILD% %PTR%  -t scale                   > %OUTPATH%\sc_scale_data%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% -b %BUILD% %PTR%  -t class_list              > %OUTPATH%\sc_spell_lists%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% -b %BUILD% %PTR%  -t spec_spell_list        >> %OUTPATH%\sc_spell_lists%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% -b %BUILD% %PTR%  -t mastery_list           >> %OUTPATH%\sc_spell_lists%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% -b %BUILD% %PTR%  -t racial_list            >> %OUTPATH%\sc_spell_lists%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% -b %BUILD% %PTR%  -t glyph_list             >> %OUTPATH%\sc_spell_lists%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% -b %BUILD% %PTR%  -t set_list               >> %OUTPATH%\sc_spell_lists%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% --itemcache=%INPATH% -b %BUILD% %PTR%  -t item > %OUTPATH%/sc_item_data%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% -b %BUILD% %PTR%  -t random_property_points >> %OUTPATH%/sc_item_data%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% -b %BUILD% %PTR%  -t random_suffix          >> %OUTPATH%/sc_item_data%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% -b %BUILD% %PTR%  -t item_ench              >> %OUTPATH%/sc_item_data%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% -b %BUILD% %PTR%  -t item_armor             >> %OUTPATH%/sc_item_data%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% -b %BUILD% %PTR%  -t weapon_damage          >> %OUTPATH%/sc_item_data%PTREXT%.inc
python.exe %RUNFILE% -p %INPATH% -b %BUILD% %PTR%  -t gem_properties         >> %OUTPATH%/sc_item_data%PTREXT%.inc

echo Done!

goto end

:usage
echo Usage: generate.bat [ptr] <build> <inpath>

:end
endlocal
