#include "Log.h"
#include "Sockets.h"
#include "WebcamClient.h"
#include "WebcamViewer.h"

#include "webcam_stream_common.h"

using namespace std;

///// WebcamClientConnection /////

	WebcamClientConnection::WebcamClientConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort):
		Connection (fd, remoteAddress, remotePort)
	{
		TRACE_ENTER;

		// Extra initialization: zero out the structs
		memset(&viewerMutex, 0, sizeof(viewerMutex));

		TRACE("Creating webcam viewer mutex...");
		int err = pthread_mutex_init(&viewerMutex, NULL);
		if (err) {
			THROW_ERROR("Error creating webcam viewer mutex: " << strerror(err));
		}

		boundHandlers.push_back(
			[this] (message_t type, message_len_t length, void* buffer)
			{
				TRACE_ENTER;
				MESSAGE("Received invalid message: " << webcamSocketMsgToString(type));
				sendMessage(ERROR_MSG_INVALID_MSG, sizeof(type), &type);
				TRACE_EXIT;
			}
		);
		addDefaultMessageHandler(boundHandlers.back());

		//   Add message handlers, which have conveniently similar names to
		// the messages they handle.
		//   Also, the syntax for this is kinda messed up. The handlers are
		// auto-casted to function wrappers without a hitch, but since we're
		// passing member functions, the wrapper functions take the object-
		// in-question (i.e. `this`) as their first parameter. Since handlers
		// shouldn't need a `this` parameter, we bind() it out.
		#define AUTO_ADD_HANDLER(msg_type)                      \
			boundHandlers.push_back(                            \
				bind(                                           \
					&WebcamClientConnection::handle_##msg_type, \
					this,                                       \
					std::placeholders::_1,                      \
					std::placeholders::_2,                      \
					std::placeholders::_3                       \
				)                                               \
			);                                                  \
			addMessageHandler(                                  \
				msg_type,                                       \
				boundHandlers.back()                            \
			)

			AUTO_ADD_HANDLER ( SERVER_MSG_FRAME                 );
			AUTO_ADD_HANDLER ( SERVER_MSG_IMAGE_SPEC            );
			AUTO_ADD_HANDLER ( SERVER_MSG_STREAM_IS_STARTED     );
			AUTO_ADD_HANDLER ( SERVER_MSG_STREAM_IS_STARTED     );
			AUTO_ADD_HANDLER ( SERVER_MSG_WEBCAM_IS_CLOSED      );
			AUTO_ADD_HANDLER ( SERVER_MSG_WEBCAM_IS_OPENED      );

		#undef AUTO_ADD_HANDLER

		boundHandlers.push_back(
			bind(
				&WebcamClientConnection::handle_generic_SERVER_ERR,
				this,
				std::placeholders::_1,
				std::placeholders::_2,
				std::placeholders::_3
			)
		);

		#define AUTO_ADD_HANDLER(msg_type)                      \
			addMessageHandler(                                  \
				msg_type,                                       \
				boundHandlers.back()                            \
			)

			AUTO_ADD_HANDLER ( SERVER_ERR_INVALID_SPEC          );
			AUTO_ADD_HANDLER ( SERVER_ERR_NO_WEBCAM_OPENED      );
			AUTO_ADD_HANDLER ( SERVER_ERR_RUNTIME_ERROR         );
			AUTO_ADD_HANDLER ( SERVER_ERR_WEBCAM_UNAVAILABLE    );
			AUTO_ADD_HANDLER ( ERROR_MSG_INVALID_MSG            );
		#undef AUTO_ADD_HANDLER

		TRACE_EXIT;
	}

	WebcamClientConnection::~WebcamClientConnection ()
	{
		pthread_mutex_destroy(&viewerMutex);
	}

	void
	WebcamClientConnection::handle_SERVER_MSG_FRAME
		(message_t type, message_len_t length, void* data)
	{
		TRACE_ENTER;

		try
		{
			if (viewer)
			{
				viewer->showFrame(data, length);
			}
			else
			{
				MESSAGE("Dropping frame because the viewer isn't initialized");
			}
		}
		catch (runtime_error e)
		{
			ERROR(e.what());
		}

		TRACE_EXIT;
	}

	void
	WebcamClientConnection::handle_SERVER_MSG_IMAGE_SPEC
		(message_t type, message_len_t length, void* data)
	{
		TRACE_ENTER;

		try
		{
			// Data validation
			if (length != sizeof(struct image_spec)) {
				THROW_ERROR("Unexpected data chunk size from server. Expected "
				         << sizeof(struct image_spec) << " bytes, but received "
				         << length << " bytes.");
			}

			// Cast data chunk to struct
			struct image_spec spec = *reinterpret_cast<struct image_spec*>(data);

			MESSAGE("Image format set to " << Webcam::fmt2string << ", "
			     << spec.width << "x" << spec.height << "px");

			if (!viewer) {
				// Initialize viewer if it hasn't been done already.
				// (It isn't initialized when the webcam is opened -- opening
				// automatically asks what the specification is, which calls
				// this function.)
				viewer = shared_ptr<WebcamViewer>(new WebcamViewer(
					spec.width, spec.height, "Viewing: " + cameraName, spec.fmt
				));
			}
			else
			{
				// Update already-open viewer
				viewer->setImageFormat(spec.fmt);
				viewer->setImageSize(spec.width, spec.height);
			}
		}
		catch (runtime_error e)
		{
			ERROR(e.what());
		}

		TRACE_EXIT;
	}

	void
	WebcamClientConnection::handle_SERVER_MSG_STREAM_IS_STARTED
		(message_t type, message_len_t length, void* data)
	{
		TRACE_ENTER;
		MESSAGE("Server has started streaming.");
	}

	void
	WebcamClientConnection::handle_SERVER_MSG_STREAM_IS_STOPPED
		(message_t type, message_len_t length, void* data)
	{
		TRACE_ENTER;
		MESSAGE("Server has stopped streaming.");
	}

	void
	WebcamClientConnection::handle_SERVER_MSG_WEBCAM_IS_CLOSED
		(message_t type, message_len_t length, void* data)
	{
		TRACE_ENTER;
		// Close viewer via garbage collection
		MESSAGE("Server has closed the webcam.");
		viewer = shared_ptr<WebcamViewer>();
		TRACE_EXIT;
	}

	void
	WebcamClientConnection::handle_SERVER_MSG_WEBCAM_IS_OPENED
		(message_t type, message_len_t length, void* data)
	{
		TRACE_ENTER;
		// We can't initialize the viewer yet because we don't know the image
		// format or dimensions. Get them from the server.
		sendMessage(CLIENT_MSG_GET_CURRENT_SPEC);
		TRACE_EXIT;
	}

	void
	WebcamClientConnection::handle_generic_SERVER_ERR
		(message_t type, message_len_t length, void* data)
	{
		TRACE_ENTER;
		ERROR("Server error: " << string(reinterpret_cast<char*>(data), length));
		TRACE_EXIT;
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


	Automatic handler actions:

	-- SERVER_MSG_FRAME -> (send to viewer if open)
	-- SERVER_MSG_IMAGE_SPEC -> (update viewer)

*/

	/*void
	startStreamMacro ()
	{
		//sendMessage(CLIENT_MSG_GET_WEBCAM_LIST);

		sendMessage(CLIENT_MSG_OPEN_WEBCAM, "/dev/video0");
		addOneTimeHandler(SERVER_MSG_WEBCAM_OPENED,
			[this] (message_t type, message_len_t length, void* data)
		{
			sendMessage(CLIENT_MSG_GET_SUPPORTED_SPECS);
			addOneTimeHandler(SERVER_MSG_SUPPORTED_SPECS,
				[this] (message_t type, message_len_t length, void* data)
			{
				struct image_spec *availableSpecs = reinterpret_cast<struct image_spec*>(data);
				message_len_t numSpecs = length / sizeof(struct image_spec);
				if (length % sizeof(struct image_spec) != 0)
				{
					ERROR("length not a multiple of expected struct size.");
					return;
				}
				else if (length < 1)
				{
					ERROR("no supported image formats found :(");
				}

				sendMessage(CLIENT_MSG_SET_CURRENT_SPEC);
				addOneTimeMessageHandler(SERVER_MSG_IMAGE_SPEC,
					[this] (message_t type, message_len_t length, void* data)
				{
					// Another handler ought to take care of updating the viewer
				});
			});
		});

	}*/

///// WebcamClient /////

	shared_ptr<Connection>
	WebcamClient::newConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort)
	{
		TRACE_ENTER;
		return shared_ptr<Connection>(new WebcamClientConnection(fd, remoteAddress, remotePort));
		TRACE_EXIT;
	}

