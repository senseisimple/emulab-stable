#!/bin/sh

export EXTRA_LIB_PATH=/users/duerig

#g++ -O3 -c -o tmp/ipassign.o -I${EXTRA_LIB_PATH}/metis/metis-4.0/Lib/ src/ipassign.cc
#g++ -O3 -c -o tmp/bitmath.o -I${EXTRA_LIB_PATH}/metis/metis-4.0/Lib/ src/bitmath.cc
#g++ -O3 -c -o tmp/Assigner.o -I${EXTRA_LIB_PATH}/metis/metis-4.0/Lib/ src/Assigner.cc
#g++ -O3 -c -o tmp/ConservativeAssigner.o -I${EXTRA_LIB_PATH}/metis/metis-4.0/Lib/ src/ConservativeAssigner.cc
#g++ -O3 -c -o tmp/Framework.o -I${EXTRA_LIB_PATH}/metis/metis-4.0/Lib/ src/Framework.cc
#g++ -O3 -c -o tmp/HostRouter.o -I${EXTRA_LIB_PATH}/metis/metis-4.0/Lib/ src/HostRouter.cc
#g++ -O3 -c -o tmp/Router.o -I${EXTRA_LIB_PATH}/metis/metis-4.0/Lib/ src/Router.cc
#g++ -O3 -c -o tmp/LanRouter.o -I${EXTRA_LIB_PATH}/metis/metis-4.0/Lib/ src/LanRouter.cc
#g++ -O3 -c -o tmp/NetRouter.o -I${EXTRA_LIB_PATH}/metis/metis-4.0/Lib/ src/NetRouter.cc
g++ -O3 -c -o tmp/coprocess.o -I${EXTRA_LIB_PATH}/metis/metis-4.0/Lib/ src/coprocess.cc
g++ -O3 -o bin/ipassign -I${EXTRA_LIB_PATH}/metis/metis-4.0/Lib/ -L${EXTRA_LIB_PATH}/metis/metis-4.0 tmp/ipassign.o tmp/bitmath.o tmp/Assigner.o tmp/ConservativeAssigner.o tmp/Framework.o tmp/HostRouter.o tmp/Router.o tmp/LanRouter.o tmp/NetRouter.o tmp/coprocess.o -lmetis -lm

g++ -O3 -c -o tmp/routestat.o -I${EXTRA_LIB_PATH}/metis/metis-4.0/Lib/ src/routestat.cc
g++ -O3 -o bin/routestat -L${EXTRA_LIB_PATH}/metis/metis-4.0 tmp/routestat.o tmp/bitmath.o -lmetis -lm

#g++ -O3 -o bin/routecalc src/routecalc.cc

#g++ -o bin/inet2graph src/inet2graph.cc -lm
#g++ -o bin/brite2graph src/brite2graph.cc -lm

#g++ -o bin/top2graph src/top2graph.cc -lm

#g++ -o bin/autocheck src/autocheck.cc -lm
#g++ -o bin/boolcmp src/boolcmp.cc -lm

#bin/autocheck


#27338.150u 429.446s 12:16:11.56 62.8%   430+-267k 1+2584io 1998215pf+0w
