:: Used to automate everything for Alex so he can be lazy.

git clean -f -x -d

For /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set mydate=%%a-%%b)
git log --no-merges -1 --pretty="%%h">bla.txt
set /p revision=<bla.txt
del bla.txt

set install=simc-603-20-

robocopy . %install%source /s *.* /xd .git %install%source /xf *.pgd /xn
set filename=%install%source.zip
7z a -r -mx9 %filename% %install%source
call start winscp /command "open downloads" "put E:\Simulationcraft\%filename% -nopreservetime -nopermissions -transfer=binary" "exit"

call win32_release_msvc12.bat
set filename=%install%-%mydate%-%revision%.zip
7z a -r -mx9 %filename% %install%
call start winscp /command "open downloads" "put E:\Simulationcraft\%filename% -nopreservetime -nopermissions -transfer=binary" "exit"

call win64_release_msvc12.bat
set filename=%install%-%mydate%-%revision%.zip
7z a -r -mx9 %filename% %install%
winscp /command "open downloads" "put E:\Simulationcraft\%filename% -nopreservetime -nopermissions -transfer=binary" "exit"
pause