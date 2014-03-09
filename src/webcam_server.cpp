
#include <iostream>     // cout
#include <pthread.h>    // multithreading
#include <memory>       // shared_ptr
#include <stdexcept>    // exceptions
#include <sstream>      // stringstream (used by Log.h)
#include <string>       // strings

#include "webcam_stream_common.h"
#include "Log.h"
#include "Sockets.h"
#include "Thread.h"
#include "Webcam.h"

using namespace std;

class WebcamServerConnection: public Connection
{
  private:
	shared_ptr<Webcam> webcam;

	pthread_t streamerThreadHandle;

	pthread_t webcamMutex;

	/// Tells the streamer thread to stop sending frames and return.
	bool streamIsActiveFlag;

	/// Flag so methods know not to throw exceptions during the destructor
	bool nowDestructingFlag;

	void *
	streamerThread ()
	{
		TRACE_ENTER;
		try
		{
			MutexLock lock(webcamMutex);

			while (streamIsActiveFlag)
			{
				lock.relock();
				shared_ptr<MappedBuffer> frame = webcam->getFrame();
				lock.unlock();

				sendMessage(MSG_TYPE_FRAME, frame->length, frame->data);
			}

			return NULL;
		}
		catch (runtime_error e)
		{
			cerr << "!! Caught exception in reader thread:" << endl
			     << "!! " << e.what() << endl;
			return NULL;
		}
	}

	const message_handler_t handleQueryStreamStatus =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		sendMessage(streamIsActiveFlag ?
		            (MSG_TYPE_STREAM_IS_STARTED) :
		            (MSG_TYPE_STREAM_IS_STOPPED) , 0, NULL);
		TRACE_EXIT;
	};

	const message_handler_t handleQueryImageFormat =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;

		struct image_info imageInfo;
		vector<uint32_t> dimensions;

		MutexLock lock(webcamMutex);
		lock.relock();

		dimensions = webcam->getDimensions();
		image_info.fmt = getImageFormat();
		image_info.width = dimensions[0];
		image_info.height = dimensions[1];

		sendMessage(MSG_TYPE_IMAGE_FORMAT_RESPONSE, sizeof(imageInfo), &imageInfo);

		TRACE_EXIT;
	};

	const message_handler_t handle_QUERY_SUPPORTED_FORMATS =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		//TODO
		sendMessage(MSG_TYPE_INTERNAL_ERROR, "This functionality is not yet implemented.");
		TRACE_EXIT;
	};

	const message_handler_t handle_USE_FORMAT =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		//TODO
		sendMessage(MSG_TYPE_INTERNAL_ERROR, "This functionality is not yet implemented.");
		TRACE_EXIT;
	};

	const message_handler_t handle_START_STREAM =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		startStream();
		TRACE_EXIT;
	};

	const message_handler_t handle_PAUSE_STREAM =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		stopStream();
		TRACE_EXIT;
	};

	const message_handler_t handle_ERROR_INTERNAL =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		ERROR("Client reports internal error: " << string(buffer, length));
		TRACE_EXIT;
	};

	const message_handler_t handle_ERROR_INVALID_MSG =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		ERROR("Client reports invalid message.");
		TRACE_EXIT;
	};

	const message_handler_t handle_TERMINATING_CONNECTION =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		MESSAGE("Client is terminating the connection.");
		close();
		TRACE_EXIT;
	};

  public:
	WebcamServerConnection(int fd, in_addr_t remoteAddress, in_port_t remotePort):
		Connection         (fd, remoteAddress, remotePort)
		streamIsActiveFlag (false),
		nowDestructingFlag (false)
	{
		TRACE_ENTER;

		TRACE("Creating webcam mutex...");
		int err = pthread_mutex_init(&webcamMutex, NULL);
		if (err) {
			THROW_ERROR("Error creating webcam mutex: " << strerror(err));
		}

		addDefaultMessageHandler(handleDefault);

		// Laziness.
		#define ADD_HANDLER (type) \
			addMessageHandler(MSG_TYPE_##type, handle_##type);
		ADD_HANDLER(QUERY_STREAM_STATUS);
		ADD_HANDLER(QUERY_IMAGE_FORMAT);
		ADD_HANDLER(QUERY_SUPPORTED_FORMATS);
		ADD_HANDLER(USE_FORMAT);
		ADD_HANDLER(START_STREAM);
		ADD_HANDLER(PAUSE_STREAM);
		ADD_HANDLER(ERROR_INVALID_INTERNAL);
		ADD_HANDLER(ERROR_INVALID_MSG);
		ADD_HANDLER(TERMINATING_CONNECTION);
		#undef ADD_HANDLER

		TRACE_EXIT;
	}

	~WebcamServer()
	{
		nowDestructing = true;
		_stopStream();
	}

	startStream ()
	{
		if (!streamIsActiveFlag)
		{
			MESSAGE("Starting stream");

			// Open webcam
			MutexLock lock(webcamMutex);
			lock.relock();
			webcam = shared_ptr<Webcam>(new Webcam(devicename));
			lock.unlock();

			streamIsActiveFlag = true;

			streamerHandle = pthread_create_using_method<WebcamServerConnection, void*>(
					*this, &WebcamServerConnection::streamerThread, NULL
			);

			sendMessge(MSG_TYPE_STREAM_IS_STARTED, 0, NULL);
		}
		else
		{
			MESSAGE("Stream is already started");
			sendMessge(MSG_TYPE_STREAM_IS_STARTED, 0, NULL);
		}
	}

	stopStream()
	{
		if (streamIsActiveFlag)
		{
			MESSAGE("Stopping stream");

			// Allow the streamer thread to end itself naturally
			streamIsActiveFlag = false;

			// Wait for the streamer thread to stop
			int err = pthread_join(streamerThreadHandle, NULL);
			if (err)
			{
				if (nowDestructingFlag) {
					WARNING("Unable to terminate streamer thread: " << strerror(err));
				} else {
					THROW_ERROR("Unable to terminate streamer thread: " << strerror(err));
				}
			}

			// Release webcam
			MutexLock lock(webcamMutex);
			lock.relock();
			webcam = shared_ptr<Webcam>();
			lock.unlock();

			sendMessge(MSG_TYPE_STREAM_IS_STOPPED, 0, NULL);
		}
		else
		{
			MESSAGE("Stream is already stopped");
			sendMessge(MSG_TYPE_STREAM_IS_STOPPED, 0, NULL);
		}
	}

};

class WebcamServer: public Server
{
	shared_ptr<Connection>
	newConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort)
	{
		TRACE_ENTER;
		return shared_ptr<Connection>(new WebcamServerConnection(fd, remoteAddress, remotePort));
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

