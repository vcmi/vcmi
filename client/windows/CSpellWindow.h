/*
 * CSpellWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CWindowObject.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CSpell;

VCMI_LIB_NAMESPACE_END

class IImage;
class CAnimImage;
class CPicture;
class CLabel;
class CGStatusBar;
class CPlayerInterface;
class CSpellWindow;

/// The spell window
class CSpellWindow : public CWindowObject
{
	class SpellArea : public CIntObject
	{
		const CSpell * mySpell;
		int schoolLevel; //range: 0 none, 3 - expert
		CSpellWindow * owner;
		std::shared_ptr<CAnimImage> image;
		std::shared_ptr<CAnimImage> schoolBorder;
		std::shared_ptr<CLabel> name;
		std::shared_ptr<CLabel> level;
		std::shared_ptr<CLabel> cost;
	public:
		SpellArea(Rect pos, CSpellWindow * owner);
		~SpellArea();
		void setSpell(const CSpell * spell);

		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
		void hover(bool on) override;
	};

	class InteractiveArea : public CIntObject
	{
		std::function<void()> onLeft;
		CSpellWindow * owner;

		std::string hoverText;
		std::string helpText;
	public:
		void clickLeft(tribool down, bool previousState) override;
		void clickRight(tribool down, bool previousState) override;
		void hover(bool on) override;

		InteractiveArea(const Rect &myRect, std::function<void()> funcL, int helpTextId, CSpellWindow * _owner);
	};

	std::shared_ptr<CAnimation> spellIcons;
	std::array<std::shared_ptr<CAnimation>, 4> schoolBorders; //[0]: air, [1]: fire, [2]: water, [3]: earth

	std::shared_ptr<CPicture> leftCorner;
	std::shared_ptr<CPicture> rightCorner;

	std::shared_ptr<CAnimImage> schoolTab;
	std::shared_ptr<CAnimImage> schoolPicture;

	std::array<std::shared_ptr<SpellArea>, 12> spellAreas;
	std::shared_ptr<CLabel> mana;
	std::shared_ptr<CGStatusBar> statusBar;

	std::vector<std::shared_ptr<InteractiveArea>> interactiveAreas;

	int sitesPerTabAdv[5];
	int sitesPerTabBattle[5];

	bool battleSpellsOnly; //if true, only battle spells are displayed; if false, only adventure map spells are displayed
	uint8_t selectedTab; // 0 - air magic, 1 - fire magic, 2 - water magic, 3 - earth magic, 4 - all schools
	int currentPage; //changes when corners are clicked
	std::vector<const CSpell *> mySpells; //all spels in this spellbook

	const CGHeroInstance * myHero; //hero whose spells are presented
	CPlayerInterface * myInt;

	void computeSpellsPerArea(); //recalculates spellAreas::mySpell

	void setCurrentPage(int value);
	void turnPageLeft();
	void turnPageRight();

public:
	CSpellWindow(const CGHeroInstance * _myHero, CPlayerInterface * _myInt, bool openOnBattleSpells = true);
	~CSpellWindow();

	void fexitb();
	void fadvSpellsb();
	void fbattleSpellsb();
	void fmanaPtsb();

	void fLcornerb();
	void fRcornerb();

	void selectSchool(int school); //schools: 0 - air magic, 1 - fire magic, 2 - water magic, 3 - earth magic, 4 - all schools
	int pagesWithinCurrentTab();

	void keyPressed(EShortcut key) override;

	void show(Canvas & to) override;
};
