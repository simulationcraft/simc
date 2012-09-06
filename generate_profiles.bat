
setlocal

set SIMC=%~dp0\simc.exe
set BASEDIR=%~dp0


cd %BASEDIR%\profiles\mop_test
%SIMC% generate_priests.simc
%SIMC% generate_priests.simc
cd %BASEDIR%\profiles\PreRaid
%SIMC% generate_PreRaid.simc
cd %BASEDIR%\profiles\RaidDummy
%SIMC% generate_dummy.simc
cd %BASEDIR%\profiles\Tier13H
%SIMC% generate_profiles_T13H.simc
cd %BASEDIR%\profiles\Tier14N
%SIMC% generate_T14N.simc
cd %BASEDIR%\profiles\Tier14H
%SIMC% generate_T14H.simc

pause