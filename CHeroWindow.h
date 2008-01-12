#pragma once
#include "CPlayerInterface.h"

template <typename T> class AdventureMapButton;
class SDL_Surface;
class CGHeroInstance;
class CDefHandler;

class CHeroWindow: public IShowable, public virtual CIntObject
{
	SDL_Surface * background, * curBack;
	const CGHeroInstance * curHero;

	//general graphics
	CDefHandler * skillpics;

	//buttons
	AdventureMapButton<CHeroWindow> * quitButton, * dismissButton, * questlogButton;
public:
	CHeroWindow(int playerColor); //c-tor
	~CHeroWindow(); //d-tor
	void setHero(const CGHeroInstance * hero); //sets main displayed hero
	void activate(); //activates hero window;
	virtual void show(SDL_Surface * to = NULL); //shows hero window
	void quit(); //stops displaying hero window
	void dismissCurrent(); //dissmissed currently displayed hero (curHero) //TODO: make it working
	void questlog(); //show quest log in 
};