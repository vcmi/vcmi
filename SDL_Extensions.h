#ifndef SDL_EXTENSIONS_H
#define SDL_EXTENSIONS_H
#include "SDL.h"

class CSDL_Ext
{
public:
	static void SDL_PutPixel(SDL_Surface *ekran, int x, int y, Uint8 R, Uint8 G, Uint8 B);
	static SDL_Surface * rotate01(SDL_Surface * toRot);
	static SDL_Surface * hFlip(SDL_Surface * toRot); //horizontal flip
	static SDL_Surface * rotate02(SDL_Surface * toRot);
	static SDL_Surface * rotate03(SDL_Surface * toRot);
};

#endif // SDL_EXTENSIONS_H