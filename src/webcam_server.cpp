
#include <iostream>     // cout
#include <stdexcept>    // exceptions
#include <string>       // strings

#include "Log.h"
#include "WebcamServer.h"

#include "webcam_stream_common.h"

using namespace std;

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

		WebcamServer server;
		server.start(port);

		return 0;

	}
	catch (runtime_error e)
	{
		cerr << "!! Caught exception:" << endl
		     << "!! " << e.what() << endl;
		return 1;
	}
}

