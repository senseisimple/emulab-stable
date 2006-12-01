#!/bin/sh

export EXTRA_LIB_PATH=/home/duerig/metis/metis-4.0
export EXTRA_INCLUDE_PATH=/home/duerig/metis/metis-4.0/Lib/
export CC=g++
export COMPILE_FLAGS=-g
export LINK_FLAGS=-g

${CC} ${COMPILE_FLAGS} -c -o tmp/ipassign.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/ipassign.cc
${CC} ${COMPILE_FLAGS} -c -o tmp/bitmath.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/bitmath.cc
${CC} ${COMPILE_FLAGS} -c -o tmp/ConservativeAssigner.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/ConservativeAssigner.cc
${CC} ${COMPILE_FLAGS} -c -o tmp/HierarchicalAssigner.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/HierarchicalAssigner.cc
${CC} ${COMPILE_FLAGS} -c -o tmp/Framework.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/Framework.cc
${CC} ${COMPILE_FLAGS} -c -o tmp/coprocess.o -I${EXTRA_INCLUDE_PATH} -DROUTECALC=\"bin/routecalc\" src/coprocess.cc
${CC} ${LINK_FLAGS} -o bin/ipassign -L${EXTRA_LIB_PATH}/ tmp/ipassign.o tmp/bitmath.o tmp/ConservativeAssigner.o tmp/HierarchicalAssigner.o tmp/Framework.o tmp/coprocess.o -lmetis -lm

#${CC} -o bin/difference src/difference.cc
#${CC} -o bin/add-x src/add-x.cc

${CC} -o bin/inet2graph src/inet2graph.cc -lm
${CC} -o bin/brite2graph src/brite2graph.cc -lm
${CC} -o bin/top2graph src/top2graph.cc -lm

${CC} -o bin/graph2single-source src/graph2single-source.cc -lm
${CC} -o bin/single-source src/single-source.cc -lm

${CC} -o bin/route2dist src/route2dist.cc -lm
${CC} -o bin/route2lan src/route2lan.cc -lm
