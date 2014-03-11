#ifndef WEBCAM_SERVER_H
#define WEBCAM_SERVER_H

#include <list>         // for the bound handlers
#include <pthread.h>    // multithreading
#include <memory>       // shared_ptr
#include <stdexcept>    // exceptions

#include "Sockets.h"
#include "Webcam.h"

class NoWebcamOpenException: public std::runtime_error
{
  public:
	NoWebcamOpenException (std::string desc) :
		std::runtime_error (desc)
	{ }
};

class WebcamServerConnection: public Connection
{
  private:
	std::shared_ptr<Webcam> webcam;

	pthread_t streamerThreadHandle;

	pthread_mutex_t webcamMutex;

	/// Tells the streamer thread to stop sending frames and return.
	bool streamIsActiveFlag;

	/// Temporary: I need somewhere to store the bound handlers that
	/// lives as long as the connection.
	/// I'm planning on refactoring Connection so this isn't necessary.
	std::list<message_handler_t>
	boundHandlers;

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
	std::shared_ptr<Connection>
	newConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort);
};

#endif // WEBCAM_SERVER_H

