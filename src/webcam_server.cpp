
#include <iostream>     // cout
#include <pthread.h>    // multithreading
#include <memory>       // shared_ptr
#include <stdexcept>    // exceptions
#include <sstream>      // stringstream (used by Log.h)
#include <string>       // strings

#include "webcam_stream_common.h"
#include "Log.h"
#include "Sockets.h"
#include "Webcam.h"

using namespace std;

const in_port_t DEFAULT_PORT = 32123;

class WebcamServer: public Server
{
	shared_ptr<Webcam> webcam;

  protected:
	virtual void
	handleMessage (uint32_t messageType, uint32_t length, void* buffer)
	{
		string text;
		struct image_info imageInfo;
		vector<uint32_t> dimensions;

		switch (messageType)
		{
			case MSG_TYPE_QUERY_IMAGE_INFO:
				dimensions = webcam->getDimensions();
				image_info.width = dimensions[0];
				image_info.height = dimensions[1];
				image_info.fmt = getImageFormat();

				sendMessage(MSG_TYPE_IMAGE_INFO_RESPONSE, sizeof(imageInfo), &imageInfo);

				break;

			case MSG_TYPE_START_STREAM:
				break;

			case MSG_TYPE_PAUSE_STREAM:
				break;

			default:
				cerr << "Server: Received unhandled message type " << messageType << endl;
				break;
		}
	}

  public:
	WebcamServer(int port, string devicename) :
		Server(port)
	{
		webcam = shared_ptr<Webcam>(new Webcam(devicename));
	}
};

void
usage (char* basename)
{
	cout << "Usage: " << basename << " [port]" << endl;
}

int
main (int argc, char* args[]) {

	try {
		int port;

		if (argc >= 3) {
			istringstream iss(args[2]);
			iss >> port;
			if (port == 0) {
				cerr << "Bad port number: " << args[2];
			}
		} else {
			port = DEFAULT_PORT;
		}

		shared_ptr<EchoServer> server(new EchoServer(port));

		// Wait for manual termination
		cout << "Press enter to end." << endl;
		getchar();

		return 0;

	}
	catch (runtime_error e)
	{
		cerr << "!! Caught exception:" << endl
		     << "!! " << e.what() << endl;
		return 1;
	}
}


