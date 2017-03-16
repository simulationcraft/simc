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


py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR% -t spec_list               -o %OUTPATH%\sc_spec_list%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t talent                 -o %OUTPATH%\sc_talent_data%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t spell                  -o %OUTPATH%\sc_spell_data%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t rppm_coeff             -a %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %GTINPATH% -b %BUILD% %PTR%  -t scale                                   -o %OUTPATH%\sc_scale_data%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t class_list             -o %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t spec_spell_list        -a %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t mastery_list           -a %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t racial_list            -a %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t set_list2              -a %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t rppm_coeff             -a %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t artifact               -a %OUTPATH%\sc_spell_lists%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item                   -o %OUTPATH%/sc_item_data%PTREXT%.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t random_property_points -o %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t random_suffix          -a %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item_ench              -a %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item_armor             -a %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t weapon_damage          -a %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t gem_properties         -a %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item_upgrade           -a %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item_bonus             -a %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item_scaling           -a %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item_name_desc         -a %OUTPATH%/sc_item_data%PTREXT%2.inc
py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR%  -t item_child             -a %OUTPATH%/sc_item_data%PTREXT%2.inc

py -3  %RUNFILE% -p %INPATH% -b %BUILD% --cache %CACHEDIR% %PTR% -t spec_enum               -o %OUTPATH%\sc_specialization_data%PTREXT%.inc

echo Done!

goto end

:usage
echo Usage: generate.bat [ptr] <build> <inpath>

:end
endlocal
