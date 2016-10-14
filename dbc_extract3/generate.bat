@echo off
chcp 65001
setlocal

set OUTPATH=%~dp0..\engine\dbc\generated
set RUNFILE=%~dp0\dbc_extract.py
set CACHEDIR=%~dp0\cache\live

set PTR=
set PTREXT=
if not %1 == ptr goto next
set PTR= --prefix=ptr
set PTREXT=_ptr
set CACHEDIR=%~dp0\cache\ptr
shift

:next
set BUILD=%1
set INPATH=%~f2\%BUILD%\DBFilesClient
set GTINPATH=%~f2\%BUILD%\GameTables

if exist %INPATH% goto okay
echo Error: Unable to find input files! %INPATH%
echo.
goto usage
:okay


py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR% -t spec_list                  > %OUTPATH%\sc_spec_list%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t talent                  > %OUTPATH%\sc_talent_data%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t spell > %OUTPATH%\sc_spell_data%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t rppm_coeff              >> %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %GTINPATH% -b %BUILD% %PTR%  -t scale                   > %OUTPATH%\sc_scale_data%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t class_list              > %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t spec_spell_list        >> %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t mastery_list           >> %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t racial_list            >> %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t set_list2              >> %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t rppm_coeff             >> %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t artifact               >> %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item                    > %OUTPATH%/sc_item_data%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t random_property_points  > %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t random_suffix          >> %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item_ench              >> %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item_armor             >> %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t weapon_damage          >> %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t gem_properties         >> %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item_upgrade           >> %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item_bonus             >> %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item_scaling           >> %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item_name_desc         >> %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item_child             >> %OUTPATH%/sc_item_data%PTREXT%2.inc

py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR% -t spec_enum > %OUTPATH%\sc_specialization_data%PTREXT%.inc

echo Done!

goto end

:usage
echo Usage: generate.bat [ptr] <build> <inpath>

:end
endlocal
