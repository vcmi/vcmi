#pragma once
#include "global.h"
#include "CGameInterface.h"
#include "SDL.h"
#include "SDL_framerate.h"
class CDefEssential;

class CHeroInstance;
class CDefHandler;
struct HeroMoveDetails;
class CDefEssential;
class CGHeroInstance;
class CAdvMapInt;

class IShowable
{
public:
	virtual void show(SDL_Surface * to = NULL)=0;
};

class IActivable
{
public:
	virtual void activate()=0;
	virtual void deactivate()=0;
};

class CIntObject //interface object
{
public:
	SDL_Rect pos;
	int ID;
};
class CSimpleWindow : public virtual CIntObject, public IShowable
{
public:
	SDL_Surface * bitmap;
	CIntObject * owner;
	virtual void show(SDL_Surface * to = NULL);
	CSimpleWindow():bitmap(NULL),owner(NULL){};
	virtual ~CSimpleWindow();
};
class CButtonBase : public virtual CIntObject, public IShowable, public IActivable //basic buttton class
{
public:
	int type; //advmapbutton=2
	bool abs;
	bool active;
	CIntObject * ourObj; // "owner"
	int state;
	std::vector< std::vector<SDL_Surface*> > imgs;
	int curimg;
	virtual void show(SDL_Surface * to = NULL);
	virtual void activate()=0;
	virtual void deactivate()=0;
	CButtonBase();
	virtual ~CButtonBase(){};
};
class ClickableL : public virtual CIntObject  //for left-clicks
{
public:
	bool pressedL;
	ClickableL();
	virtual void clickLeft (tribool down)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class ClickableR : public virtual CIntObject //for right-clicks
{
public:
	bool pressedR;
	ClickableR();
	virtual void clickRight (tribool down)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class Hoverable  : public virtual CIntObject
{
public:
	Hoverable(){hovered=false;}
	bool hovered;
	virtual void hover (bool on)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class KeyInterested : public virtual CIntObject
{
public:
	virtual void keyPressed (SDL_KeyboardEvent & key)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class MotionInterested: public virtual CIntObject
{
public:
	virtual void mouseMoved (SDL_MouseMotionEvent & sEvent)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class TimeInterested: public virtual CIntObject
{
public:
	int toNextTick;
	virtual void tick()=0;
	virtual void activate();
	virtual void deactivate();
};
template <typename T> class CSCButton: public CButtonBase, public ClickableL //prosty guzik, ktory tylko zmienia obrazek
{
public:
	int3 posr; //position in the bitmap
	int state;
	T* delg;
	void(T::*func)(tribool);
	CSCButton(CDefHandler * img, CIntObject * obj, void(T::*poin)(tribool), T* Delg=NULL);
	void clickLeft (tribool down);
	void activate();
	void deactivate();
	void show(SDL_Surface * to = NULL);
};

class CInfoWindow : public CSimpleWindow //text + comp. + ok button
{
public:
	CSCButton<CInfoWindow> okb;
	std::vector<SComponent*> components;
	virtual void okClicked(tribool down);
	virtual void close();
	CInfoWindow();
	virtual ~CInfoWindow();
};
class CSelWindow : public CInfoWindow //component selection window
{
public:
	void selectionChange(SComponent * to);
	void okClicked(tribool down);
	void close();
	CSelWindow(){};
};
class SComponent : public ClickableR
{
public:
	enum Etype
	{
		primskill, secskill, resource, creature, artifact, experience
	} type;
	int subtype; 
	int val;

	std::string description; //r-click
	std::string subtitle; 

	SComponent(Etype Type, int Subtype, int Val);
	//SComponent(const & SComponent r);
	
	void clickRight (tribool down);
	virtual SDL_Surface * getImg();
	virtual void activate();
	virtual void deactivate();
};

class CSelectableComponent : public SComponent, public ClickableL
{
public:
	bool selected;

	bool customB;
	SDL_Surface * border, *myBitmap;
	CSelWindow * owner;


	void clickLeft(tribool down);
	CSelectableComponent(Etype Type, int Sub, int Val, CSelWindow * Owner=NULL, SDL_Surface * Border=NULL);
	~CSelectableComponent();
	void activate();
	void deactivate();
	void select(bool on);
	SDL_Surface * getImg();
};

class CPlayerInterface : public CGameInterface
{
public:
	bool makingTurn;
	SDL_Event * current;
	CAdvMapInt * adventureInt;
	FPSmanager * mainFPSmng;
	//TODO: town interace, battle interface, other interfaces

	CCallback * cb;

	std::vector<ClickableL*> lclickable;
	std::vector<ClickableR*> rclickable;
	std::vector<Hoverable*> hoverable;
	std::vector<KeyInterested*> keyinterested;
	std::vector<MotionInterested*> motioninterested;
	std::vector<TimeInterested*> timeinterested;
	std::vector<IShowable*> objsToBlit;

	SDL_Surface * hInfo;
	std::vector<std::pair<int, int> > slotsPos;
	CDefEssential *luck22, *luck30, *luck42, *luck82,
		*morale22, *morale30, *morale42, *morale82;
	std::map<int,SDL_Surface*> heroWins;
	//std::map<int,SDL_Surface*> townWins;

	//overloaded funcs from Interface
	void yourTurn();
	void heroMoved(const HeroMoveDetails & details);
	void tileRevealed(int3 pos);
	void tileHidden(int3 pos);
	void heroKilled(const CGHeroInstance* hero);
	void heroCreated(const CGHeroInstance* hero);
	void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val);
	void receivedResource(int type, int val);
	void showSelDialog(std::string text, std::vector<CSelectableComponent*> & components, int askID);

	void showComp(SComponent comp);

	SDL_Surface * infoWin(const CGObjectInstance * specific); //specific=0 => draws info about selected town/hero //TODO - gdy sie dorobi sensowna hierarchie klas ins. to wywalic tego brzydkiego void*
	void handleEvent(SDL_Event * sEvent);
	void handleKeyDown(SDL_Event *sEvent);
	void handleKeyUp(SDL_Event *sEvent);
	void handleMouseMotion(SDL_Event *sEvent);
	void init(ICallback * CB);
	int3 repairScreenPos(int3 pos);
	void showInfoDialog(std::string text, std::vector<SComponent*> & components);
	void removeObjToBlit(IShowable* obj);
	SDL_Surface * drawHeroInfoWin(const CGHeroInstance * curh);
	SDL_Surface * drawPrimarySkill(const CGHeroInstance *curh, SDL_Surface *ret, int from=0, int to=PRIMARY_SKILLS);

	SDL_Surface * drawTownInfoWin(const CGTownInstance * curh);

	CPlayerInterface(int Player, int serial);
};