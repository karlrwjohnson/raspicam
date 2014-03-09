
CXX = g++
FLAGS = --std=c++0x -g -I ./include/
BINDIR = ../bin

OBJECTS := Sockets.o Webcam.o WebcamViewer.o WebcamServer.o


.PHONY: clean
clean:
	rm -f ../bin/*
	rm -f *.o

$(OBJECTS): %.o: %.cpp
	######################################################################
	$(CXX) $(FLAGS) $(INCLUDES) -c $^ -o $@


yaywebcam: yaywebcam.cpp Webcam.o WebcamViewer.o
	$(CXX) $(FLAGS) $^ -o $(BINDIR)/$@ -lSDL2

headlesswebcam: headlesswebcam.cpp Webcam.o
	$(CXX) $(FLAGS) $^ -o $(BINDIR)/$@

webcaminfo: webcaminfo.cpp Webcam.o
	$(CXX) $(FLAGS) $^ -o $(BINDIR)/$@

sockets_demo: sockets_demo.cpp Sockets.o
	$(CXX) $(FLAGS) $^ -o $(BINDIR)/$@ -lpthread

thread_demo: thread_demo.cpp
	######################################################################
	$(CXX) $(FLAGS) $^ -o $(BINDIR)/$@ -lpthread

chat_server: chat_server.cpp Sockets.cpp
	######################################################################
	$(CXX) $(FLAGS) $^ -o $(BINDIR)/$@ -lpthread

chat_client: chat_client.cpp Sockets.cpp
	######################################################################
	$(CXX) $(FLAGS) $^ -o $(BINDIR)/$@ -lpthread


