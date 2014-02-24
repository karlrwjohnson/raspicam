#include <cstddef>
#include <stdexcept>
#include <string>

#include <SDL2/SDL.h>

class QuitEventException : public std::runtime_error {
  public:
	QuitEventException() :
		std::runtime_error("Caught quit request event")
	{ }
};

/**
 * A barebones window to display a webcam feed
 */
class WebcamViewer {

	int canvaswidth;
	int canvasheight;

	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *canvas;

  public:

	/**
	 * Opens an SDL window to accept frames.
	 * @param width        The dimensions of the framebuffer
	 * @param height
	 * @param title	       For the titlebar
	 * @param imageFormat  A Video4Linux image format constant (not SDL's),
	 *                     defined in videodev2.h
	 */
	WebcamViewer (int width, int height, std::string title, int imageFormat);
	~WebcamViewer ();

	void checkEvents();
	void showFrame (void* imgdata, size_t datalength);
};

uint32_t
v4l2sdl_fmt(uint32_t fmt);

