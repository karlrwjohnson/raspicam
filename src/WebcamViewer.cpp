#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>

#include "Log.h"
#include "WebcamViewer.h"

#include <linux/videodev2.h>

using namespace std;

/**
 * Global flag ensuring that only one instance of WebcamViewer is instantiated.
 *
 * I don't think SDL lets you start up multiple instances, but I wanted to
 * wrap it in an object so I could use the destructor to take things down
 * gracefully when it went out of scope.
 *
 * Using a singleton might have been more appropriate.
 */
bool initiated = false;

WebcamViewer::WebcamViewer(
		uint32_t width_,
		uint32_t height_,
		string title,
		Webcam::video_fmt_enum_t v4lImageFormat
):
	width  (width_),
	height (height_)
{
	TRACE_ENTER;

	if(initiated) {
		THROW_ERROR("Cannot start multiple instances of SDL, and by extension, WebcamViewer.");
	}

	// Attempt to convert the format right away
	sdlImageFormat = v4l2sdl_fmt(v4lImageFormat);

	//if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) != 0) {
		THROW_ERROR("SDL_Init Error: " << SDL_GetError());
	}

	window = SDL_CreateWindow(title.c_str(), 0, 0, width, height, SDL_WINDOW_SHOWN);
	if (window == NULL)
	{
		THROW_ERROR("SDL_CreateWindow Error: " << SDL_GetError());
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL)
	{
		THROW_ERROR("SDL_CreateRenderer Error: " << SDL_GetError());
	}

	canvas = SDL_CreateTexture(
		renderer,
		sdlImageFormat,
		SDL_TEXTUREACCESS_STREAMING,
		width,
		height
	);
	if (canvas == NULL)
	{
		THROW_ERROR("SDL_CreateTexture Error: " << SDL_GetError());
	}

	initiated = true;

	TRACE_EXIT;
}

WebcamViewer::~WebcamViewer()
{
	TRACE_ENTER;

	if(canvas) {
		SDL_DestroyTexture(canvas);
		canvas = NULL;
	}

	if(renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = NULL;
	}

	if(window) {
		SDL_DestroyWindow(window);
		window = NULL;
	}

	SDL_Quit();

	initiated = false;

	TRACE_EXIT;
}

void
WebcamViewer::checkEvents ()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			throw QuitEventException();
		}
	}
}

void
WebcamViewer::setImageSize (uint32_t newWidth, uint32_t newHeight)
{
	TRACE_ENTER;

	SDL_Texture *newCanvas = SDL_CreateTexture(
		renderer,
		sdlImageFormat,
		SDL_TEXTUREACCESS_STREAMING,
		newWidth,
		newHeight
	);

	if (newCanvas == NULL) {
		THROW_ERROR("SDL_CreateTexture Error: " << SDL_GetError());
	}

	SDL_DestroyTexture(canvas);
	canvas = newCanvas;

	width = newWidth;
	height = newHeight;

	SDL_SetWindowSize(window, width, height);

	TRACE_EXIT;
}

void
WebcamViewer::setImageSize (Webcam::resolution_t size)
{
	setImageSize(size.first, size.second);
}

void
WebcamViewer::setImageFormat (Webcam::video_fmt_enum_t v4lImageFormat)
{
	TRACE_ENTER;

	uint32_t newFormat = v4l2sdl_fmt(v4lImageFormat);

	SDL_Texture *newCanvas = SDL_CreateTexture(
		renderer,
		newFormat,
		SDL_TEXTUREACCESS_STREAMING,
		width,
		height
	);

	if (newCanvas == NULL) {
		THROW_ERROR("SDL_CreateTexture Error: " << SDL_GetError());
	}

	SDL_DestroyTexture(canvas);
	canvas = newCanvas;

	sdlImageFormat = newFormat;

	TRACE_EXIT;
}

void
WebcamViewer::showFrame (void* sourceBuffer, size_t sourceLength)
{
	TRACE_ENTER;

	void* targetBuffer;
	int   targetPitch;

	if (SDL_LockTexture(canvas, NULL, &targetBuffer, &targetPitch)) {
		THROW_ERROR("SDL_LockTexture Error: " << SDL_GetError());
	}

	if (sourceLength != height * targetPitch) {
		SDL_UnlockTexture(canvas);
		THROW_ERROR("Image data size mismatch: Source buffer is " << sourceLength
			<< " bytes; destination buffer is " << (height * targetPitch) << " bytes"
			<< " (image height = " << height << "; bytes/row = " << targetPitch << ")"
		);
	}

	memcpy(targetBuffer, sourceBuffer, sourceLength);

	SDL_UnlockTexture(canvas);

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, canvas, NULL, NULL);
	SDL_RenderPresent(renderer);

	TRACE_EXIT;
}

uint32_t
WebcamViewer::v4l2sdl_fmt (Webcam::video_fmt_enum_t v4l2_fmt)
{
  switch(v4l2_fmt) {

	case V4L2_PIX_FMT_RGB332: return SDL_PIXELFORMAT_RGB332;
	case V4L2_PIX_FMT_RGB444: return SDL_PIXELFORMAT_RGB444;
	case V4L2_PIX_FMT_RGB555: return SDL_PIXELFORMAT_RGB555;
	case V4L2_PIX_FMT_RGB565: return SDL_PIXELFORMAT_RGB565;
	//case V4L2_PIX_FMT_RGB555X: return SDL_PIXELFORMAT_UNKNOWN  // These put the alpha bit
	//case V4L2_PIX_FMT_RGB565X: return SDL_PIXELFORMAT_UNKNOWN  // in a different byte
	case V4L2_PIX_FMT_BGR24: return SDL_PIXELFORMAT_BGR888;
	case V4L2_PIX_FMT_RGB24: return SDL_PIXELFORMAT_RGB888;
	case V4L2_PIX_FMT_BGR32: return SDL_PIXELFORMAT_BGRX8888;
	case V4L2_PIX_FMT_RGB32: return SDL_PIXELFORMAT_RGBX8888;

	case V4L2_PIX_FMT_YUYV: return SDL_PIXELFORMAT_YUY2;
	case V4L2_PIX_FMT_YVYU: return SDL_PIXELFORMAT_YVYU;
	case V4L2_PIX_FMT_UYVY: return SDL_PIXELFORMAT_UYVY;

	default:
		char c0 = (char) ((v4l2_fmt >>  0) & 0xff),
		     c1 = (char) ((v4l2_fmt >>  8) & 0xff),
		     c2 = (char) ((v4l2_fmt >> 16) & 0xff),
		     c3 = (char) ((v4l2_fmt >> 24) & 0xff);
		THROW_ERROR("Unknown or incompatible Video4Linux2 image format "
		         << c0 << c1 << c2 << c3);
  }
}

