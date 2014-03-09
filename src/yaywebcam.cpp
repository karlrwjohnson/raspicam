
#include <cstdio>      // printf
#include <cstdlib>
#include <cstring>
#include <iostream>    // cout
#include <memory>      // shared_ptr()
#include <stdexcept>   // runtime_exception
#include <sstream>     // stringstream
#include <string>      // strings
#include <vector>      // vectors

#include "Webcam.h"
#include "WebcamViewer.h"

using namespace std;

const string DEFAULT_CAMERA = "/dev/video0";

int
main (int argc, char* args[]) {
	try
	{
		shared_ptr<Webcam> webcam;
		shared_ptr<WebcamViewer> viewer;

		string filename;
		if (argc < 2) {
			filename = DEFAULT_CAMERA;
		} else {
			filename = args[1];
		}

		webcam = shared_ptr<Webcam>(new Webcam(filename));

		Webcam::resolution_t res = webcam->getResolution();

		webcam->startCapture();

		viewer = shared_ptr<WebcamViewer>(new WebcamViewer(
			res.first,
			res.second,
			"Yay, Webcam! - " + filename,
			webcam->getImageFormat()
		));

		// For FPS calculation
		timeval then, now;

		// Set the FPS timer
		gettimeofday(&then, NULL);

		while(1)
		{
			shared_ptr<MappedBuffer> frame = webcam->getFrame();
			viewer->showFrame(frame->data, frame->length);

			gettimeofday(&now, NULL);
			int dt = (now.tv_sec - then.tv_sec) * 1000000 + (now.tv_usec-then.tv_usec);
			printf("Framerate: %.2f fps\n", (1000000.0 / dt));
			then = now;

			// Check for termination. (This throws an exception when that happens.)
			viewer->checkEvents();
		}
	}
	catch (QuitEventException e)
	{
		cerr << e.what() << "\nQuitting...";
		return 0;
	}
	catch (runtime_error e)
	{
		cerr << "!! Exception thrown: " << e.what() << "\n";
		return 1;
	}

	return 0;
}

