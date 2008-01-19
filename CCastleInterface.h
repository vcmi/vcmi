#pragma once
#include "global.h"
#include "SDL.h"
#include "CPlayerInterface.h"
//#include "boost/tuple/tuple.hpp"
class CGTownInstance;
class CTownHandler;
struct Structure;
template <typename T> class AdventureMapButton;
class CBuildingRect : public MotionInterested, public ClickableL, public ClickableR//, public TimeInterested
{
public:
	Structure* str;
	CDefHandler* def;
	SDL_Surface* border;
	SDL_Surface* area;
	CBuildingRect(Structure *Str);
	~CBuildingRect();
	void activate();
	void deactivate();
	bool operator<(const CBuildingRect & p2) const;
	void mouseMoved (SDL_MouseMotionEvent & sEvent);
	void clickLeft (tribool down);
	void clickRight (tribool down);
};

class CCastleInterface : public IShowable
{
public:
	SDL_Surface * townInt;
	SDL_Surface * cityBg;
	const CGTownInstance * town;

	unsigned char animval, count;

	CDefHandler *hall,*fort,
		*bigTownPic, *flag;

	AdventureMapButton<CCastleInterface> * exit;

	std::vector<CBuildingRect*> buildings; //building id, building def, structure struct, border, filling

	CCastleInterface(const CGTownInstance * Town, bool Activate=true);
	~CCastleInterface();
	void show(SDL_Surface * to=NULL);
	void showAll(SDL_Surface * to=NULL);
	void close();
	void activate();
	void deactivate();
};