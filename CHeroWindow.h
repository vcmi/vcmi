#pragma once
#include "CPlayerInterface.h"

template <typename T> class AdventureMapButton;
class SDL_Surface;
class CGHeroInstance;

class CHeroWindow: public IShowable, public virtual CIntObject
{
	SDL_Surface * background;
	const CGHeroInstance * curHero;

	//buttons
	AdventureMapButton<CHeroWindow> * quitButton;
public:
	CHeroWindow(int playerColor); //c-tor
	~CHeroWindow(); //d-tor
	void setHero(const CGHeroInstance * hero); //sets main displayed hero
	void activate(); //activates hero window;
	virtual void show(SDL_Surface * to = NULL); //shows hero window
	void quit(); //stops displaying hero window
};