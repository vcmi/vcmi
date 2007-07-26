#ifndef CTOWNHANDLER_H
#define CTOWNHANDLER_H
#include "CDefHandler.h"
#include "SDL.h"
#include <string>
#include <vector>
class CTown
{
public:
	std::string name;
	int bonus; //pic number
};
class CTownHandler
{
	CDefHandler * smallIcons;
public:
	CTownHandler();
	~CTownHandler();
	std::vector<CTown> towns;
	void loadNames();
	SDL_Surface * getPic(int ID, bool fort=true, bool builded=false); //ID=-1 - blank; -2 - border; -3 - random
};

#endif //CTOWNHANDLER_H