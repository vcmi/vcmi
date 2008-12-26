#ifndef __CBITMAPHANDLER_H__
#define __CBITMAPHANDLER_H__



#include "../global.h"
struct SDL_Surface;
class CLodHandler;

enum Epcxformat {PCX8B, PCX24B};

struct BMPPalette
{
	unsigned char R,G,B,F;
};
struct BMPHeader
{
	int fullSize, _h1, _h2, _h3, _c1, _c2, _c3, _c4, x, y,
		dataSize1, dataSize2; //DataSize=X*Y+2*Y
	unsigned char _c5[8];
	void print(std::ostream & out);
	BMPHeader(){_h1=_h2=0;for(int i=0;i<8;i++)_c5[i]=0;};
};
class CPCXConv
{	
public:
	unsigned char * pcx, *bmp;
	int pcxs, bmps;
	void fromFile(std::string path);
	void saveBMP(std::string path);
	void openPCX(char * PCX, int len);
	void openPCX();
	void convert();
	SDL_Surface * getSurface(); //for standard H3 PCX
	//SDL_Surface * getSurfaceZ(); //for ZSoft PCX
	CPCXConv(){pcx=bmp=NULL;pcxs=bmps=0;};
	~CPCXConv(){if (pcxs) delete[] pcx; if(bmps) delete[] bmp;}
};
namespace BitmapHandler
{
	extern CLodHandler *bitmaph;
	SDL_Surface * loadBitmap(std::string fname, bool setKey=false);
};

#endif // __CBITMAPHANDLER_H__
