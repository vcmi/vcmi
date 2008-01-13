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
	int player;

	//general graphics
	CDefHandler * skillpics, *flags;

	//buttons
	AdventureMapButton<CHeroWindow> * quitButton, * dismissButton, * questlogButton, //general
		* gar1button, * gar2button, * gar3button, * gar4button; //garrison / formation handling
public:
	CHeroWindow(int playerColor); //c-tor
	~CHeroWindow(); //d-tor
	void setHero(const CGHeroInstance * hero); //sets main displayed hero
	void activate(); //activates hero window;
	virtual void show(SDL_Surface * to = NULL); //shows hero window
	void quit(); //stops displaying hero window
	void dismissCurrent(); //dissmissed currently displayed hero (curHero) //TODO: make it working
	void questlog(); //show quest log in 
	void gar1(); //garrison / formation handling
	void gar2(); //garrison / formation handling
	void gar3(); //garrison / formation handling
	void gar4(); //garrison / formation handling
};