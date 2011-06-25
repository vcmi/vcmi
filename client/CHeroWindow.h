#ifndef __CHEROWINDOW_H__
#define __CHEROWINDOW_H__
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

class AdventureMapButton;
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
public:
	int id;

	CHeroWindow * getOwner();
	virtual void clickLeft(tribool down, bool previousState);

	CHeroSwitcher(int serial);
};

//helper class for calculating values of hero bonuses without bonuses from picked up artifact
class CHeroWithMaybePickedArtifact : public IBonusBearer
{
public:
	const CGHeroInstance *hero;
	CWindowWithArtifacts *cww;

	CHeroWithMaybePickedArtifact(CWindowWithArtifacts *Cww, const CGHeroInstance *Hero);
	const boost::shared_ptr<BonusList> getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = NULL) const OVERRIDE;
};

class CHeroWindow: public CWindowWithGarrison, public CWindowWithArtifacts
{
	enum ELabel {};
	std::map<ELabel, CLabel*> labels;
	CPicture *background;
	CGStatusBar * ourBar; //heroWindow's statusBar

	//general graphics
	CDefEssential *flags;

	//buttons
	//AdventureMapButton * gar4button; //splitting
	std::vector<CHeroSwitcher *> heroListMi; //new better list of heroes

	//clickable areas
	LRClickableAreaWText * portraitArea;
	std::vector<LRClickableAreaWTextComp *> primSkillAreas;
	LRClickableAreaWText * expArea;
	LRClickableAreaWText * spellPointsArea;
	LRClickableAreaWText * specArea;//speciality
	MoraleLuckBox * morale, * luck;
	std::vector<LRClickableAreaWTextComp *> secSkillAreas;
	CHeroWithMaybePickedArtifact heroWArt;

public:
	const CGHeroInstance * curHero;
	AdventureMapButton * quitButton, * dismissButton, * questlogButton; //general
		
	CHighlightableButton *tacticsButton; //garrison / formation handling;
	CHighlightableButtonsGroup *formations;
	int player;
	CHeroWindow(const CGHeroInstance *hero); //c-tor
	~CHeroWindow(); //d-tor

	void update(const CGHeroInstance * hero, bool redrawNeeded = false); //sets main displayed hero
	void showAll(SDL_Surface * to); //shows and activates adv. map interface
//		void redrawCurBack(); //redraws curBAck from scratch
		void quit(); //stops displaying hero window and disposes
	void dismissCurrent(); //dissmissed currently displayed hero (curHero)
	void questlog(); //show quest log in hero window
		void switchHero(); //changes displayed hero

	//friends
	friend void CArtPlace::clickLeft(tribool down, bool previousState);
	friend class CPlayerInterface;
};

#endif // __CHEROWINDOW_H__
