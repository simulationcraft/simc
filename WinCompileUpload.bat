:: Used to automate everything for Alex so he can be lazy.

For /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set mydate=%%a-%%b)
git log --no-merges -1 --pretty="%%h">bla.txt
set /p revision=<bla.txt
del bla.txt

call win32_release_msvc12.bat
set install=simc-602-5-win32
set filename=%install%-%mydate%-%revision%.zip
7z a -r -mx9 %filename% %install%
call start winscp /command "open downloads" "put E:\Simulationcraft\%filename% -nopreservetime -nopermissions -transfer=binary" "exit"

call win64_release_msvc12.bat
set install=simc-602-5-win64
set filename=%install%-%mydate%-%revision%.zip
7z a -r -mx9 %filename% %install%
winscp /command "open downloads" "put E:\Simulationcraft\%filename% -nopreservetime -nopermissions -transfer=binary" "exit"
for /d %i in (simc-6*) do rd "%~i" /s /q
pause