#!/bin/csh

perl opsrecv.pl -e tbres/pelabbgmon -d1 >! log.opsrecv &
perl manager.pl >! log.manager &
perl automanagerclient.pl -l600 0.10 >! log.automanagerclient &

