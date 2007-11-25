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
class CIntObject //interface object
{
public:
	SDL_Rect pos;
	int ID;
};
class CSimpleWindow : public virtual CIntObject
{
public:
	SDL_Surface * bitmap;
	CIntObject * owner;
	CSimpleWindow():bitmap(NULL),owner(NULL){};
	virtual ~CSimpleWindow();
};
class CButtonBase : public virtual CIntObject //basic buttton class
{
public:
	int type; //advmapbutton=2
	bool abs;
	bool active;
	CIntObject * ourObj; // "owner"
	int state;
	std::vector< std::vector<SDL_Surface*> > imgs;
	int curimg;
	virtual void show() ;
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
	void show();
};

class CInfoWindow : public CSimpleWindow //text + comp. + ok button
{
public:
	CSCButton<CInfoWindow> okb;
	std::vector<SComponent*> components;
	void okClicked(tribool down);
	void close();
	CInfoWindow();
	~CInfoWindow();
};

class SComponent : public ClickableR
{
public:
	enum Etype
	{
		primskill, secskill, resource, creature, artifact
	} type;
	int subtype; 
	int val;

	std::string description; //r-click
	std::string subtitle; 

	SComponent(Etype Type, int Subtype, int Val);
	//SComponent(const & SComponent r);
	SDL_Surface * getImg();
	
	void clickRight (tribool down);
	void activate();
	void deactivate();
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
	std::vector<CSimpleWindow*> objsToBlit;

	SDL_Surface * hInfo;
	std::vector<std::pair<int, int> > slotsPos;
	CDefEssential *luck22, *luck30, *luck42, *luck82,
		*morale22, *morale30, *morale42, *morale82;

	//overloaded funcs from Interface
	void yourTurn();
	void heroMoved(const HeroMoveDetails & details);
	void heroKilled(const CGHeroInstance*);
	void heroCreated(const CGHeroInstance*);
	void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val);
	void receivedResource(int type, int val);
	
	SDL_Surface * infoWin(const void * specific); //specific=0 => draws info about selected town/hero //TODO - gdy sie dorobi sensowna hierarchie klas ins. to wywalic tego brzydkiego void*
	void handleEvent(SDL_Event * sEvent);
	void init(CCallback * CB);
	int3 repairScreenPos(int3 pos);
	void showInfoDialog(std::string text, std::vector<SComponent*> & components);
	void removeObjToBlit(CSimpleWindow* obj);

	CPlayerInterface(int Player, int serial);
};