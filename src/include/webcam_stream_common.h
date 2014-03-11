#ifndef WEBCAM_STREAM_COMMON
#define WEBCAM_STREAM_COMMON

#include <netinet/in.h> // in_addr_t, in_port_t
#include <sstream>
#include <string>

struct image_spec
{
	uint32_t width;
	uint32_t height;
	uint32_t fmt;
};

const in_port_t DEFAULT_PORT = 32123;

enum WEBCAM_SOCKET_MSG_ENUM
{
	/**
	 * An unexpected message was received.
	 *
	 * @param <uint32_t> The message ID that was received
	 */
	ERROR_MSG_INVALID_MSG,

	/**
	 * The server or client needs to terminate the connection;
	 * the other end should close down safely as well.
	 *
	 * @param none
	 */
	ERROR_MSG_TERMINATING_CONNECTION,

  /// @name Client messages
  /// Control messages sent from the WebcamClient to the WebcamServer.
  ///@{

	/**
	 * Query whether a webcam device is opened.
	 *
	 * @param none
	 *
	 * @return SERVER_MSG_WEBCAM_IS_OPENED
	 * @return SERVER_MSG_WEBCAM_IS_CLOSED
	 */
	CLIENT_MSG_GET_WEBCAM_STATUS,

	/**
	 * Ask for list of available webcam devices.
	 *
	 * @param none
	 *
	 * @return SERVER_MSG_WEBCAM_LIST
	 */
	CLIENT_MSG_GET_WEBCAM_LIST,

	/**
	 * Request that a webcam be opened.
	 *
	 * If another webcam is opened, the previous webcam will be automatically
	 * closed as if CLIENT_MSG_CLOSE_WEBCAM had been sent. This implicitly
	 * means the server will send SERVER_MSG_STREAM_IS_STOPPED (if a stream
	 * was active) and SERVER_MSG_WEBCAM_IS_CLOSED.
	 *
	 * If this request fails and a webcam was previously open, it will remain
	 * open.
	 *
	 * @param <char[]> Webcam name (?)
	 *
	 * @return SERVER_MSG_WEBCAM_IS_OPENED
	 * @throws SERVER_ERR_WEBCAM_UNAVAILABLE  If there is an error opening the
	 *                                        webcam; e.g. it's busy
	 */
	CLIENT_MSG_OPEN_WEBCAM,

	/**
	 * Request that a webcam be closed.
	 *
	 * If a stream is currently running, the stream will be stopped as if
	 * CLIENT_MSG_STOP_STREAM had been sent.
	 *
	 * @param none
	 *
	 * @return SERVER_MSG_WEBCAM_IS_CLOSED
	 * @return SERVER_MSG_STREAM_IS_STOPPED
	 */
	CLIENT_MSG_CLOSE_WEBCAM,

	/**
	 * Query whether the server is streaming data from an open webcam.
	 *
	 * @param none
	 *
	 * @return SERVER_MSG_STREAM_IS_STARTED
	 * @return SERVER_MSG_STREAM_IS_STOPPED
	 */
	CLIENT_MSG_GET_STREAM_STATUS,

	/**
	 * Query the current image specification
	 * (i.e. the resolution and pixel format).
	 *
	 * @param <struct image_spec> The current image specification
	 *
	 * @return SERVER_MSG_IMAGE_SPEC
	 * @throws SERVER_ERR_NO_WEBCAM_OPENED   If no webcam has been opened
	 */
	CLIENT_MSG_GET_CURRENT_SPEC,

	/**
	 * Enumerate all available image specifications
	 * (i.e. the set of available resolution and pixel format combinations).
	 *
	 * @param <struct image_spec[]> An array of image specifications
	 *
	 * @return SERVER_MSG_SUPPORTED_SPECS
	 * @throws SERVER_ERR_NO_WEBCAM_OPENED   If no webcam has been opened
	 */
	CLIENT_MSG_GET_SUPPORTED_SPECS,

	/**
	 * Change the current image specification.
	 *
	 * @param <struct image_spec> The new image specification
	 *
	 * @return SERVER_MSG_IMAGE_SPEC
	 * @throws SERVER_ERR_INVALID_SPEC       If the image spec wasn't listed
	 *                                       by SERVER_MSG_SUPPORTED_SPECS
	 * @throws SERVER_ERR_NO_WEBCAM_OPENED   If no webcam has been opened
	 */
	CLIENT_MSG_SET_CURRENT_SPEC,

	/**
	 * Asks the server to start streaming frames.
	 *
	 * @param none
	 *
	 * @return SERVER_MSG_STREAM_IS_STARTED
	 * @throws SERVER_ERR_NO_WEBCAM_OPENED   If no webcam has been opened
	 */
	CLIENT_MSG_START_STREAM,

	/**
	 * Asks the server to stop sending frames.
	 *
	 * @param none
	 *
	 * @return SERVER_MSG_STREAM_IS_STOPPED
	 */
	CLIENT_MSG_STOP_STREAM,

  ///@}

  /// @name Client messages
  /// Control messages sent from the WebcamClient to the WebcamServer.
  ///@{

	/**
	 * A single frame captured by the camera.
	 *
	 * @param Raw binary data conforming to the current image specification
	 */
	SERVER_MSG_FRAME,

