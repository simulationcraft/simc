:: Used to automate everything for Alex so he can be lazy.

git clean -f -x -d

For /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set mydate=%%a-%%b)
git log --no-merges -1 --pretty="%%h">bla.txt
set /p revision=<bla.txt
del bla.txt

set simcversion=603-20
set install=simc-%simcversion%-source

robocopy . %install% /s *.* /xd .git %install% /xf *.pgd /xn
set filename=%install%.zip
7z a -r -mx9 %install%.zip %install%
call start winscp /command "open downloads" "put E:\Simulationcraft\%filename% -nopreservetime -nopermissions -transfer=binary" "exit"

set install=SimulationcraftSetup-%simcversion%-win32
call win32_release_msvc12.bat
iscc.exe "setup32.iss"
call start winscp /command "open downloads" "put E:\Simulationcraft\Output\%install%.exe -nopreservetime -nopermissions -transfer=binary" "exit"

set install=SimulationcraftSetup-%simcversion%-win64
call win64_release_msvc12.bat
iscc.exe "setup64.iss"
winscp /command "open downloads" "put E:\Simulationcraft\Output\%install%.exe -nopreservetime -nopermissions -transfer=binary" "exit"
pause