all: magent

magent: Command.o Connection.o Decayer.o KernelTcp.o SensorList.o Time.o log.o main.o
	g++ -I. -g -Wall Command.o Connection.o Decayer.o KernelTcp.o SensorList.o Time.o log.o main.o -lm -lpcap -o magent

Command.o: Command.cc lib.h Command.h Sensor.h Connection.h ConnectionModel.h TrafficModel.h
	g++ -I. -g -Wall -c Command.cc 
Connection.o: Connection.cc lib.h log.h Connection.h Time.h ConnectionModel.h TrafficModel.h Sensor.h
	g++ -I. -g -Wall -c Connection.cc
Decayer.o: Decayer.cc lib.h Decayer.h
	g++ -I. -g -Wall -c Decayer.cc
KernelTcp.o: KernelTcp.cc lib.h log.h KernelTcp.h
	g++ -I. -g -Wall -c KernelTcp.cc
SensorList.o: SensorList.cc lib.h log.h SensorList.h Sensor.h Command.h
	g++ -I. -g -Wall -c SensorList.cc
Time.o: Time.cc lib.h Time.h
	g++ -I. -g -Wall -c Time.cc
log.o: log.cc lib.h log.h
	g++ -I. -g -Wall -c log.cc
main.o: main.cc lib.h log.h CommandInput.h CommandOutput.h Command.h Time.h Connection.h Sensor.h TrafficModel.h KernelTcp.h
	g++ -I. -g -Wall -c main.cc

clean:
	rm *.o magent
