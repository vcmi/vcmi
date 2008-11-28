#ifndef SDL_EXTENSIONS_H
#define SDL_EXTENSIONS_H
#include "SDL.h"
#include "SDL_ttf.h"


extern SDL_Surface * screen;
extern SDL_Color tytulowy, tlo, zwykly ;
extern TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX, *GEORM;
void blitAtWR(SDL_Surface * src, int x, int y, SDL_Surface * dst=screen);
void blitAt(SDL_Surface * src, int x, int y, SDL_Surface * dst=screen);
void blitAtWR(SDL_Surface * src, SDL_Rect pos, SDL_Surface * dst=screen);
void blitAt(SDL_Surface * src, SDL_Rect pos, SDL_Surface * dst=screen);
void updateRect (SDL_Rect * rect, SDL_Surface * scr = screen);
bool isItIn(const SDL_Rect * rect, int x, int y);
template <typename T> int getIndexOf(const std::vector<T> & v, const T & val)
{
	for(int i=0;i<v.size();i++)
		if(v[i]==val)
			return i;
	return -1;
}
inline SDL_Rect genRect(const int & hh, const int & ww, const int & xx, const int & yy);
namespace CSDL_Ext
{
	extern SDL_Surface * std32bppSurface;
	inline void SDL_PutPixel(SDL_Surface *ekran, const int & x, const int & y, const Uint8 & R, const Uint8 & G, const Uint8 & B, Uint8 A = 255); //myC influences the start of reading pixels
	inline void SDL_PutPixelWithoutRefresh(SDL_Surface *ekran, const int & x, const int & y, const Uint8 & R, const Uint8 & G, const Uint8 & B, Uint8 A = 255); //myC influences the start of reading pixels ; without refreshing
	SDL_Surface * rotate01(SDL_Surface * toRot); //vertical flip
	SDL_Surface * hFlip(SDL_Surface * toRot); //horizontal flip
	SDL_Surface * rotate02(SDL_Surface * toRot); //rotate 90 degrees left
	SDL_Surface * rotate03(SDL_Surface * toRot); //rotate 180 degrees
	SDL_Cursor * SurfaceToCursor(SDL_Surface *image, int hx, int hy);
	Uint32 SDL_GetPixel(SDL_Surface *surface, const int & x, const int & y, bool colorByte = false);
	SDL_Color SDL_GetPixelColor(SDL_Surface *surface, int x, int y);
	SDL_Surface * alphaTransform(SDL_Surface * src); //adds transparency and shadows (partial handling only; see examples of using for details)
	int blit8bppAlphaTo24bpp(SDL_Surface * src, SDL_Rect * srcRect, SDL_Surface * dst, SDL_Rect * dstRect); //blits 8 bpp surface with alpha channel to 24 bpp surface
	Uint32 colorToUint32(const SDL_Color * color); //little endian only
	void printTo(const std::string & text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=screen, unsigned char quality = 2);// quality: 0 - lowest, 1 - medium, 2 - highest; prints at right bottom corner of specific area. position of corner indicated by (x, y)
	void printToWR(const std::string & text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=screen, unsigned char quality = 2);// quality: 0 - lowest, 1 - medium, 2 - highest; prints at right bottom corner of specific area. position of corner indicated by (x, y)
	void printAtMiddle(const std::string & text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=screen, unsigned char quality = 2, bool refresh = false); // quality: 0 - lowest, 1 - medium, 2 - highest
	void printAtMiddleWB(const std::string & text, int x, int y, TTF_Font * font, int charpr, SDL_Color kolor=tytulowy, SDL_Surface * dst=screen);
	void printAtWB(const std::string & text, int x, int y, TTF_Font * font, int charpr, SDL_Color kolor=tytulowy, SDL_Surface * dst=screen);
	void printAt(const std::string & text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=screen, unsigned char quality = 2); // quality: 0 - lowest, 1 - medium, 2 - highest
	void update(SDL_Surface * what = screen); //updates whole surface (default - main screen)
	void drawBorder(SDL_Surface * sur, int x, int y, int w, int h, int3 color);
	void setPlayerColor(SDL_Surface * sur, unsigned char player); //sets correct color of flags; -1 for neutral
	std::string processStr(std::string str, std::vector<std::string> & tor); //replaces %s in string
	SDL_Surface * newSurface(int w, int h, SDL_Surface * mod=screen); //creates new surface, with flags/format same as in surface given
	SDL_Surface * copySurface(SDL_Surface * mod); //returns copy of given surface
};

#endif // SDL_EXTENSIONS_H
