:: Used to automate everything for Alex so he can be lazy.

git clean -f -x -d
:: Clean the directory up, otherwise it'll zip up all sorts of stuff.
For /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set mydate=%%a-%%b)
:: Get the date, because I guess that's important.
git log --no-merges -1 --pretty="%%h">bla.txt
:: Gives the git revision
set /p revision=<bla.txt
:: Hacky hack because windows command prompt is annoying.
del bla.txt

set simcversion=603-20
set install=simc-%simcversion%-source
cd>bla.txt
set /p download=<bla.txt
del bla.txt

robocopy . %install% /s *.* /xd .git %install% /xf *.pgd /xn
:: Don't zip up the PGO file, that's a large file.
set filename=%install%.zip
7z a -r -tzip %install%.zip %install% -mx9 -mm=Deflate
call start winscp /command "open downloads" "put %download%\%filename% -nopreservetime -nopermissions -transfer=binary" "exit"
:: Zips source code, calls winscp to upload it. If anyone else is attempting this 'downloads' is the nickname of whatever server you are trying to upload to in winscp.

set install=simc-%simcversion%-win32
call win32_release_msvc12.bat
iscc.exe "setup32.iss"
call start winscp /command "open downloads" "put %download%\SimcSetup-%simcversion%-win32.exe -nopreservetime -nopermissions -transfer=binary" "exit"
7z a -r -tzip %install%.zip %install% -mx9 -mm=Deflate
call start winscp /command "open downloads" "put %download%\%install%.zip -nopreservetime -nopermissions -transfer=binary" "exit"

set install=simc-%simcversion%-win64
call win64_release_msvc12.bat
iscc.exe "setup64.iss"
call start winscp /command "open downloads" "put %download%\SimcSetup-%simcversion%-win64.exe  -nopreservetime -nopermissions -transfer=binary" "exit"
7z a -r -tzip %install%.zip %install% -mx9 -mm=Deflate
winscp /command "open downloads" "put %download%\%install%.zip -nopreservetime -nopermissions -transfer=binary" "exit"
pause