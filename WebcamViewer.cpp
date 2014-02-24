#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>

#include "WebcamViewer.h"
#include "Log.h"

#include <linux/videodev2.h>

using namespace std;

/**
 * Global flag for what really should be a singleton.
 *
 * I don't think SDL lets you start up multiple instances, but I wanted to
 * wrap it in an object so I could use the destructor to take things down
 * gracefully when it went out of scope.
 *
 * Did I mention the program officially terminates by throwing an exception?
 */
bool initiated = false;

WebcamViewer::WebcamViewer(int width, int height, string title, int imageFormat)
{
	TRACE("enter");

	if(initiated) {
		THROW_ERROR("Cannot start multiple instances of SDL, and by extension, WebcamViewer.");
	}

	TRACE("\tstarting SDL");

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

	canvaswidth = width;
	canvasheight = height;
	TRACE("\tcanvasheight = " << height);
	TRACE("\tcanvaswidth = " << width);

	canvas = SDL_CreateTexture(
		renderer,
		v4l2sdl_fmt(imageFormat), // i.e. SDL_PIXELFORMAT_YUY2
		SDL_TEXTUREACCESS_STREAMING,
		canvaswidth,
		canvasheight
	);
	if (canvas == NULL)
	{
		THROW_ERROR("SDL_CreateTexture Error: " << SDL_GetError());
	}

	initiated = true;

	TRACE("\texit");
}

WebcamViewer::~WebcamViewer()
{
	TRACE(" enter");

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

	TRACE("\texit");
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
WebcamViewer::showFrame (void* imgdata, size_t datalength)
{
	TRACE("enter\n")

	if (!initiated)
	{
		THROW_ERROR("showimg_init() hasn't been called yet!");
	}

	void* pixels;
	int pitch;
	if (SDL_LockTexture(canvas, NULL, &pixels, &pitch))
	{
		THROW_ERROR("SDL_LockTexture Error: " << SDL_GetError());
	}

	if (datalength != canvasheight * pitch)
	{
		SDL_UnlockTexture(canvas);
		THROW_ERROR("Image data size mismatch: Source buffer is " << datalength
			<< " bytes; destination buffer is " << (canvasheight * pitch) << " bytes"
			<< " (canvasheight=" << canvasheight << "; pitch = " << pitch << ")"
		);
	}

	memcpy(pixels, imgdata, datalength);

	SDL_UnlockTexture(canvas);

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, canvas, NULL, NULL);
	SDL_RenderPresent(renderer);

	TRACE("exit")
}

/**
 * Conversion between Video4Linux and SDL image formats.
 * Hopefully, this will provide at least some resilience against hardware changes.
 * If the correct conversion can't be located, this throws an exception so the 
 * failure is detected nice and early.
 *
 * @param v4l2_fmt   A V4L2_PIX_FMT_* constant defined in videodev2.h
 * @return           Its corresponding SDL_PIXELFORMAT_* constant, if it exists
 */
uint32_t
v4l2sdl_fmt (uint32_t v4l2_fmt)
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
		THROW_ERROR("Unknown Video4Linux2 image format " << c0 << c1 << c2 << c3);
  }
}

