#ifndef SDL_EXTENSIONS_H
#define SDL_EXTENSIONS_H
#include "SDL.h"
#include "SDL_ttf.h"


extern SDL_Surface * ekran;
extern SDL_Color tytulowy, tlo, zwykly ;
extern TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX, *GEORM;
void blitAtWR(SDL_Surface * src, int x, int y, SDL_Surface * dst=ekran);
void blitAt(SDL_Surface * src, int x, int y, SDL_Surface * dst=ekran);
void blitAtWR(SDL_Surface * src, SDL_Rect pos, SDL_Surface * dst=ekran);
void blitAt(SDL_Surface * src, SDL_Rect pos, SDL_Surface * dst=ekran);
void updateRect (SDL_Rect * rect, SDL_Surface * scr = ekran);
bool isItIn(const SDL_Rect * rect, int x, int y);
template <typename T> int getIndexOf(const std::vector<T> & v, const T & val)
{
	for(int i=0;i<v.size();i++)
		if(v[i]==val)
			return i;
	return -1;
}
SDL_Rect genRect(int hh, int ww, int xx, int yy);
namespace CSDL_Ext
{
	extern SDL_Surface * std32bppSurface;
	void SDL_PutPixel(SDL_Surface *ekran, int x, int y, Uint8 R, Uint8 G, Uint8 B, int myC=0, Uint8 A = 255); //myC influences the start of reading pixels
	void SDL_PutPixelWithoutRefresh(SDL_Surface *ekran, int x, int y, Uint8 R, Uint8 G, Uint8 B, int myC=0, Uint8 A = 255); //myC influences the start of reading pixels ; without refreshing
	SDL_Surface * rotate01(SDL_Surface * toRot, int myC = 2); //vertical flip
	SDL_Surface * hFlip(SDL_Surface * toRot); //horizontal flip
	SDL_Surface * rotate02(SDL_Surface * toRot); //rotate 90 degrees left
	SDL_Surface * rotate03(SDL_Surface * toRot); //rotate 180 degrees
	SDL_Cursor * SurfaceToCursor(SDL_Surface *image, int hx, int hy);
	Uint32 SDL_GetPixel(SDL_Surface *surface, const int & x, const int & y, bool colorByte = false);
	SDL_Color SDL_GetPixelColor(SDL_Surface *surface, int x, int y);
	SDL_Surface * alphaTransform(SDL_Surface * src); //adds transparency and shadows (partial handling only; see examples of using for details)
	SDL_Surface * secondAlphaTransform(SDL_Surface * src, SDL_Surface * alpha); //alpha is a surface we want to blit src to
	void fullAlphaTransform(SDL_Surface *& src); //performs first and second alpha transform
	Uint32 colorToUint32(const SDL_Color * color); //little endian only
	void printTo(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=ekran, unsigned char quality = 2);// quality: 0 - lowest, 1 - medium, 2 - highest; prints at right bottom corner of specific area. position of corner indicated by (x, y)
	void printAtMiddle(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=ekran, unsigned char quality = 2); // quality: 0 - lowest, 1 - medium, 2 - highest
	void printAtMiddleWB(std::string text, int x, int y, TTF_Font * font, int charpr, SDL_Color kolor=tytulowy, SDL_Surface * dst=ekran);
	void printAt(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=ekran, unsigned char quality = 2); // quality: 0 - lowest, 1 - medium, 2 - highest
	void update(SDL_Surface * what = ekran); //updates whole surface (default - main screen)
	void blueToPlayers(SDL_Surface * sur, int player); //simple color substitution
	void blueToPlayersAdv(SDL_Surface * sur, int player, int mode=0, void* additionalInfo=NULL); //substitute blue color by another one, makes it nicer keeping nuances
																							//mode 1 is calibrated for hero infobox
																							//mode 2 is calibrated for resbar and gets in additionalInfo a pointer to the set of (SDL_Color) which shouldn't be replaced
	void blueToPlayersNice(SDL_Surface * sur, int player); //uses interface gems to substitute colours
	void setPlayerColor(SDL_Surface * sur, unsigned char player); //sets correct color of flags; -1 for neutral
	std::string processStr(std::string str, std::vector<std::string> & tor); //replaces %s in string
	SDL_Surface * newSurface(int w, int h, SDL_Surface * mod=ekran); //creates new surface, with flags/format same as in surface given
	SDL_Surface * copySurface(SDL_Surface * mod); //returns copy of given surface
};

#endif // SDL_EXTENSIONS_H