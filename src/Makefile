
CXX = g++
FLAGS = --std=c++0x -g -I ./include/
BINDIR = ../bin

OBJECTS := Sockets.o Webcam.o WebcamViewer.o WebcamServer.o WebcamClient.o


.PHONY: clean
clean:
	rm -f ../bin/*
	rm -f *.o

$(OBJECTS): %.o: %.cpp include/%.h
	######################################################################
	$(CXX) $(FLAGS) $(INCLUDES) -c $< -o $@


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

chat_server: chat_server.cpp Sockets.o
	######################################################################
	$(CXX) $(FLAGS) $^ -o $(BINDIR)/$@ -lpthread

chat_client: chat_client.cpp Sockets.o
	######################################################################
	$(CXX) $(FLAGS) $^ -o $(BINDIR)/$@ -lpthread

webcam_server: webcam_server.cpp Sockets.o Webcam.o WebcamServer.o
	######################################################################
	$(CXX) $(FLAGS) $^ -o $(BINDIR)/$@ -lpthread

webcam_client: webcam_client.cpp Sockets.o WebcamClient.o WebcamViewer.o
	######################################################################
	$(CXX) $(FLAGS) $^ -o $(BINDIR)/$@ -lpthread -lSDL2


