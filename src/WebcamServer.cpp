#include <algorithm>    // for_each
#include <iostream>     // cout
#include <functional>   // bind()
#include <string>       // strings

#include "Log.h"
#include "Thread.h"
#include "WebcamServer.h"
#include "webcam_stream_common.h"

using namespace std;

///// WebcamServerConnection /////

	WebcamServerConnection::WebcamServerConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort):
		Connection         (fd, remoteAddress, remotePort),
		streamIsActiveFlag (false)
	{
		TRACE_ENTER;

		// Extra initialization: zero out the structs
		memset(&webcamMutex, 0, sizeof(webcamMutex));
		memset(&streamerThreadHandle, 0, sizeof(streamerThreadHandle));

		TRACE("Creating webcam mutex...");
		int err = pthread_mutex_init(&webcamMutex, NULL);
		if (err) {
			THROW_ERROR("Error creating webcam mutex: " << strerror(err));
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
			boundHandlers.push_back(                            \
				bind(                                           \
					&WebcamServerConnection::handle_##msg_type, \
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

	WebcamServerConnection::~WebcamServerConnection ()
	{
		TRACE_ENTER;

		// Attempt to stop the streamer thread if it's running
		try
		{
			stopStream();
		}
		catch (runtime_error e)
		{
			ERROR(e.what());
		}

		// Attempt to destroy the mutex
		int status = pthread_mutex_destroy(&webcamMutex);
		if (status) {
			ERROR("Unable to destroy webcamMutex: " << strerror(status));
		}

		TRACE_EXIT;
	}

	void
	WebcamServerConnection::streamerThread (void* unused)
	{
		TRACE_ENTER;
		try
		{
			MutexLock lock(webcamMutex);
			lock.relock();
			webcam->startCapture();
			lock.unlock();

			//TODO: Look into signals for a better way of getting the
			// thread to terminate. In particular, I don't want to wait
			// for a frame from the webcam before I realize I should
			// shut down.
			while (streamIsActiveFlag)
			{
				lock.relock();
				shared_ptr<MappedBuffer> frame = webcam->getFrame();
				lock.unlock();

				sendMessage(SERVER_MSG_FRAME, frame->length, frame->data);
			}

			return;
		}
		catch (runtime_error e)
		{
			cerr << "!! Caught exception in reader thread:" << endl
			     << "!! " << e.what() << endl;
			return;
		}

		// Attempt to stop the webcam video capture. Do it in a try/catch block
		// in case something happened to the webcam and this throws and error.
		try
		{
			MutexLock lock(webcamMutex);
			lock.relock();
			webcam->stopCapture();
			lock.unlock();
		}
		catch (runtime_error e)
		{
			cerr << "!! Caught exception in reader thread:" << endl
			     << "!! " << e.what() << endl;
			return;
		}
	}

	void
	WebcamServerConnection::handle_ERROR_MSG_INVALID_MSG
		(message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		if (length != sizeof(message_t))
		{
			ERROR("Client reports invalid message, but did not specify what "
			      "type of message it was.");
		}
		else
		{
			message_t rogueType = *reinterpret_cast<message_t*>(buffer);
			ERROR("Client reports invalid message of type "
			   << webcamSocketMsgToString(rogueType));
		}
		TRACE_EXIT;
	};

	void
	WebcamServerConnection::handle_ERROR_MSG_TERMINATING_CONNECTION
		(message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		MESSAGE("Client is terminating the connection.");
		close();
		TRACE_EXIT;
	}

	void
	WebcamServerConnection::handle_CLIENT_MSG_GET_WEBCAM_LIST
		(message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		// TODO
		sendMessage(SERVER_ERR_RUNTIME_ERROR, string(__FUNCTION__) + " is currently unimplemented.");
		TRACE_EXIT;
	}

	void
	WebcamServerConnection::handle_CLIENT_MSG_GET_WEBCAM_STATUS
		(message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;

		if (webcam) {
			sendMessage(SERVER_MSG_WEBCAM_IS_OPENED, webcam->getFilename());
		} else {
			sendMessage(SERVER_MSG_WEBCAM_IS_CLOSED);
		}

		TRACE_EXIT;
	}

	void
	WebcamServerConnection::handle_CLIENT_MSG_OPEN_WEBCAM
		(message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;

		// TODO: Implement some sort of list of devices so we can validate this
		// against that list and not, oh, say, inject shell code !!?!
		// At least validate the filename!
		string newFilename = string(reinterpret_cast<char*>(buffer), length);
		shared_ptr<Webcam> newcam;

		try
		{
			// Attempt to open new webcam
			newcam = shared_ptr<Webcam>(new Webcam(newFilename));
		}
		catch (runtime_error e)
		{
			ERROR("Unable to open webcam " << newFilename << " for client:");
			ERROR(e.what());
			sendMessage(SERVER_ERR_WEBCAM_UNAVAILABLE, length, buffer);
			TRACE_EXIT;
			return;
		}

		// Close the old webcam, using the official handler to make sure things
		// are done correctly -- stopping the stream, notifying the client, etc
		if (webcam)
		{
			handle_CLIENT_MSG_CLOSE_WEBCAM(CLIENT_MSG_CLOSE_WEBCAM, 0, NULL);
		}

		MutexLock lock(webcamMutex);
		lock.relock();
		webcam = newcam;
		lock.unlock();

		sendMessage(SERVER_MSG_WEBCAM_IS_OPENED, webcam->getFilename());

		TRACE_EXIT;
	}

	void
	WebcamServerConnection::handle_CLIENT_MSG_CLOSE_WEBCAM
		(message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;

		if (streamIsActiveFlag) {
			handle_CLIENT_MSG_STOP_STREAM(CLIENT_MSG_STOP_STREAM, 0, NULL);
		}

		// Release the webcam through garbage collection
		MutexLock lock(webcamMutex);
		lock.relock();
		webcam = shared_ptr<Webcam>();
		lock.unlock();

		sendMessage(SERVER_MSG_WEBCAM_IS_CLOSED);

		TRACE_EXIT;
	}

	void
	WebcamServerConnection::handle_CLIENT_MSG_GET_SUPPORTED_SPECS
		(message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		if (!webcam)
		{
			sendMessage(SERVER_ERR_NO_WEBCAM_OPENED);
		}
		else
		{
			try
			{
				vector<struct image_spec> specs;

				MutexLock lock(webcamMutex);
				lock.relock();

				Webcam::fmtdesc_v fmts = *(webcam->getSupportedFormats());
				for_each(fmts.begin(), fmts.end(), [&specs, this] (struct v4l2_fmtdesc fmt)
				{
					Webcam::resolution_set rezes = *(webcam->getSupportedResolutions(fmt.pixelformat));
					for_each(rezes.begin(), rezes.end(), [&specs, &fmt] (Webcam::resolution_t res)
					{
						specs.push_back({ res.first, res.second, fmt.pixelformat });
					});
				});

				lock.unlock();

				sendMessage(SERVER_MSG_SUPPORTED_SPECS,
			            	specs.size() * sizeof(struct image_spec),
			            	&(specs[0]));
			}
			catch (runtime_error e)
			{
				ERROR(e.what());
				sendMessage(SERVER_ERR_RUNTIME_ERROR, e.what());
			}
		}
		TRACE_EXIT;
	}

	void
	WebcamServerConnection::handle_CLIENT_MSG_GET_CURRENT_SPEC
		(message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		if (!webcam)
		{
			sendMessage(SERVER_ERR_NO_WEBCAM_OPENED);
		}
		else
		{
			try
			{
				MutexLock lock(webcamMutex);
				lock.relock();

				struct image_spec spec;
				Webcam::resolution_t res = webcam->getResolution();
				spec.width = res.first;
				spec.height = res.second;
				spec.fmt = webcam->getImageFormat();

				lock.unlock();
				
				sendMessage(SERVER_MSG_IMAGE_SPEC, sizeof(struct image_spec), &spec);
			}
			catch (runtime_error e)
			{
				ERROR(e.what());
				sendMessage(SERVER_ERR_RUNTIME_ERROR, e.what());
			}
		}
		TRACE_EXIT;
	}

	void
	WebcamServerConnection::handle_CLIENT_MSG_SET_CURRENT_SPEC
		(message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		if (!webcam)
		{
			sendMessage(SERVER_ERR_NO_WEBCAM_OPENED);
		}
		else if (length != sizeof(struct image_spec))
		{
			sendMessage(SERVER_ERR_INVALID_SPEC);
		}
		else
		{
			struct image_spec spec = *reinterpret_cast<struct image_spec*>(buffer);
			try
			{
				MutexLock lock(webcamMutex);
				lock.relock();

				webcam->setImageFormat(spec.fmt, spec.width, spec.height);

				lock.unlock();

				// If the above hasn't thrown an exception, tell the client it worked.
				handle_CLIENT_MSG_GET_CURRENT_SPEC(CLIENT_MSG_GET_CURRENT_SPEC, 0, NULL);
			}
			catch (runtime_error e)
			{
				ERROR(e.what());
				sendMessage(SERVER_ERR_RUNTIME_ERROR, e.what());
			}
		}
		TRACE_EXIT;
	}

	void
	WebcamServerConnection::handle_CLIENT_MSG_GET_STREAM_STATUS
		(message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		sendMessage(streamIsActiveFlag ? SERVER_MSG_STREAM_IS_STARTED :
		                                 SERVER_MSG_STREAM_IS_STOPPED);
		TRACE_EXIT;
	}

	void
	WebcamServerConnection::handle_CLIENT_MSG_START_STREAM
		(message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		try
		{
			startStream();
		}
		catch (NoWebcamOpenException e)
		{
			ERROR(e.what());
			sendMessage(SERVER_ERR_NO_WEBCAM_OPENED, e.what());
		}
		catch (runtime_error e)
		{
			ERROR(e.what());
			sendMessage(SERVER_ERR_RUNTIME_ERROR, e.what());
		}
		TRACE_EXIT;
	}

	void
	WebcamServerConnection::startStream ()
	{
		if (!webcam)
		{
			throw NoWebcamOpenException("Unable to start stream: No webcam is open.");
		}
		else if (streamIsActiveFlag)
		{
			MESSAGE("Client tried to start a stream when it's already started");
			sendMessage(SERVER_MSG_STREAM_IS_STARTED);
		}
		else
		{
			MESSAGE("Starting stream");

			// Without this flag set, the streamer thread will think it's
			// being told to stop.
			streamIsActiveFlag = true;

			streamerThreadHandle = pthread_create_using_method<WebcamServerConnection, void*>(
					*this, &WebcamServerConnection::streamerThread, NULL
			);

			sendMessage(SERVER_MSG_STREAM_IS_STARTED);
		}
	}

	void
	WebcamServerConnection::handle_CLIENT_MSG_STOP_STREAM
		(message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		try
		{
			stopStream();
			sendMessage(SERVER_MSG_STREAM_IS_STOPPED);
		}
		catch (runtime_error e)
		{
			ERROR(e.what());
			sendMessage(SERVER_ERR_RUNTIME_ERROR, e.what());
		}
		TRACE_EXIT;
	}

	void
	WebcamServerConnection::stopStream()
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
				THROW_ERROR("Unable to terminate streamer thread: " << strerror(err));
			}
		}
		else
		{
			MESSAGE("Stream is already stopped");
		}

		// Notify the client, if connected.
		try {
			sendMessage(SERVER_MSG_STREAM_IS_STOPPED);
		} catch (runtime_error e) {
			ERROR(e.what());
		}
	}


///// WebcamServer /////
	shared_ptr<Connection>
	WebcamServer::newConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort)
	{
		TRACE_ENTER;
		return shared_ptr<Connection>(new WebcamServerConnection(fd, remoteAddress, remotePort));
		TRACE_EXIT;
	}

