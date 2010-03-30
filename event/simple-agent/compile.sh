gcc -DELVIN_COMPAT -c ../lib/event.c -o event.o -I../lib/ -I../../lib/libtb/ \
  -I../../.. -L../../../pubsub
gcc -DELVIN_COMPAT -c ../lib/util.c -o util.o -I../lib/ -I../../lib/libtb/ \
  -I../../.. -L../../../pubsub
ar crv libevent.a event.o util.o
ranlib libevent.a
g++ -DELVIN_COMPAT -o simple-agent -Wall -I../lib/ -I../../lib/libtb/ \
  -I../../.. -L../../../pubsub main.cc libevent.a -lpubsub -lssl

