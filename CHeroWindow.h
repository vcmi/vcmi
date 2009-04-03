#ifndef __CHEROWINDOW_H__
#define __CHEROWINDOW_H__


#include "CPlayerInterface.h"

class AdventureMapButton;
struct SDL_Surface;
class CGHeroInstance;
class CDefHandler;
class CArtifact;
class CHeroWindow;

class LClickableArea: public ClickableL
{
public:
	virtual void clickLeft (tribool down);
	virtual void activate();
	virtual void deactivate();
};

class RClickableArea: public ClickableR
{
public:
	virtual void clickRight (tribool down);
	virtual void activate();
	virtual void deactivate();
};

class LClickableAreaHero : public LClickableArea
{
public:
	int id;
	CHeroWindow * owner;
	virtual void clickLeft (tribool down);
};

class LRClickableAreaWText: public LClickableArea, public RClickableArea, public Hoverable
{
public:
	std::string text, hoverText;
	virtual void activate();
	virtual void deactivate();
	virtual void clickLeft (tribool down);
	virtual void clickRight (tribool down);
	virtual void hover(bool on);
};

class LRClickableAreaWTextComp: public LClickableArea, public RClickableArea, public Hoverable
{
public:
	std::string text, hoverText;
	int baseType;
	int bonus, type;
	virtual void activate();
	virtual void deactivate();
	virtual void clickLeft (tribool down);
	virtual void clickRight (tribool down);
	virtual void hover(bool on);
};

class CArtPlace: public IShowable, public LRClickableAreaWTextComp
{
private:
	bool active;
public:
	//bool spellBook, warMachine1, warMachine2, warMachine3, warMachine4,
	//	misc1, misc2, misc3, misc4, misc5, feet, lRing, rRing, torso,
	//	lHand, rHand, neck, shoulders, head; //my types
	ui16 slotID; //0   	head	1 	shoulders		2 	neck		3 	right hand		4 	left hand		5 	torso		6 	right ring		7 	left ring		8 	feet		9 	misc. slot 1		10 	misc. slot 2		11 	misc. slot 3		12 	misc. slot 4		13 	ballista (war machine 1)		14 	ammo cart (war machine 2)		15 	first aid tent (war machine 3)		16 	catapult		17 	spell book		18 	misc. slot 5		19+ 	backpack slots

	bool clicked;
	CHeroWindow * ourWindow;
	const CArtifact * ourArt;
	CArtPlace(const CArtifact * Art);
	void clickLeft (tribool down);
	void clickRight (tribool down);
	void activate();
	void deactivate();
	void show(SDL_Surface * to = NULL);
	bool fitsHere(const CArtifact * art); //returns true if given artifact can be placed here
	~CArtPlace();
};

class CHeroWindow: public IShowActivable, public virtual CIntObject
{
	SDL_Surface * background, * curBack;
	const CGHeroInstance * curHero;
	CGarrisonInt * garInt;
	CStatusBar * ourBar; //heroWindow's statusBar

	//general graphics
	CDefHandler *flags;

	//buttons
	AdventureMapButton * gar4button; //splitting
	std::vector<LClickableAreaHero *> heroListMi; //new better list of heroes

	std::vector<CArtPlace *> artWorn; // 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5
	std::vector<CArtPlace *> backpack; //hero's visible backpack (only 5 elements!)
	int backpackPos; //unmber of first art visible in backpack (in hero's vector)
	CArtPlace * activeArtPlace;
	//clickable areas
	LRClickableAreaWText * portraitArea;
	std::vector<LRClickableAreaWTextComp *> primSkillAreas;
	LRClickableAreaWText * expArea;
	LRClickableAreaWText * spellPointsArea;
	LRClickableAreaWTextComp * luck;
	LRClickableAreaWTextComp * morale;
	std::vector<LRClickableAreaWTextComp *> secSkillAreas;
public:
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
	virtual void show(SDL_Surface * to = NULL); //shows hero window
	void redrawCurBack(); //redraws curBAck from scratch
	void quit(); //stops displaying hero window
	void dismissCurrent(); //dissmissed currently displayed hero (curHero)
	void questlog(); //show quest log in hero window
	void scrollBackpack(int dir); //dir==-1 => to left; dir==-2 => to right
	void switchHero(); //changes displayed hero

	//friends
	friend void CArtPlace::clickLeft(tribool down);
	friend class CPlayerInterface;
};

#endif // __CHEROWINDOW_H__
