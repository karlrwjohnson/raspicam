



#include "webcam_stream_common.h"
#include "Log.h"
#include "Sockets.h"
#include "Thread.h"
#include "WebcamViewer.h"

using namespace std;

class WebcamClientConnection: public Connection
{
  private:

	shared_ptr<WebcamViewer> viewer;

	pthread_t viewerMutex;

	bool ViewerIsActive

  public
	WebcamClient(int fd, in_addr_t remoteAddress, in_port_t remotePort):
		Connection (fd, remoteAddress, remotePort)
	{
		TRACE_ENTER;

		TRACE("Creating webcam viewer mutex...");
		int err = pthread_mutex_init(&webcamMutex, NULL);
		if (err) {
			THROW_ERROR("Error creating webcam viewer mutex: " << strerror(err));
		}

		addDefaultMessageHandler(handleDefault);

		// Laziness.
		#define ADD_HANDLER (type) \
			addMessageHandler(MSG_TYPE_##type, handle_##type);
		ADD_HANDLER(STREAM_IS_STARTED);
		ADD_HANDLER(STREAM_IS_STOPPED);
		ADD_HANDLER(IMAGE_FORMAT_RESPONSE);
		ADD_HANDLER(SUPPORTED_FORMATS_RESPONSE);
		ADD_HANDLER(FRAME);
		ADD_HANDLER(ERROR_INVALID_INTERNAL);
		ADD_HANDLER(ERROR_INVALID_MSG);
		ADD_HANDLER(TERMINATING_CONNECTION);
		#undef ADD_HANDLER

		TRACE_EXIT;
	}
};

class WebcamClient: public Client
{
	shared_ptr<Connection>
	newConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort)
	{
		TRACE_ENTER;
		return shared_ptr<Connection>(new WebcamClientConnection(fd, remoteAddress, remotePort));
		TRACE_EXIT;
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

		WebcamClient client;
		shared_ptr<Connection> conn = client.connect("127.0.0.1", port);

		string input;
		do {
			getline(cin, input);
			
			if (input == "start") {
				/*??*/
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

