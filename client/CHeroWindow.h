#ifndef __CHEROWINDOW_H__
#define __CHEROWINDOW_H__


#include "CPlayerInterface.h"

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



class CHeroWindow: public CWindowWithGarrison, public virtual CIntObject
{
	SDL_Surface * background, * curBack;
	CStatusBar * ourBar; //heroWindow's statusBar

	//general graphics
	CDefEssential *flags;

	//buttons
	AdventureMapButton * gar4button; //splitting
	std::vector<LClickableAreaHero *> heroListMi; //new better list of heroes

	CArtifactsOfHero * artifs;

	//clickable areas
	LRClickableAreaWText * portraitArea;
	std::vector<LRClickableAreaWTextComp *> primSkillAreas;
	LRClickableAreaWText * expArea;
	LRClickableAreaWText * spellPointsArea;
	LRClickableAreaWTextComp * luck;
	LRClickableAreaWTextComp * morale;
	std::vector<LRClickableAreaWTextComp *> secSkillAreas;
public:
	const CGHeroInstance * curHero;
	AdventureMapButton * quitButton, * dismissButton, * questlogButton, //general
		* leftArtRoll, * rightArtRoll;
	CHighlightableButton *gar2button; //garrison / formation handling;
	CHighlightableButtonsGroup *formations;
	int player;
	CHeroWindow(int playerColor); //c-tor
	~CHeroWindow(); //d-tor
	void setHero(const CGHeroInstance * Hero); //sets main displayed hero
	void activate(); //activates hero window;
	void deactivate(); //activates hero window;
	virtual void show(SDL_Surface * to); //shows hero window
	void redrawCurBack(); //redraws curBAck from scratch
	void dispose(); //free resources not needed after closing windows and reset state
	void quit(); //stops displaying hero window and disposes
	void dismissCurrent(); //dissmissed currently displayed hero (curHero)
	void questlog(); //show quest log in hero window
	void scrollBackpack(int dir); //dir==-1 => to left; dir==-2 => to right
	void switchHero(); //changes displayed hero

	//friends
	friend void CArtPlace::clickLeft(tribool down);
	friend class CPlayerInterface;
};

#endif // __CHEROWINDOW_H__
