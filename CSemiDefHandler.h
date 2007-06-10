#ifndef SEMIDEF_H
#define SEMIDEF_H
#include "global.h"
#include <string>
#include "SDL.h"
#include "SDL_image.h"
#include <vector>
struct Cimage
{
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
	void openDef(std::string name, std::string lodName);
	void readFileList();
	void loadImages(std::string path);
};
#endif // SEMIDEF_H