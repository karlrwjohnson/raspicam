#ifndef WEBCAM_CLIENT_H
#define WEBCAM_CLIENT_H

#include <list>
#include <pthread.h>
#include <memory>    // shared_ptr>
#include <string>

#include "Log.h"
#include "Sockets.h"
#include "WebcamViewer.h"

class WebcamClientConnection: public Connection
{
  private:

	std::shared_ptr<WebcamViewer> viewer;

	pthread_mutex_t viewerMutex;

	std::string cameraName;

	/// Temporary: I need somewhere to store the bound handlers that
	/// lives as long as the connection.
	/// I'm planning on refactoring Connection so this isn't necessary.
	std::list<message_handler_t>
	boundHandlers;

  public:
	WebcamClientConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort);

	~WebcamClientConnection ();

	void
	handle_ERROR_MSG_INVALID_MSG            (message_t type, message_len_t length, void* data);
	//void
	//handle_ERROR_MSG_TERMINATING_CONNECTION (message_t type, message_len_t length, void* data);

	void
	handle_SERVER_MSG_FRAME                 (message_t type, message_len_t length, void* data);
	void
	handle_SERVER_MSG_IMAGE_SPEC            (message_t type, message_len_t length, void* data);
	void
	handle_SERVER_MSG_STREAM_IS_STARTED     (message_t type, message_len_t length, void* data);
	void
	handle_SERVER_MSG_STREAM_IS_STOPPED     (message_t type, message_len_t length, void* data);
	//void
	//handle_SERVER_MSG_SUPPORTED_SPECS       (message_t type, message_len_t length, void* data);
	void
	handle_SERVER_MSG_WEBCAM_IS_CLOSED      (message_t type, message_len_t length, void* data);
	void
	handle_SERVER_MSG_WEBCAM_IS_OPENED      (message_t type, message_len_t length, void* data);
	//void
	//handle_SERVER_MSG_WEBCAM_LIST           (message_t type, message_len_t length, void* data);
	//void
	//handle_SERVER_ERR_INVALID_SPEC          (message_t type, message_len_t length, void* data);
	//void
	//handle_SERVER_ERR_NO_WEBCAM_OPENED      (message_t type, message_len_t length, void* data);
	//void
	//handle_SERVER_ERR_RUNTIME_ERROR         (message_t type, message_len_t length, void* data);
	//void
	//handle_SERVER_ERR_WEBCAM_UNAVAILABLE    (message_t type, message_len_t length, void* data);
	void
	handle_generic_SERVER_ERR               (message_t type, message_len_t length, void* data);

};

class WebcamClient: public Client
{
	std::shared_ptr<Connection>
	newConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort);
};

#endif // WEBCAM_CLIENT_H

