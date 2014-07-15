#pragma once

#include "CWindowObject.h"

/*
 * CSpellWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct SDL_Surface;
class CDefHandler;
struct SDL_Rect;
class CGHeroInstance;
class CGStatusBar;
class CPlayerInterface;

/// Spellbook button is used by the spell window class
class SpellbookInteractiveArea : public CIntObject
{
private:
	std::function<void()> onLeft;
	std::string textOnRclick;
	std::function<void()> onHoverOn;
	std::function<void()> onHoverOff;
	CPlayerInterface * myInt;
public:
	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	void hover(bool on);

	SpellbookInteractiveArea(const SDL_Rect & myRect, std::function<void()> funcL, const std::string & textR,
		std::function<void()> funcHon, std::function<void()> funcHoff, CPlayerInterface * _myInt);//c-tor
};

/// The spell window
class CSpellWindow : public CWindowObject
{
private:
	class SpellArea : public CIntObject
	{
	public:
		SpellID mySpell;
		int schoolLevel; //range: 0 none, 3 - expert
		int whichSchool; //0 - air magic, 1 - fire magic, 2 - water magic, 3 - earth magic,
		int spellCost;
		CSpellWindow * owner;


		SpellArea(SDL_Rect pos, CSpellWindow * owner);

		void setSpell(SpellID spellID);

		void clickLeft(tribool down, bool previousState);
		void clickRight(tribool down, bool previousState);
		void hover(bool on);
		void showAll(SDL_Surface * to);
	};

	SDL_Surface * leftCorner, * rightCorner;
	
	CAnimation * spells; //pictures of spells
	
	CDefHandler	* spellTab, //school select
		* schools, //schools' pictures
		* schoolBorders [4]; //schools' 'borders': [0]: air, [1]: fire, [2]: water, [3]: earth

	SpellbookInteractiveArea * exitBtn, * battleSpells, * adventureSpells, * manaPoints;
	SpellbookInteractiveArea * selectSpellsA, * selectSpellsE, * selectSpellsF, * selectSpellsW, * selectSpellsAll;
	SpellbookInteractiveArea * lCorner, * rCorner;
	SpellArea * spellAreas[12];
	CGStatusBar * statusBar;

	Uint8 sitesPerTabAdv[5];
	Uint8 sitesPerTabBattle[5];

	bool battleSpellsOnly; //if true, only battle spells are displayed; if false, only adventure map spells are displayed
	Uint8 selectedTab; // 0 - air magic, 1 - fire magic, 2 - water magic, 3 - earth magic, 4 - all schools
	Uint8 currentPage; //changes when corners are clicked
	std::set<SpellID> mySpells; //all spels in this spellbook

	const CGHeroInstance * myHero; //hero whose spells are presented

	void computeSpellsPerArea(); //recalculates spellAreas::mySpell

	void turnPageLeft();
	void turnPageRight();

	CPlayerInterface * myInt;

public:

	CSpellWindow(const SDL_Rect & myRect, const CGHeroInstance * _myHero, CPlayerInterface * _myInt, bool openOnBattleSpells = true); //c-tor
	~CSpellWindow(); //d-tor

	void fexitb();
	void fadvSpellsb();
	void fbattleSpellsb();
	void fmanaPtsb();

	void fLcornerb();
	void fRcornerb();

	void selectSchool(int school); //schools: 0 - air magic, 1 - fire magic, 2 - water magic, 3 - earth magic, 4 - all schools
	Uint8 pagesWithinCurrentTab();
	void keyPressed(const SDL_KeyboardEvent & key);
	void activate();
	void deactivate();
	void showAll(SDL_Surface * to);
	void show(SDL_Surface * to);

	void teleportTo(int town, const CGHeroInstance * hero);
};
