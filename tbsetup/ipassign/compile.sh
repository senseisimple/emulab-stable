#!/bin/sh

export EXTRA_LIB_PATH=/home/duerig/metis/metis-4.0
export EXTRA_INCLUDE_PATH=/home/duerig/metis/metis-4.0/Lib/
export COMPILE_FLAGS=-g
export LINK_FLAGS=-g

g++ ${COMPILE_FLAGS} -c -o tmp/ipassign.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/ipassign.cc
g++ ${COMPILE_FLAGS} -c -o tmp/bitmath.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/bitmath.cc
g++ ${COMPILE_FLAGS} -c -o tmp/Assigner.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/Assigner.cc
g++ ${COMPILE_FLAGS} -c -o tmp/ConservativeAssigner.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/ConservativeAssigner.cc
g++ ${COMPILE_FLAGS} -c -o tmp/HierarchicalAssigner.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/HierarchicalAssigner.cc
g++ ${COMPILE_FLAGS} -c -o tmp/Framework.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/Framework.cc
#g++ ${COMPILE_FLAGS} -c -o tmp/HostRouter.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/HostRouter.cc
g++ ${COMPILE_FLAGS} -c -o tmp/Router.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/Router.cc
#g++ ${COMPILE_FLAGS} -c -o tmp/LanRouter.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/LanRouter.cc
g++ ${COMPILE_FLAGS} -c -o tmp/NetRouter.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/NetRouter.cc
g++ ${COMPILE_FLAGS} -c -o tmp/coprocess.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/coprocess.cc
g++ ${LINK_FLAGS} -o bin/ipassign -L${EXTRA_LIB_PATH}/ tmp/ipassign.o tmp/bitmath.o tmp/Assigner.o tmp/ConservativeAssigner.o tmp/HierarchicalAssigner.o tmp/Framework.o  tmp/Router.o tmp/NetRouter.o tmp/coprocess.o -lmetis -lm

#tmp/LanRouter.o 
#tmp/HostRouter.o

g++ -O3 -c -o tmp/routestat.o -I${EXTRA_INCLUDE_PATH} src/routestat.cc
g++ -O3 -o bin/routestat -L${EXTRA_LIB_PATH} tmp/routestat.o tmp/bitmath.o -lmetis -lm

g++ -O3 -o bin/routecalc src/routecalc.cc

g++ -o bin/difference src/difference.cc
g++ -o bin/add-x src/add-x.cc

g++ -o bin/inet2graph src/inet2graph.cc -lm
g++ -o bin/brite2graph src/brite2graph.cc -lm
g++ -o bin/top2graph src/top2graph.cc -lm

