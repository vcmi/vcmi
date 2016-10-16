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
class IImage;
class CGHeroInstance;
class CGStatusBar;
class CPlayerInterface;
class CSpellWindow;
class CSpell;

/// The spell window
class CSpellWindow : public CWindowObject
{
private:
	class SpellArea : public CIntObject
	{
	public:
		const CSpell * mySpell;
		int schoolLevel; //range: 0 none, 3 - expert
		int whichSchool; //0 - air magic, 1 - fire magic, 2 - water magic, 3 - earth magic,
		int spellCost;
		CSpellWindow * owner;
		IImage * icon;

		SpellArea(SDL_Rect pos, CSpellWindow * owner);

		void setSpell(const CSpell * spell);

		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
		void hover(bool on) override;
		void showAll(SDL_Surface * to) override;
	};

	class InteractiveArea : public CIntObject
	{
	private:
		std::function<void()> onLeft;
		CSpellWindow * owner;

		std::string hoverText;
		std::string helpText;
	public:
		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
		void hover(bool on) override;

		InteractiveArea(const SDL_Rect & myRect, std::function<void()> funcL, int helpTextId, CSpellWindow * _owner);//c-tor
	};

	SDL_Surface * leftCorner, * rightCorner;

	SDL_Rect lCorner, rCorner;

	CAnimation * spells; //pictures of spells

	CDefHandler	* spellTab, //school select
		* schools, //schools' pictures
		* schoolBorders [4]; //schools' 'borders': [0]: air, [1]: fire, [2]: water, [3]: earth

	SpellArea * spellAreas[12];
	CGStatusBar * statusBar;

	int sitesPerTabAdv[5];
	int sitesPerTabBattle[5];

	bool battleSpellsOnly; //if true, only battle spells are displayed; if false, only adventure map spells are displayed
	Uint8 selectedTab; // 0 - air magic, 1 - fire magic, 2 - water magic, 3 - earth magic, 4 - all schools
	int currentPage; //changes when corners are clicked
	std::vector<const CSpell *> mySpells; //all spels in this spellbook

	const CGHeroInstance * myHero; //hero whose spells are presented

	void computeSpellsPerArea(); //recalculates spellAreas::mySpell

	void turnPageLeft();
	void turnPageRight();

	CPlayerInterface * myInt;

public:

	CSpellWindow(const CGHeroInstance * _myHero, CPlayerInterface * _myInt, bool openOnBattleSpells = true); //c-tor
	~CSpellWindow(); //d-tor

	void fexitb();
	void fadvSpellsb();
	void fbattleSpellsb();
	void fmanaPtsb();

	void fLcornerb();
	void fRcornerb();

	void selectSchool(int school); //schools: 0 - air magic, 1 - fire magic, 2 - water magic, 3 - earth magic, 4 - all schools
	int pagesWithinCurrentTab();
	void keyPressed(const SDL_KeyboardEvent & key);
	void showAll(SDL_Surface * to);
	void show(SDL_Surface * to);
};
