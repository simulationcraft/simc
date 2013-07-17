#!/bin/sh
# Adapted from https://gist.github.com/lasconic/5965542
QTDIR=$1
for F in `find $QTDIR/lib $QTDIR/plugins $QTDIR/qml -perm 755 -type f` 
do 
  for P in `otool -L $F | grep -E -e "[^:]$" | awk '{print $1}'`
  do   
    if [[ "$P" == *//* ]] 
    then 
      PSLASH=$(echo $P | sed 's,//,/,g')
      install_name_tool -change $P $PSLASH $F
      install_name_tool -id $PSLASH $F
    fi 
  done
done
