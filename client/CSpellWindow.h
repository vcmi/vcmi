#pragma once
#include "global.h"
#include "CPlayerInterface.h"

struct SDL_Surface;
class CDefHandler;
struct SDL_Rect;
class CGHeroInstance;

class SpellbookInteractiveArea : public ClickableL, public ClickableR, public Hoverable
{
private:
	boost::function<void()> onLeft;
	std::string textOnRclick;
	boost::function<void()> onHoverOn;
	boost::function<void()> onHoverOff;
public:
	void clickLeft(boost::logic::tribool down);
	void clickRight(boost::logic::tribool down);
	void hover(bool on);
	void activate();
	void deactivate();

	SpellbookInteractiveArea(SDL_Rect & myRect, boost::function<void()> funcL, std::string textR, boost::function<void()> funcHon, boost::function<void()> funcHoff);//c-tor
};

class CSpellWindow : public IShowActivable, public CIntObject
{
private:
	SDL_Surface * background, * leftCorner, * rightCorner;
	CDefHandler * spells, //pictures of spells
		* spellTab, //school select
		* schools, //schools' pictures
		* schoolW, * schoolE, * schoolF, * schoolA; //schools' 'borders'

	SpellbookInteractiveArea * exitBtn, * battleSpells, * adventureSpells, * manaPoints;
	SpellbookInteractiveArea * selectSpellsA, * selectSpellsE, * selectSpellsF, * selectSpellsW, * selectSpellsAll;
	CStatusBar * statusBar;
	Uint8 selectedTab; // 0 - air magic, 1 - fire magic, 2 - water magic, 3 - earth magic, 4 - all schools
public:
	CSpellWindow(const SDL_Rect & myRect, const CGHeroInstance * myHero); //c-tor
	~CSpellWindow(); //d-tor

	void fexitb();
	void fadvSpellsb();
	void fbattleSpellsb();
	void fmanaPtsb();

	void fspellsAb();
	void fspellsEb();
	void fspellsFb();
	void fspellsWb();
	void fspellsAllb();

	void activate();
	void deactivate();
	void show(SDL_Surface * to = NULL);
};
