#include "stdafx.h"
#include "CSemiDefHandler.h"
#include <fstream>
extern SDL_Surface * ekran;
std::string CSemiDefHandler::nameFromType (EterrainType typ)
{
	switch(typ)
	{
		case dirt:
		{
			return std::string("DIRTTL.DEF");
			break;
		}
		case sand:
		{
			return std::string("SANDTL.DEF");
			break;
		}
		case grass:
		{
			return std::string("GRASTL.DEF");
			break;
		}
		case snow:
		{
			return std::string("SNOWTL.DEF");
			break;
		}
		case swamp:
		{
			return std::string("SWMPTL.DEF");			
			break;
		}
		case rough:
		{
			return std::string("ROUGTL.DEF");		
			break;
		}
		case subterranean:
		{
			return std::string("SUBBTL.DEF");		
			break;
		}
		case lava:
		{
			return std::string("LAVATL.DEF");		
			break;
		}
		case water:
		{
			return std::string("WATRTL.DEF");
			break;
		}
		case rock:
		{
			return std::string("ROCKTL.DEF");		
			break;
		}
	}
}
void CSemiDefHandler::openDef(std::string name, std::string lodName, int dist)
{
	std::ifstream * is = new std::ifstream();
	is -> open((lodName+"\\"+name).c_str(),std::ios::binary);
	is->seekg(0,std::ios::end); // na koniec
	int andame = is->tellg();  // read length
	is->seekg(0,std::ios::beg); // wracamy na poczatek
	buforD = new unsigned char[andame]; // allocate memory 
	is->read((char*)buforD, andame); // read map file to buffer
	defName = name;
	int gdzie = defName.find_last_of("\\");
	defName = defName.substr(gdzie+1, gdzie-defName.length());
	delete is;

	readFileList(dist);
	loadImages(lodName);

}
void CSemiDefHandler::readFileList(int dist)
{
	howManyImgs = buforD[788];
	int i = 800;
	for (int pom=0;pom<howManyImgs;pom++)
	{
		std::string temp;
		while (buforD[i]!=0)
		{
			temp+=buforD[i++];
		}
		i+=dist; //by³o zwiêkszenie tylko o jedno
		if (temp!="")
		{
			temp = temp.substr(0,temp.length()-4) + ".BMP";
			namesOfImgs.push_back(temp);
		}
		else pom--;
	}
}
void CSemiDefHandler::loadImages(std::string path)
{
	for (int i=0; i<namesOfImgs.size(); i++)
	{
		openImg((path+"\\_"+defName+"\\"+namesOfImgs[i]).c_str());
	}
}
 void SDL_DisplayBitmap(const char *file, SDL_Surface *ekran, int x, int y)
{
	SDL_Surface *image;
	SDL_Rect dest;
	image = SDL_LoadBMP(file);
	if ( image == NULL )
	{
		fprintf(stderr, "Nie mo¿na wczytaæ %s: %s\n", file, SDL_GetError());
		return;
	}
	dest.x = x;
	dest.y = y;
	dest.w = image->w;
	dest.h = image->h;
	SDL_BlitSurface(image, NULL, ekran, &dest);
	SDL_UpdateRects(ekran, 1, &dest);
	SDL_FreeSurface(image);
}
void CSemiDefHandler::openImg(const char *name)
{
	SDL_Surface *image;
	image=IMG_Load(name); 
	//SDL_DisplayBitmap(name,image, 0,0);
	if(!image) 
	{
		printf("IMG_Load: %s\n", IMG_GetError());
		return;
		// handle error
	}
	Cimage vinya;
	vinya.bitmap = image;
	SDL_SetColorKey(vinya.bitmap,SDL_SRCCOLORKEY,SDL_MapRGB(vinya.bitmap->format,0,255,255));
	vinya.imName = name;
	ourImages.push_back(vinya);
}