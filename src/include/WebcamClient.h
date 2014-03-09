



#include "webcam_stream_common.h"
#include "Log.h"
#include "Sockets.h"
#include "Thread.h"
#include "WebcamViewer.h"

using namespace std;

///// WebcamClientConnection /////

	WebcamClient::WebcamClient (int fd, in_addr_t remoteAddress, in_port_t remotePort):
		Connection (fd, remoteAddress, remotePort)
	{
		TRACE_ENTER;

		// Extra initialization: zero out the structs
		memset(&viewerMutex, 0, sizeof(viewerMutex));

		TRACE("Creating webcam viewer mutex...");
		int err = pthread_mutex_init(&webcamMutex, NULL);
		if (err) {
			THROW_ERROR("Error creating webcam viewer mutex: " << strerror(err));
		}

		addDefaultMessageHandler([this] (message_t type, message_len_t length, void* buffer)
		{
			MESSAGE("Received invalid message: " << webcamSocketMsgToString(type));
			sendMessage(ERROR_MSG_INVALID_MSG, sizeof(type), &type);
		});

		//   Add message handlers, which have conveniently similar names to
		// the messages they handle.
		//   Also, the syntax for this is kinda messed up. The handlers are
		// auto-casted to function wrappers without a hitch, but since we're
		// passing member functions, the wrapper functions take the object-
		// in-question (i.e. `this`) as their first parameter. Since handlers
		// shouldn't need a `this` parameter, we bind() it out.
		#define AUTO_ADD_HANDLER(msg_type)                      \
			auto tmp_handler_##msg_type =                       \
				bind(                                           \
					&WebcamServerConnection::handle_##msg_type, \
					this,                                       \
					std::placeholders::_1,                      \
					std::placeholders::_2,                      \
					std::placeholders::_3                       \
				);                                              \
			addMessageHandler(                                  \
				msg_type,                                       \
				tmp_handler_##msg_type                          \
			)

			AUTO_ADD_HANDLER ( ERROR_MSG_INVALID_MSG            ); // DONE
			AUTO_ADD_HANDLER ( ERROR_MSG_TERMINATING_CONNECTION ); // DONE

			AUTO_ADD_HANDLER ( CLIENT_MSG_CLOSE_WEBCAM          ); // DONE
			AUTO_ADD_HANDLER ( CLIENT_MSG_GET_CURRENT_SPEC      ); // DONE
			AUTO_ADD_HANDLER ( CLIENT_MSG_GET_STREAM_STATUS     ); // DONE
			AUTO_ADD_HANDLER ( CLIENT_MSG_GET_SUPPORTED_SPECS   ); // DONE
			AUTO_ADD_HANDLER ( CLIENT_MSG_GET_WEBCAM_STATUS     ); // DONE
			AUTO_ADD_HANDLER ( CLIENT_MSG_GET_WEBCAM_LIST       ); //      TODO
			AUTO_ADD_HANDLER ( CLIENT_MSG_OPEN_WEBCAM           ); // DONE
			AUTO_ADD_HANDLER ( CLIENT_MSG_STOP_STREAM           ); // DONE
			AUTO_ADD_HANDLER ( CLIENT_MSG_SET_CURRENT_SPEC      ); // DONE
			AUTO_ADD_HANDLER ( CLIENT_MSG_START_STREAM          ); // DONE

		#undef AUTO_ADD_HANDLER

		TRACE_EXIT;
	}

	WebcamClient::~WebcamClient ()
	{

	}

/*
	Steps to getting a stream up and running:
	1. CLIENT_MSG_GET_WEBCAM_LIST
	1# That isn't possible yet; just ask the user for a device name
	                                                |
	                                                V
	2. SERVER_MSG_WEBCAM_LIST -> (prompt user) -> CLIENT_MSG_OPEN_WEBCAM
		error: SERVER_ERR_WEBCAM_UNAVAILABLE
		       SERVER_ERR_RUNTIME_ERROR
	3. SERVER_MSG_WEBCAM_IS_OPENED -> CLIENT_MSG_GET_SUPPORTED_SPECS
		error: SERVER_ERR_NO_WEBCAM_OPENED
		       SERVER_ERR_RUNTIME_ERROR
	4. SERVER_MSG_SUPPORTED_SPECS  -> (prompt user) -> CLIENT_MSG_SET_CURRENT_SPEC
		error: SERVER_ERR_INVALID_SPEC
		       SERVER_ERR_NO_WEBCAM_OPENED
		       SERVER_ERR_RUNTIME_ERROR
	5. SERVER_MSG_IMAGE_SPEC -> (update viewer) -> CLIENT_MSG_START_STREAM

	Messages to watch out for:
	-- SERVER_MSG_FRAME -> (send to viewer)
	-- SERVER_MSG_STREAM_IS_STOPPED -> (inform user)
	-- SERVER_MSG_WEBCAM_IS_CLOSED -> (inform user, possibly close/blank viewer)

	-- ERROR_MSG_TERMINATING_CONNECTION -> (inform user, bring the viewer down)
	-- ERROR_MSG_INVALID_MSG -> (inform user)

	Other events:
	-- (SDL wants to close) -> CLIENT_MSG_CLOSE_WEBCAM, ERROR_MSG_TERMINATING_CONNECTION
*/


///// WebcamClient /////

	shared_ptr<Connection>
	WebcamClient::newConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort)
	{
		TRACE_ENTER;
		return shared_ptr<Connection>(new WebcamClientConnection(fd, remoteAddress, remotePort));
		TRACE_EXIT;
	}

