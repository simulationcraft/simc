@echo off

echo -----------------------------------------
echo Initializing environment
echo.

rmdir wiki\wikiFromSVN /s /q

echo.
echo -----------------------------------------
echo Updating from SVN repository
echo.

svn co https://simulationcraft.googlecode.com/svn/wiki/ wiki\wikiFromSVN

echo.
echo -----------------------------------------
echo Applying changes to checkout directory
echo.
cd wiki
For %%x in (*.wiki) do (copy %%x wikiFromSVN\%%x /-Y)

echo.
echo -----------------------------------------
echo Committing
echo.
For %%x in (*.wiki) do (svn add wikiFromSVN\%%x)
echo. > commit.log
svn ci wikiFromSVN --file commit.log

echo.
pause
