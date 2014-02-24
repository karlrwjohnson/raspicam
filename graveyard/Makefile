
yaywebcam: yaywebcam.cpp Webcam.cpp WebcamViewer.cpp
	g++ yaywebcam.cpp Webcam.cpp WebcamViewer.cpp -o yaywebcam -std=c++0x -lSDL2 -g
	
run: yaywebcam
	./yaywebcam /dev/video1

headlesswebcam: headlesswebcam.cpp Webcam.cpp
	g++ headlesswebcam.cpp Webcam.cpp -o headlesswebcam -std=c++0x -g
	
run_headless: headlesswebcam
	./headlesswebcam /dev/video1

#webcam_demo: webcam_demo.cpp WebcamViewer.cpp
#	g++ webcam_demo.cpp WebcamViewer.cpp -o webcam_demo -std=c++0x -lSDL2 -g
	
#run: webcam_demo
#	./webcam_demo /dev/video1

sdl_demo: sdl_demo.cpp
	g++ sdl_demo.cpp -o sdl_demo -std=c++0x -lSDL2 -g

run_sdl_demo: sdl_demo
	./sdl_demo

