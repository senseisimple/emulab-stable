all: magent

magent: CircularTraffic.o Command.o Connection.o Decayer.o DelaySensor.o DirectInput.o KernelTcp.o MaxDelaySensor.o MinDelaySensor.o PacketSensor.o Sensor.o SensorList.o ThroughputSensor.o Time.o TrivialCommandOutput.o log.o main.o saveload.o
	g++ -I. -g -Wall CircularTraffic.o Command.o Connection.o Decayer.o DelaySensor.o DirectInput.o KernelTcp.o MaxDelaySensor.o MinDelaySensor.o PacketSensor.o Sensor.o SensorList.o ThroughputSensor.o Time.o TrivialCommandOutput.o log.o main.o saveload.o -lm -lpcap -o magent

CircularTraffic.o: CircularTraffic.cc lib.h log.h TrafficModel.h CircularTraffic.h Command.h ConnectionModel.h
	g++ -I. -g -Wall -c CircularTraffic.cc

Command.o: Command.cc lib.h Command.h Sensor.h Connection.h ConnectionModel.h TrafficModel.h CircularTraffic.h
	g++ -I. -g -Wall -c Command.cc 

Connection.o: Connection.cc lib.h log.h Connection.h Time.h ConnectionModel.h TrafficModel.h Sensor.h
	g++ -I. -g -Wall -c Connection.cc

Decayer.o: Decayer.cc lib.h Decayer.h
	g++ -I. -g -Wall -c Decayer.cc

DelaySensor.o: DelaySensor.cc lib.h Sensor.h DelaySensor.h PacketSensor.h Time.h
	g++ -I. -g -Wall -c DelaySensor.cc

DirectInput.o: DirectInput.cc lib.h log.h CommandInput.h saveload.h DirectInput.h
	g++ -I. -g -Wall -c DirectInput.cc

KernelTcp.o: KernelTcp.cc lib.h log.h KernelTcp.h Command.h
	g++ -I. -g -Wall -c KernelTcp.cc

MaxDelaySensor.o: MaxDelaySensor.cc lib.h Sensor.h Decayer.h MaxDelaySensor.h DelaySensor.h log.h saveload.h CommandOutput.h
	g++ -I. -g -Wall -c MaxDelaySensor.cc

MinDelaySensor.o: MinDelaySensor.cc lib.h Sensor.h Decayer.h MinDelaySensor.h DelaySensor.h log.h saveload.h CommandOutput.h
	g++ -I. -g -Wall -c MinDelaySensor.cc

PacketSensor.o: PacketSensor.cc lib.h Sensor.h PacketSensor.h
	g++ -I. -g -Wall -c PacketSensor.cc

Sensor.o: Sensor.cc lib.h Sensor.h
	g++ -I. -g -Wall -c Sensor.cc

SensorList.o: SensorList.cc lib.h log.h SensorList.h Sensor.h Command.h PacketSensor.h DelaySensor.h MinDelaySensor.h MaxDelaySensor.h ThroughputSensor.h
	g++ -I. -g -Wall -c SensorList.cc

ThroughputSensor.o: ThroughputSensor.cc lib.h Sensor.h ThroughputSensor.h PacketSensor.h
	g++ -I. -g -Wall -c ThroughputSensor.cc

Time.o: Time.cc lib.h Time.h
	g++ -I. -g -Wall -c Time.cc

TrivialCommandOutput.o: TrivialCommandOutput.cc lib.h log.h saveload.h CommandOutput.h TrivialCommandOutput.h CommandInput.h
	g++ -I. -g -Wall -c TrivialCommandOutput.cc

log.o: log.cc lib.h log.h
	g++ -I. -g -Wall -c log.cc

main.o: main.cc lib.h log.h CommandInput.h CommandOutput.h Command.h Time.h Connection.h Sensor.h TrafficModel.h KernelTcp.h saveload.h
	g++ -I. -g -Wall -c main.cc

saveload.o: saveload.cc lib.h log.h saveload.h Command.h
	g++ -I. -g -Wall -c saveload.cc

clean:
	rm *.o magent
