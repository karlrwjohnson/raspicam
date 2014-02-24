/*
	libSDL2 demo which displays a bitmap.
	Specifically, the one located at ../Bitmap 2 UTF-8/chicago200.bmp.

	... and then it starts to go all 3rd-grader on said bitmap, drawing random crap.

	Compile: g++ sdl_demo.cpp -o sdl_demo -std=c++0x -lSDL2
	Run: ./sdl_demo
*/

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>

#include <SDL2/SDL.h>

using namespace std;


struct pixel_rgba {

	// ARGB8888, little-endian
	uint8_t b, g, r, a;

	pixel_rgba() : r(0), g(0), b(0), a(0)
	{ }

	pixel_rgba(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) :
		r(_r), g(_g), b(_b), a(_a)
	{ }
};

// ARGB8888 storage mask
const uint32_t rmask = 0x00ff0000;
const uint32_t gmask = 0x0000ff00;
const uint32_t bmask = 0x000000ff;
const uint32_t amask = 0xff000000;

SDL_Rect sdl_rect(int x, int y, int w, int h) {
	SDL_Rect ret;
	ret.x = x;
	ret.y = y;
	ret.w = w;
	ret.h = h;
	return ret;
}

void doIt() {
	int status;

	stringstream err_ss;
	#define THROW_ERROR(error) \
		err_ss << __FUNCTION__ << ":" << __LINE__ << " " << error;\
		throw runtime_error(err_ss.str())

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		THROW_ERROR("SDL_Init Error: " << SDL_GetError());
	}

	SDL_Window *window = SDL_CreateWindow("Hello World!", 100, 100, 640, 480, SDL_WINDOW_SHOWN);

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	if (renderer == NULL) {
		THROW_ERROR("SDL_CreateRenderer Error: " << SDL_GetError());
	}

	SDL_Surface *bmp = SDL_LoadBMP("../Bitmap 2 UTF-8/chicago200.bmp");
	if (bmp == NULL) {
		THROW_ERROR("SDL_LoadBMP Error: " << SDL_GetError());
	}

	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, bmp);
	SDL_FreeSurface(bmp);
	if (tex == NULL) {
		THROW_ERROR("SDL_CreateTextureFromSurface Error: " << SDL_GetError());
	}

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, tex, NULL, NULL);
	SDL_RenderPresent(renderer);

////// This wasn't part of the first tutorial.

	// A tutorial for streaming textures is at:
	// http://slouken.blogspot.com/2011/02/streaming-textures-with-sdl-13.html

	// I could create another surface, make a texture based on that,
	// and update the texture with SDL_UpdateTexture every frame, but
	// apparently that's bad performance (an extra copy operation!)
	//SDL_Surface *canvas = SDL_CreateRGBSurface(0, canvaswidth, canvasheight, 32, rmask, gmask, bmask, amask);
	//if (canvas == NULL) {
	//	THROW_ERROR("SDL_CreateTextureFromSurface Error: " << SDL_GetError());
	//}

	// Instead, I can just create the texture to begin with and write to it directly.
	// The SDL_Surface struct is good for "old-style SDL blitting operations".
	// See http://wiki.libsdl.org/SDL_CreateTexture for a list of formats.
	// I'll want to use SDL_PIXELFORMAT_YUY2 for the webcam, since that's its native format.
	int canvaswidth = 120;
	int canvasheight = 100;
	SDL_Texture *canvastex = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		canvaswidth,
		canvasheight
	);
	if (canvastex == NULL) {
		THROW_ERROR("SDL_CreateTexture Error: " << SDL_GetError());
	}

	// Location to paste the texture
	SDL_Rect pasteLoc = sdl_rect(50,50,170,150);

	// It's necessary to lock the texture each frame before we can write to it.
	// SDL_LockTexture() passes back two pieces of information: a pointer to
	// the buffer, and the "pitch" -- which I think is the number of bytes per
	// row of pixels. I guess it's useful for manual blitting.
	void* pixels;
	int pitch;
	status = SDL_LockTexture(canvastex, NULL, &pixels, &pitch);
	if (status) {
		THROW_ERROR("SDL_LockTexture Error: " << SDL_GetError());
	}

	cout << "pitch = " << pitch << "\n";

	// Test pattern: A black-green-yellow-red 2D gradient
	for (int y = 0; y < canvasheight; y++)
	for (int x = 0; x < canvaswidth; x++)
	{
		pixel_rgba *dest = (pixel_rgba*) pixels + (y * canvaswidth) + x;
		uint8_t xPercent = (int) (0xff * x / canvaswidth);
		uint8_t yPercent = (int) (0xff * y / canvasheight);
		*dest = pixel_rgba(xPercent, yPercent, 0x00, 0xff);
	}

	// Unlock the texture so it can be used
	SDL_UnlockTexture(canvastex);

	// It seems we need to manually re-call the render methods for each frame.
	// RenderClear obviously clears the buffer, RenderCopy blits each texture,
	// and RenderPresent must switch the back and front framebuffers.
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, tex, NULL, NULL);
	SDL_RenderCopy(renderer, canvastex, NULL, &pasteLoc);
	SDL_RenderPresent(renderer);

	//SDL_Delay(2000);
	getchar();

////// Free resources

	SDL_DestroyTexture(tex);
	SDL_DestroyTexture(canvastex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

int main() {
	try {
		doIt();
	}
	catch (runtime_error e) {
		cerr << "=============\n"
		     << "Runtime Error:\n"
		     << "\t" << e.what() << "\n";
		return 1;
	}
}

