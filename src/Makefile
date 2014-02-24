
CXX = g++
FLAGS = --std=c++0x -g -I ./include/
BINDIR = ../bin

OBJECTS := Sockets.o Webcam.o WebcamViewer.o


.PHONY: clean
clean:
	rm -f ../bin/*
	rm -f *.o

$(OBJECTS): %.o: %.cpp
	$(CXX) $(FLAGS) $(INCLUDES) -c $^ -o $@


yaywebcam: yaywebcam.cpp Webcam.o WebcamViewer.o
	$(CXX) $(FLAGS) $^ -o $(BINDIR)/$@ -lSDL2

headlesswebcam: headlesswebcam.cpp Webcam.o
	$(CXX) $(FLAGS) $^ -o $(BINDIR)/$@

sockets_demo: sockets_demo.cpp Sockets.o
	$(CXX) $(FLAGS) $^ -o $(BINDIR)/$@ -lpthread


