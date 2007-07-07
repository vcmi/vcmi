#ifndef SDL_EXTENSIONS_H
#define SDL_EXTENSIONS_H
#include "SDL.h"
#include "SDL_ttf.h"


extern SDL_Surface * ekran;
extern SDL_Color tytulowy, tlo, zwykly ;
class CSDL_Ext
{
public:
	static void SDL_PutPixel(SDL_Surface *ekran, int x, int y, Uint8 R, Uint8 G, Uint8 B, int myC=0, Uint8 A = 255); //myC influences the start of reading pixels
	static SDL_Surface * rotate01(SDL_Surface * toRot, int myC = 2); //vertical flip
	static SDL_Surface * hFlip(SDL_Surface * toRot); //horizontal flip
	static SDL_Surface * rotate02(SDL_Surface * toRot); //rotate 90 degrees left
	static SDL_Surface * rotate03(SDL_Surface * toRot); //rotate 180 degrees
	static SDL_Cursor * SurfaceToCursor(SDL_Surface *image, int hx, int hy);
	static Uint32 SDL_GetPixel(SDL_Surface *surface, int x, int y, bool colorByte = false);
	static SDL_Color SDL_GetPixelColor(SDL_Surface *surface, int x, int y);
	static SDL_Surface * alphaTransform(SDL_Surface * src); //adds transparency and shadows (partial handling only; see examples of using for details)
	static SDL_Surface * secondAlphaTransform(SDL_Surface * src, SDL_Surface * alpha); //alpha is a surface we want to blit src to
	static Uint32 colorToUint32(const SDL_Color * color); //little endian only
	static void printAt(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=ekran, unsigned char quality = 2); // quality: 0 - lowest, 1 - medium, 2 - highest
};

#endif // SDL_EXTENSIONS_H