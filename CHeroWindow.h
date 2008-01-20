#pragma once
#include "CPlayerInterface.h"
#include "CAdvmapInterface.h"

template <typename T> class AdventureMapButton;
struct SDL_Surface;
class CGHeroInstance;
class CDefHandler;
class CArtifact;

class LClickableArea: public ClickableL
{
public:
	virtual void clickLeft (tribool down);
	virtual void activate();
	virtual void deactivate();
};

class LClickableAreaWText: public LClickableArea
{
public:
	std::string text;
	virtual void clickLeft (tribool down);
};

class CArtPlace: public ClickableL, public IShowable
{
private:
	bool active;
public:
	const CArtifact * ourArt;
	CArtPlace(CArtifact * art);
	void clickLeft (tribool down);
	void activate();
	void deactivate();
	void show(SDL_Surface * to = NULL);
	~CArtPlace();
};

class CHeroWindow: public IActivable, public IShowable, public virtual CIntObject
{
	SDL_Surface * background, * curBack;
	const CGHeroInstance * curHero;
	int player;

	//general graphics
	CDefHandler * skillpics, *flags;

	//buttons
	AdventureMapButton<CHeroWindow> * quitButton, * dismissButton, * questlogButton, //general
		* gar1button, * gar2button, * gar3button, * gar4button, //garrison / formation handling
		* leftArtRoll, * rightArtRoll;
	std::vector< AdventureMapButton<CHeroWindow> * > heroList; //list of heroes

	//artifact places
	CArtPlace * artHead, * artLRing, * artRRing, * artLHand, * artRhand,
		* artFeet, * artSpellBook, * artMach1, * artMach2, * artMach3,
		* artMach4, * artMisc1, * artMisc2, * artMisc3, * artMisc4,
		* artMisc5, * artTorso, * artNeck, * artShoulders; //heroes' artifacts
	std::vector<CArtPlace *> backpack; //hero's visible backpack (only 5 elements!)
	int backpackPos; //unmber of first art visible in backpack (in hero's vector)
	//clickable areas
	LClickableAreaWText * portraitArea;
public:
	CHeroWindow(int playerColor); //c-tor
	~CHeroWindow(); //d-tor
	void setHero(const CGHeroInstance * hero); //sets main displayed hero
	void activate(); //activates hero window;
	void deactivate(); //activates hero window;
	virtual void show(SDL_Surface * to = NULL); //shows hero window
	void redrawCurBack(); //redraws curBAck from scratch
	void quit(); //stops displaying hero window
	void dismissCurrent(); //dissmissed currently displayed hero (curHero) //TODO: make it working
	void questlog(); //show quest log in hero window
	void gar1(); //garrison / formation handling
	void gar2(); //garrison / formation handling
	void gar3(); //garrison / formation handling
	void gar4(); //garrison / formation handling
	void leftArtRoller(); //scrolls artifacts in bag left
	void rightArtRoller(); //scrolls artifacts in bag right
	void switchHero(); //changes displayed hero
};
