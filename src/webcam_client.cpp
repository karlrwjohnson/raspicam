#include <iostream>
#include <memory>

#include "Log.h"
#include "WebcamClient.h"

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
		string address;
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

		if (argc >= 2) {
			address = args[1];
		} else {
			address = "127.0.0.1";
		}

		WebcamClient client;
		shared_ptr<Connection> conn = client.connect(address, port);
		//shared_ptr<WebcamClientConnection> conn =
		//	dynamic_pointer_cast<WebcamClientConnection, Connection>
		//	();

		string input;
		do {
			getline(cin, input);
			
			if (input == "start") {
				conn->sendMessage(CLIENT_MSG_START_STREAM);
			} else if (input == "stop") {
				conn->sendMessage(CLIENT_MSG_STOP_STREAM);
			} else if (input == "open") {
				conn->sendMessage(CLIENT_MSG_OPEN_WEBCAM, "/dev/video0");
			} else if (input == "close") {
				conn->sendMessage(CLIENT_MSG_CLOSE_WEBCAM);
			} else if (input == "getspec") {
				conn->sendMessage(CLIENT_MSG_GET_CURRENT_SPEC);
			} else if (input == "exit") {
				conn->sendMessage(ERROR_MSG_TERMINATING_CONNECTION);
				exit;
			} else {
				MESSAGE("Unknown command " << input);
			}
		} while(1);

		return 0;

	}
	catch (runtime_error e)
	{
		cerr << "!! Caught exception:" << endl
		     << "!! " << e.what() << endl;
		return 1;
	}
}

