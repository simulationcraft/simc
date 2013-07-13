#!/bin/sh
# https://gist.github.com/lasconic/5965542
BIN_FILE=SimulationCraft.app/Contents/MacOS/SimulationCraft
for P in `otool -L $BIN_FILE | awk '{print $1}'` 
do 
  if [[ "$P" == *//* ]] 
  then 
    PSLASH=$(echo $P | sed 's,//,/,g')
    install_name_tool -change $P $PSLASH $BIN_FILE
  fi 
done 

QTDIR=$1
for F in `find $QTDIR/lib $QTDIR/plugins $QTDIR/qml  -perm 755 -type f` 
do 
  for P in `otool -L $F | awk '{print $1}'`
  do   
    if [[ "$P" == *//* ]] 
    then 
      PSLASH=$(echo $P | sed 's,//,/,g')
      install_name_tool -change $P $PSLASH $F
    fi 
  done
done