	/**
	 * The current specification (pixel format and resolution) of frames that
	 * would come from the webcam if a stream is active.
	 *
	 * @param <struct image_spec> The current specification
	 */
	SERVER_MSG_IMAGE_SPEC,

	/**
	 * The opened webcam is currently sending SERVER_MSG_FRAME messages as
	 * frames become available from the camera.
	 *
	 * @param none
	 */
	SERVER_MSG_STREAM_IS_STARTED,

	/**
	 * No stream is running; in particular, if a stream was previously active,
	 * it is now stopped.
	 *
	 * @param none
	 */
	SERVER_MSG_STREAM_IS_STOPPED,

	/**
	 * A list of legal image specifications the webcam is capable of supplying.
	 *
	 * @param <struct image_spec[]> An array of supported image specifications
	 */
	SERVER_MSG_SUPPORTED_SPECS,

	/**
	 * Any previously-opened webcam is now closed, or no webcam was open in
	 * the first place.
	 *
	 * @param none
	 */
	SERVER_MSG_WEBCAM_IS_CLOSED,

	/**
	 * A webcam is open and available for streaming.
	 *
	 * @param <char[]> The name of the open webcam
	 */
	SERVER_MSG_WEBCAM_IS_OPENED,

	/**
	 * A list of webcams available to be opened.
	 *
	 * @param <char[][]> The list of webcams as a sequence of zero-terminated
	 *                   strings, the last of which followed by a second '\0'
	 *                   character.
	 */
	SERVER_MSG_WEBCAM_LIST,

	/**
	 * The previous call to CLIENT_MSG_SET_CURRENT_SPEC failed because it
	 * specified a pixel format and/or resolution the webcam could not produce.
	 *
	 * @param none
	 */
	SERVER_ERR_INVALID_SPEC,

	/**
	 * The previous message failed because no webcam is currently opened.
	 *
	 * @param none
	 */
	SERVER_ERR_NO_WEBCAM_OPENED,

	/**
	 * The server experienced an internal runtime error. It isn't necessarily
	 * crashing, but it's reporting it to the client for human diagnosis.
	 * 
	 * @param <char[]> The error message string
	 */
	SERVER_ERR_RUNTIME_ERROR,

	/**
	 * The previous call to CLIENT_MSG_OPEN_WEBCAM failed because the webcam
	 * is busy or doesn't exist.
	 *
	 * @param <char[]> The webcam the client tried to open
	 */
	SERVER_ERR_WEBCAM_UNAVAILABLE,

  ///@}
};

inline std::string
webcamSocketMsgToString (uint32_t messageId)
{
	std::stringstream ss;

	switch (messageId)
	{
	// I admit I occasionally take DRY to extremes.
	#define DEFINE_MSG(x)\
		case x: return #x;
	
		DEFINE_MSG ( ERROR_MSG_INVALID_MSG            );
		DEFINE_MSG ( ERROR_MSG_TERMINATING_CONNECTION );

		DEFINE_MSG ( CLIENT_MSG_CLOSE_WEBCAM          );
		DEFINE_MSG ( CLIENT_MSG_GET_CURRENT_SPEC      );
		DEFINE_MSG ( CLIENT_MSG_GET_STREAM_STATUS     );
		DEFINE_MSG ( CLIENT_MSG_GET_SUPPORTED_SPECS   );
		DEFINE_MSG ( CLIENT_MSG_GET_WEBCAM_STATUS     );
		DEFINE_MSG ( CLIENT_MSG_GET_WEBCAM_LIST       );
		DEFINE_MSG ( CLIENT_MSG_OPEN_WEBCAM           );
		DEFINE_MSG ( CLIENT_MSG_STOP_STREAM           );
		DEFINE_MSG ( CLIENT_MSG_SET_CURRENT_SPEC      );
		DEFINE_MSG ( CLIENT_MSG_START_STREAM          );

		DEFINE_MSG ( SERVER_MSG_FRAME                 );
		DEFINE_MSG ( SERVER_MSG_IMAGE_SPEC            );
		DEFINE_MSG ( SERVER_MSG_STREAM_IS_STARTED     );
		DEFINE_MSG ( SERVER_MSG_STREAM_IS_STOPPED     );
		DEFINE_MSG ( SERVER_MSG_SUPPORTED_SPECS       );
		DEFINE_MSG ( SERVER_MSG_WEBCAM_IS_CLOSED      );
		DEFINE_MSG ( SERVER_MSG_WEBCAM_IS_OPENED      );
		DEFINE_MSG ( SERVER_MSG_WEBCAM_LIST           );
		DEFINE_MSG ( SERVER_ERR_INVALID_SPEC          );
		DEFINE_MSG ( SERVER_ERR_NO_WEBCAM_OPENED      );
		DEFINE_MSG ( SERVER_ERR_RUNTIME_ERROR         );
		DEFINE_MSG ( SERVER_ERR_WEBCAM_UNAVAILABLE    );

	#undef DEFINE_MSG

	default:
		ss << "(Unknown message type " << messageId << ")";
		return ss.str();
	}
}

#endif // WEBCAM_STREAM_COMMON

