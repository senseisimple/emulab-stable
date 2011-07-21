# .bashrc for root on XP/CygWin under Emulab.


# paths.sh trashes the system path while adding $BINPATH.
OLDPATH=$PATH
export ee=/etc/emulab
source $ee/paths.sh
PATH=$BINDIR":"$OLDPATH

export rc=$BINDIR/rc
tmcc=$BINDIR/tmcc

alias v='ls -lsF'
