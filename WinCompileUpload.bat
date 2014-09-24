:: Used to automate everything for Alex so he can be lazy.

For /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set mydate=%%a-%%b)
git log --no-merges -1 --pretty="%%h">bla.txt
set /p revision=<bla.txt
del bla.txt


call win32_release_msvc12.bat
set filename=E:\simulationcraft\simc-602-alpha-win32-%mydate%-%revision%.zip
call start winscp /command "open downloads" "put %filename% -nopreservetime -nopermissions -transfer=binary" "exit"

call win64_release_msvc12.bat
set filename=E:\simulationcraft\simc-602-alpha-win64-%mydate%-%revision%.zip
winscp /command "open downloads" "put %filename% -nopreservetime -nopermissions -transfer=binary" "exit"
pause