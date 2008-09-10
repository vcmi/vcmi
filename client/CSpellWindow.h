#pragma once
#include "global.h"
#include "CPlayerInterface.h"

struct SDL_Surface;
class CDefHandler;
struct SDL_Rect;
class CGHeroInstance;

class ClickableArea : public ClickableL
{
private:
	boost::function<void()> myFunc;
public:
	void clickLeft(boost::logic::tribool down);
	void activate();
	void deactivate();

	ClickableArea(SDL_Rect & myRect, boost::function<void()> func);//c-tor
};

class CSpellWindow : public IShowActivable, public CIntObject
{
private:
	SDL_Surface * background, * leftCorner, * rightCorner;
	CDefHandler * spells, //pictures of spells
		* spellTab, //school select
		* schools, //schools' pictures
		* schoolW, * schoolE, * schoolF, * schoolA; //schools' 'borders'

	ClickableArea * exitBtn;
	CStatusBar * statusBar;
public:
	Uint8 selectedTab;
	CSpellWindow(const SDL_Rect & myRect, const CGHeroInstance * myHero); //c-tor
	~CSpellWindow(); //d-tor

	void fexitb();

	void activate();
	void deactivate();
	void show(SDL_Surface * to = NULL);
};
