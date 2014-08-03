#pragma once

#include "../../lib/HeroBonus.h"
#include "../widgets/CArtifactHolder.h"
#include "../widgets/CGarrisonInt.h"

/*
 * CHeroWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CButton;
struct SDL_Surface;
class CGHeroInstance;
class CDefHandler;
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
	virtual void clickLeft(tribool down, bool previousState);

	CHeroSwitcher(Point pos, const CGHeroInstance * hero);
};

//helper class for calculating values of hero bonuses without bonuses from picked up artifact
class CHeroWithMaybePickedArtifact : public IBonusBearer
{
public:
	const CGHeroInstance *hero;
	CWindowWithArtifacts *cww;

	CHeroWithMaybePickedArtifact(CWindowWithArtifacts *Cww, const CGHeroInstance *Hero);
	const TBonusListPtr getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = nullptr, const std::string &cachingStr = "") const override;
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

	CHeroWindow(const CGHeroInstance *hero); //c-tor

	void update(const CGHeroInstance * hero, bool redrawNeeded = false); //sets main displayed hero
	void showAll(SDL_Surface * to);

	void dismissCurrent(); //dissmissed currently displayed hero (curHero)
	void questlog(); //show quest log in hero window
	void commanderWindow();
	void switchHero(); //changes displayed hero

	//friends
	friend void CArtPlace::clickLeft(tribool down, bool previousState);
	friend class CPlayerInterface;
};
