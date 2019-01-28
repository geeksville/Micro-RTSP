
SRCS = ../src/CRtspSession.cpp ../src/CStreamer.cpp ../src/JPEGSamples.cpp ../src/SimStreamer.cpp

run: *.cpp ../src/*
	skill testerver
	g++ -o testserver -I ../src -I . *.cpp $(SRCS)
	./testserver
