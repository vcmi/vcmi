#pragma once
#include "CPlayerInterface.h"

class SDL_Surface;
class CGHeroInstance;

class CHeroWindow: public IShowable, public virtual CIntObject
{
	SDL_Surface * background;
	const CGHeroInstance * curHero;
public:
	CHeroWindow(); //c-tor
	~CHeroWindow(); //d-tor
	void setHero(const CGHeroInstance * hero); //sets main displayed hero
	virtual void show(SDL_Surface * to = NULL); //shows hero window
};