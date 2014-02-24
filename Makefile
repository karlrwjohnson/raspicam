
yaywebcam: yaywebcam.cpp Webcam.cpp WebcamViewer.cpp
	g++ yaywebcam.cpp Webcam.cpp WebcamViewer.cpp -o yaywebcam -std=c++0x -lSDL2 -g
	
run: yaywebcam
	./yaywebcam /dev/video1

headless: headlesswebcam.cpp Webcam.cpp
	g++ headlesswebcam.cpp Webcam.cpp -o headlesswebcam -std=c++0x -g
	
run_headless: headlesswebcam
	./headlesswebcam /dev/video1

Sockets: Sockets.cpp
	g++ --std=c++0x -g -c Sockets.cpp

sockets_demo: sockets_demo.cpp Sockets
	g++ --std=c++0x -g -c sockets_demo.cpp
	g++ -o sockets_demo -lpthread sockets_demo.o Sockets.o

