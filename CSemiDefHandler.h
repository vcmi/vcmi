#ifndef SEMIDEF_H
#define SEMIDEF_H
#include "global.h"
#include <string>
#include "SDL.h"
#include "SDL_image.h"
#include <vector>

struct Cimage
{
	int groupNumber;
	std::string imName; //name without extension
	SDL_Surface * bitmap;
};
class CSemiDefHandler
{
public:
	int howManyImgs;	
	std::string defName;
	std::vector<Cimage> ourImages;
	std::vector<std::string> namesOfImgs;
	unsigned char * buforD;

	static std::string nameFromType(EterrainType typ);
	void openImg(const char *name);
	void openDef(std::string name, std::string lodName, int dist=1);
	void readFileList(int dist = 1);
	void loadImages(std::string path);
	~CSemiDefHandler();
};
#endif // SEMIDEF_H