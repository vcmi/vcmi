#pragma once

#include "../lib/HeroBonus.h"


//#include "CPlayerInterface.h"

/*
 * CHeroWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CAdventureMapButton;
struct SDL_Surface;
class CGHeroInstance;
class CDefHandler;
class CArtifact;
class CHeroWindow;
class LClickableAreaHero;
class LRClickableAreaWText;
class LRClickableAreaWTextComp;
class CArtifactsOfHero;

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
	const TBonusListPtr getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = NULL, const std::string &cachingStr = "") const OVERRIDE;
};

class CHeroWindow: public CWindowWithGarrison, public CWindowWithArtifacts
{
	CPicture *background;
	CGStatusBar * ourBar; //heroWindow's statusBar

	//buttons
	//CAdventureMapButton * gar4button; //splitting
	std::vector<CHeroSwitcher *> heroList; //list of heroes
	CPicture * listSelection; //selection border

	//clickable areas
	LRClickableAreaWText * portraitArea;
	CAnimImage * portraitImage;

	std::vector<LRClickableAreaWTextComp *> primSkillAreas;
	LRClickableAreaWText * expArea;
	LRClickableAreaWText * spellPointsArea;
	LRClickableAreaWText * specArea;//speciality
	CAnimImage *specImage;
	MoraleLuckBox * morale, * luck;
	std::vector<LRClickableAreaWTextComp *> secSkillAreas;
	std::vector<CAnimImage *> secSkillImages;
	CHeroWithMaybePickedArtifact heroWArt;

	CAdventureMapButton * quitButton, * dismissButton, * questlogButton; //general
		
	CHighlightableButton *tacticsButton; //garrison / formation handling;
	CHighlightableButtonsGroup *formations;

public:
	const CGHeroInstance * curHero;

	CHeroWindow(const CGHeroInstance *hero); //c-tor

	void update(const CGHeroInstance * hero, bool redrawNeeded = false); //sets main displayed hero
	void showAll(SDL_Surface * to);

	void quit(); //stops displaying hero window and disposes
	void dismissCurrent(); //dissmissed currently displayed hero (curHero)
	void questlog(); //show quest log in hero window
	void switchHero(); //changes displayed hero

	//friends
	friend void CArtPlace::clickLeft(tribool down, bool previousState);
	friend class CPlayerInterface;
};
