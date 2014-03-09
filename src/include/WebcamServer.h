
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

#ifndef WEBCAM_SERVER_H
#define WEBCAM_SERVER_H

using namespace std;

class NoWebcamOpenException: public runtime_error
{
  public:
	NoWebcamOpenException (std::string desc) :
		runtime_error (desc)
	{ }
};

class WebcamServerConnection: public Connection
{
  private:
	shared_ptr<Webcam> webcam;

	pthread_t streamerThreadHandle;

	pthread_mutex_t webcamMutex;

	/// Tells the streamer thread to stop sending frames and return.
	bool streamIsActiveFlag;


  public:
	WebcamServerConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort);

	~WebcamServerConnection ();

  private:
	void
	streamerThread (void* unused);

	void
	startStream ();

	void
	stopStream();

  // Error handlers

	void
	handle_ERROR_MSG_INVALID_MSG            (message_t type, message_len_t len, void* data);
	void
	handle_ERROR_MSG_TERMINATING_CONNECTION (message_t type, message_len_t len, void* data);

  // Client message handlers

	void
	handle_CLIENT_MSG_CLOSE_WEBCAM          (message_t type, message_len_t len, void* data);
	void
	handle_CLIENT_MSG_GET_CURRENT_SPEC      (message_t type, message_len_t len, void* data);
	void
	handle_CLIENT_MSG_GET_STREAM_STATUS     (message_t type, message_len_t len, void* data);
	void
	handle_CLIENT_MSG_GET_SUPPORTED_SPECS   (message_t type, message_len_t len, void* data);
	void
	handle_CLIENT_MSG_GET_WEBCAM_STATUS     (message_t type, message_len_t len, void* data);
	void
	handle_CLIENT_MSG_GET_WEBCAM_LIST       (message_t type, message_len_t len, void* data);
	void
	handle_CLIENT_MSG_OPEN_WEBCAM           (message_t type, message_len_t len, void* data);
	void
	handle_CLIENT_MSG_STOP_STREAM           (message_t type, message_len_t len, void* data);
	void
	handle_CLIENT_MSG_SET_CURRENT_SPEC      (message_t type, message_len_t len, void* data);
	void
	handle_CLIENT_MSG_START_STREAM          (message_t type, message_len_t len, void* data);

};

class WebcamServer: public Server
{
	shared_ptr<Connection>
	newConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort);
};

#endif // WEBCAM_SERVER_H

