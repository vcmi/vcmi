#pragma once
#include "global.h"
#include "SDL.h"
#include "CPlayerInterface.h"
#include "boost/tuple/tuple.hpp"
class CGTownInstance;
class CTownHandler;
struct Structure;
template <typename T> class AdventureMapButton;
class CBuildingRect : public MotionInterested, public ClickableL, public ClickableR//, public TimeInterested
{
	
	void activate();
	void deactivate();
};

class CCastleInterface
{
public:
	SDL_Surface * townInt;
	SDL_Surface * cityBg;
	const CGTownInstance * town;

	CDefHandler *hall,*fort,
		*bigTownPic, *flag;

	AdventureMapButton<CCastleInterface> * exit;

	std::vector<boost::tuples::tuple<int,CDefHandler*,Structure*,SDL_Surface*,SDL_Surface*> *> buildings; //building id, building def, structure struct, border, filling

	CCastleInterface(const CGTownInstance * Town, bool Activate=true);
	~CCastleInterface();
	void show();
	void close();
	void activate();
	void deactivate();
};