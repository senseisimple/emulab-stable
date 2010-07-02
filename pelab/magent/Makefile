#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#CC=g++ -g
CC=g++

OBJECTS=CircularTraffic.o \
	Command.o \
	Connection.o \
	Decayer.o \
	DelaySensor.o \
	DirectInput.o \
	EwmaThroughputSensor.o \
	KernelTcp.o \
	MaxDelaySensor.o \
	MinDelaySensor.o \
	PacketSensor.o \
	Sensor.o \
	SensorList.o \
	StateSensor.o \
	ThroughputSensor.o \
	Time.o \
	TrivialCommandOutput.o \
	log.o \
	main.o \
	saveload.o \
	lib.o \
	TSThroughputSensor.o \
	LeastSquaresThroughput.o \
	UdpPacketSensor.o \
	UdpMinDelaySensor.o \
	UdpMaxDelaySensor.o \
	UdpThroughputSensor.o

all: magent

magent: ${OBJECTS}
	${CC} -I. -Wall $+ -lm -lpcap -o magent

UdpThroughputSensor.o: UdpThroughputSensor.cc UdpThroughputSensor.h UdpPacketSensor.h Sensor.h lib.h
	${CC} -I. -Wall -c $<

UdpMaxDelaySensor.o: UdpMaxDelaySensor.cc UdpMaxDelaySensor.h UdpPacketSensor.h UdpMinDelaySensor.h Sensor.h lib.h
	${CC} -I. -Wall -c $<

UdpMinDelaySensor.o: UdpMinDelaySensor.cc UdpMinDelaySensor.h UdpPacketSensor.h lib.h
	${CC} -I. -Wall -c $<

UdpPacketSensor.o: UdpPacketSensor.cc UdpPacketSensor.h lib.h
	${CC} -I. -Wall -c $<

TSThroughputSensor.o: TSThroughputSensor.cc TSThroughputSensor.h lib.h Sensor.h ThroughputSensor.h PacketSensor.h StateSensor.h
	${CC} -I. -Wall -c TSThroughputSensor.cc

LeastSquaresThroughput.o: LeastSquaresThroughput.cc LeastSquaresThroughput.h Sensor.h TSThroughputSensor.h DelaySensor.h lib.h CommandOutput.h 
	${CC} -I. -Wall -c LeastSquaresThroughput.cc

CircularTraffic.o: CircularTraffic.cc lib.h log.h TrafficModel.h CircularTraffic.h Command.h ConnectionModel.h
	${CC} -I. -Wall -c CircularTraffic.cc

Command.o: Command.cc lib.h Command.h Sensor.h Connection.h ConnectionModel.h TrafficModel.h CircularTraffic.h
	${CC} -I. -Wall -c Command.cc 

Connection.o: Connection.cc lib.h log.h Connection.h Time.h ConnectionModel.h TrafficModel.h Sensor.h
	${CC} -I. -Wall -c Connection.cc

Decayer.o: Decayer.cc lib.h Decayer.h
	${CC} -I. -Wall -c Decayer.cc

DelaySensor.o: DelaySensor.cc lib.h Sensor.h DelaySensor.h PacketSensor.h Time.h StateSensor.h
	${CC} -I. -Wall -c DelaySensor.cc

DirectInput.o: DirectInput.cc lib.h log.h CommandInput.h saveload.h DirectInput.h
	${CC} -I. -Wall -c DirectInput.cc

EwmaThroughputSensor.o: EwmaThroughputSensor.cc lib.h Sensor.h EwmaThroughputSensor.h ThroughputSensor.h CommandOutput.h
	${CC} -I. -Wall -c EwmaThroughputSensor.cc

KernelTcp.o: KernelTcp.cc lib.h log.h KernelTcp.h Command.h
	${CC} -I. -Wall -c KernelTcp.cc

MaxDelaySensor.o: MaxDelaySensor.cc lib.h Sensor.h Decayer.h MaxDelaySensor.h DelaySensor.h log.h saveload.h CommandOutput.h StateSensor.h
	${CC} -I. -Wall -c MaxDelaySensor.cc

MinDelaySensor.o: MinDelaySensor.cc lib.h Sensor.h Decayer.h MinDelaySensor.h DelaySensor.h log.h saveload.h CommandOutput.h
	${CC} -I. -Wall -c MinDelaySensor.cc

PacketSensor.o: PacketSensor.cc lib.h Sensor.h PacketSensor.h StateSensor.h
	${CC} -I. -Wall -c PacketSensor.cc

Sensor.o: Sensor.cc lib.h Sensor.h
	${CC} -I. -Wall -c Sensor.cc

SensorList.o: SensorList.cc lib.h log.h SensorList.h Sensor.h Command.h PacketSensor.h DelaySensor.h MinDelaySensor.h MaxDelaySensor.h ThroughputSensor.h StateSensor.h EwmaThroughputSensor.h
	${CC} -I. -Wall -c SensorList.cc

StateSensor.o: StateSensor.cc lib.h Sensor.h StateSensor.h
	${CC} -I. -Wall -c StateSensor.cc

ThroughputSensor.o: ThroughputSensor.cc lib.h Sensor.h ThroughputSensor.h PacketSensor.h StateSensor.h
	${CC} -I. -Wall -c ThroughputSensor.cc

Time.o: Time.cc lib.h Time.h
	${CC} -I. -Wall -c Time.cc

TrivialCommandOutput.o: TrivialCommandOutput.cc lib.h log.h saveload.h CommandOutput.h TrivialCommandOutput.h CommandInput.h
	${CC} -I. -Wall -c TrivialCommandOutput.cc

log.o: log.cc lib.h log.h
	${CC} -I. -Wall -c log.cc

main.o: main.cc lib.h log.h CommandInput.h CommandOutput.h Command.h Time.h Connection.h Sensor.h TrafficModel.h KernelTcp.h saveload.h DirectInput.h TrivialCommandOutput.h
	${CC} -I. -Wall -c main.cc

saveload.o: saveload.cc lib.h log.h saveload.h Command.h
	${CC} -I. -Wall -c saveload.cc

lib.o: lib.cc lib.h
	${CC} -I. -Wall -c lib.cc

clean:
	rm *.o magent
