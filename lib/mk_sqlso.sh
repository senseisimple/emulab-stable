#!/bin/csh -fv
###
# Compiles and installs sql.so
###
#webcopy http://www.binevolve.com/~tdarugar/tcl-sql/download/tcl-sql-20000114.tgz  
tar xvfz tcl-sql-20000114.tgz
cd tcl-sql
pwd
chmod u+w Makefile
ed Makefile < ../tcl-sql_patch.ed
gmake
mv sql.so ..
cd ..
rm -rf tcl-sql W.log
