#pragma once
#include "global.h"
#include "SDL.h"
#include "CPlayerInterface.h"
class CGTownInstance;
template <typename T> class AdventureMapButton;
class CBuildingRect : public MotionInterested, public ClickableL, public ClickableR, public TimeInterested
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

	CDefHandler *hall,*fort;

	AdventureMapButton<CCastleInterface> * exit;

	CCastleInterface(const CGTownInstance * Town, bool Activate=true);
	~CCastleInterface();
	void show();
	void close();
	void activate();
	void deactivate();
};