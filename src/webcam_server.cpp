
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

class WebcamServer: public Server
{
  private:
	shared_ptr<Webcam> webcam;

	pthread_t streamerHandle;

	pthread_t webcamMutex;

	/** Flag to tell the streamer thread to stop sending frames and return. **/
	bool streamerThreadKeepGoing;

	/** Flag so _stopStream() doesn't throw an exception during the destructor. **/
	bool nowDestructing;

	void *
	_streamerThread ()
	{
		TRACE_ENTER;
		try
		{
			while (streamerThreadKeepGoing)
			{
				MutexLock lock(webcamMutex);
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

	void *
	_streamerThreadCaller (void *webcamServerObject)
	{
		TRACE_ENTER;
		void *retval = static_cast<WebcamServer*>(webcamServerObject)->_streamerThread();
		TRACE_EXIT;
		return retval;
	}

  protected:
	virtual void
	handleMessage (uint32_t messageType, uint32_t length, void* buffer)
	{
		string text;
		struct image_info imageInfo;
		vector<uint32_t> dimensions;

		switch (messageType)
		{
			case MSG_TYPE_QUERY_STREAM_STATUS:
				MESSAGE("Message received: MSG_TYPE_QUERY_STREAM_STATUS");

				if (streamerThreadKeepGoing)
					sendMessage(MSG_TYPE_STREAM_IS_STARTED);
				else
					sendMessage(MSG_TYPE_STREAM_IS_STOPPED);

				break;

			case MSG_TYPE_QUERY_IMAGE_INFO:
				MESSAGE("Message received: MSG_TYPE_QUERY_IMAGE_INFO");

				dimensions = webcam->getDimensions();
				image_info.width = dimensions[0];
				image_info.height = dimensions[1];
				image_info.fmt = getImageFormat();

				sendMessage(MSG_TYPE_IMAGE_INFO_RESPONSE, sizeof(imageInfo), &imageInfo);

				break;

			case MSG_TYPE_START_STREAM:
				MESSAGE("Message received: MSG_TYPE_START_STREAM");
				_startStream();
				sendMessage(MSG_TYPE_STREAM_IS_STARTED); 
				break;

			case MSG_TYPE_PAUSE_STREAM:
				MESSAGE("Message received: MSG_TYPE_PAUSE_STREAM");
				_stopStream();
				sendMessage(MSG_TYPE_STREAM_IS_STOPPED);
				break;

			default:
				MESSAGE("Message received: Unhandled message type " << messageType);
				break;
		}
	}

  public:
	WebcamServer(int port, string devicename) :
		Server(port),
		streamerThreadKeepGoing(false),
		destructing(false)
	{
		int err;
		
		TRACE("Creating webcam mutex...");
		if((err = pthread_mutex_init(&webcamMutex, NULL))) {
			THROW_ERROR("Error creating webcam mutex: " << strerror(err));
		}

		webcam = shared_ptr<Webcam>(new Webcam(devicename));
	}

	~WebcamServer()
	{
		nowDestructing = true;
		_stopStream();
	}

	startStream ()
	{
		if (!streamerThreadKeepGoing)
		{
			MESSAGE("Starting stream");

			streamerThreadKeepGoing = true;

			int err = pthread_create(&streamerHandle, NULL, _streamerThreadCaller, this);
			if (err)
			{
				THROW_ERROR("Unable to start streamer thread: " << strerror(err));
			}
		}
		else
		{
			MESSAGE("Stream is already started");
		}
	}

	stopStream()
	{
		if (streamerThreadKeepGoing)
		{
			MESSAGE("Stopping stream");

			// Allow the streamer thread to end itself naturally
			streamerThreadKeepGoing = false;

			int err = pthread_join(streamerHandle, NULL);
			if (err)
			{
				if (nowDestructing) {
					WARNING("Unable to terminate streamer thread: " << strerror(err));
				} else {
					THROW_ERROR("Unable to terminate streamer thread: " << strerror(err));
				}
			}
		}
		else
		{
			MESSAGE("Stream is already stopped");
		}
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

		// Wait for server to terminate
		server->joinReaderThread();

		return 0;

	}
	catch (runtime_error e)
	{
		cerr << "!! Caught exception:" << endl
		     << "!! " << e.what() << endl;
		return 1;
	}
}


