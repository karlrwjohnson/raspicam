#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>   // pair

#include <SDL2/SDL.h>

#include "Webcam.h"

#ifndef WEBCAM_VIEWER_H
#define WEBCAM_VIEWER_H

class QuitEventException : public std::runtime_error {
  public:
	QuitEventException() :
		std::runtime_error("Caught quit request event")
	{ }
};

class IncompatibleFormatException : public std::runtime_error {
  public:
	IncompatibleFormatException() :
		std::runtime_error("Caught quit request event")
	{ }
};

/**
 * A barebones window to display a webcam feed
 */
class WebcamViewer {

	uint32_t width;
	uint32_t height;
	uint32_t sdlImageFormat;

	SDL_Window   *window;
	SDL_Renderer *renderer;
	SDL_Texture  *canvas;

  public:

	/**
	 * Opens an SDL window to accept frames.
	 * @param width_          The dimensions of the framebuffer
	 * @param height_
	 * @param title	          For the titlebar
	 * @param v4lImageFormat  A supported Video4Linux image format constant
	 *                        (not an SDL format constant), as defined in
	 *                        videodev2.h. For a list of supported formats,
	 *                        see v4l2sdl_fmt()
 	 * @throws IncompatibleFormatException
 	 *                        If the format is not supported
	 */
	WebcamViewer (
		uint32_t width_,
		uint32_t height_,
		std::string title,
		Webcam::video_fmt_enum_t v4lImageFormat
	);

	~WebcamViewer ();

	/**
	 * Checks if the user has indicated that they want to exit the program,
	 * i.e. by closing the window. If this function isn't periodically polled,
	 * the operating system will think the program "isn't responding".
	 *
	 * @throws QuitEventException If the user has indicated the application
	 *                            should quit
	 */
	void
	checkEvents ();

	/**
	 * Change the size of image to accept
	 * @param newWidth         The dimensions of the framebuffer
	 * @param newHeight
	 */
	void
	setImageSize (uint32_t newWidth, uint32_t newHeight);

	/**
	 * Change the size of image to accept
	 * @param size             The dimensions of the framebuffer
	 */
	void
	setImageSize (Webcam::resolution_t size);

	/**
	 * Change the image format
	 * @param v4lImageFormat  A supported Video4Linux image format constant
	 *                        (not an SDL format constant), as defined in
	 *                        videodev2.h. For a list of supported formats,
	 *                        see v4l2sdl_fmt()
 	 * @throws IncompatibleFormatException
 	 *                        If the format is not supported
	 */
	void
	setImageFormat (Webcam::video_fmt_enum_t v4lImageFormat);

	/**
	 * Draws a frame to the screen
	 * @param  sourceBuffer   The image data to draw
	 * @param  sourceLength   The size of the image buffer
	 * @throws runtime_error  If sourceLength doesn't match the destination
	 *                        image buffer, calculated by the image height and
	 *                        pitch (bytes per row) given by SDL.
	 */
	void
	showFrame (void* sourceBuffer, size_t sourceLength);

	/**
 	 * Converts a Video4Linux image format to its equivalent SDL image formats, if
 	 * one exists.
 	 *
 	 * Hopefully, this will provide at least some resilience against hardware changes.
 	 * If no equivalent format can be located, this throws an exception.
 	 *
 	 * These formats are supported:
 	 *  - V4L2_PIX_FMT_RGB332 -> SDL_PIXELFORMAT_RGB332
 	 *  - V4L2_PIX_FMT_RGB444 -> SDL_PIXELFORMAT_RGB444
 	 *  - V4L2_PIX_FMT_RGB555 -> SDL_PIXELFORMAT_RGB555
 	 *  - V4L2_PIX_FMT_RGB565 -> SDL_PIXELFORMAT_RGB565
 	 *  - V4L2_PIX_FMT_BGR24  -> SDL_PIXELFORMAT_BGR888
 	 *  - V4L2_PIX_FMT_RGB24  -> SDL_PIXELFORMAT_RGB888
 	 *  - V4L2_PIX_FMT_BGR32  -> SDL_PIXELFORMAT_BGRX8888
 	 *  - V4L2_PIX_FMT_RGB32  -> SDL_PIXELFORMAT_RGBX8888
 	 *  - V4L2_PIX_FMT_YUYV   -> SDL_PIXELFORMAT_YUY2
 	 *  - V4L2_PIX_FMT_YVYU   -> SDL_PIXELFORMAT_YVYU
 	 *  - V4L2_PIX_FMT_UYVY   -> SDL_PIXELFORMAT_UYVY
 	 *
 	 * @param  v4l2_fmt  A V4L2_PIX_FMT_* constant defined in videodev2.h
 	 * @return           Its corresponding SDL_PIXELFORMAT_* constant,
 	 *                   if one exists
 	 * @throws IncompatibleFormatException
 	 *                   If no supported format exists
 	 */
	static uint32_t
	v4l2sdl_fmt(Webcam::video_fmt_enum_t v4l2_fmt);
};

#endif // WEBCAM_VIEWER_H

