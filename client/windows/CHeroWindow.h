/*
 * CHeroWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/HeroBonus.h"
#include "../widgets/CArtifactHolder.h"
#include "../widgets/CGarrisonInt.h"

class CButton;
struct SDL_Surface;
class CGHeroInstance;
class CArtifact;
class CHeroWindow;
class LClickableAreaHero;
class LRClickableAreaWText;
class LRClickableAreaWTextComp;
class CArtifactsOfHero;
class MoraleLuckBox;
class CToggleButton;
class CToggleGroup;
class CGStatusBar;

/// Button which switches hero selection
class CHeroSwitcher : public CIntObject
{
	const CGHeroInstance * hero;
	CAnimImage *image;
public:
	virtual void clickLeft(tribool down, bool previousState) override;

	CHeroSwitcher(Point pos, const CGHeroInstance * hero);
};

//helper class for calculating values of hero bonuses without bonuses from picked up artifact
class CHeroWithMaybePickedArtifact : public virtual IBonusBearer
{
public:
	const CGHeroInstance *hero;
	CWindowWithArtifacts *cww;

	CHeroWithMaybePickedArtifact(CWindowWithArtifacts *Cww, const CGHeroInstance *Hero);
	const TBonusListPtr getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = nullptr, const std::string &cachingStr = "") const override;

	int64_t getTreeVersion() const override;
};

class CHeroWindow: public CWindowObject, public CWindowWithGarrison, public CWindowWithArtifacts
{
	CGStatusBar * ourBar; //heroWindow's statusBar

	//buttons
	//CButton * gar4button; //splitting
	std::vector<CHeroSwitcher *> heroList; //list of heroes
	CPicture * listSelection; //selection border

	//clickable areas
	LRClickableAreaWText * portraitArea;
	CAnimImage * portraitImage;

	std::vector<LRClickableAreaWTextComp *> primSkillAreas;
	LRClickableAreaWText * expArea;
	LRClickableAreaWText * spellPointsArea;
	LRClickableAreaWText * specArea;//specialty
	CAnimImage *specImage;
	MoraleLuckBox * morale, * luck;
	std::vector<LRClickableAreaWTextComp *> secSkillAreas;
	std::vector<CAnimImage *> secSkillImages;
	CHeroWithMaybePickedArtifact heroWArt;

	CButton * quitButton, * dismissButton, * questlogButton, * commanderButton; //general

	CToggleButton *tacticsButton; //garrison / formation handling;
	CToggleGroup *formations;

public:
	const CGHeroInstance * curHero;

	CHeroWindow(const CGHeroInstance *hero);

	void update(const CGHeroInstance * hero, bool redrawNeeded = false); //sets main displayed hero
	void showAll(SDL_Surface * to) override;

	void dismissCurrent(); //dissmissed currently displayed hero (curHero)
	void commanderWindow();
	void switchHero(); //changes displayed hero
	virtual void updateGarrisons() override;  //updates the morale widget and calls the parent

	//friends
	friend void CHeroArtPlace::clickLeft(tribool down, bool previousState);
	friend class CPlayerInterface;
};
