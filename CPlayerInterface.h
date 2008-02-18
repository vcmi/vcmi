#pragma once
#include "global.h"
#include "CGameInterface.h"
#include "SDL.h"
#include "SDL_framerate.h"
class CDefEssential;

class CDefHandler;
struct HeroMoveDetails;
class CDefEssential;
class CGHeroInstance;
class CAdvMapInt;
class CCastleInterface;
class IShowable
{
public:
	virtual void show(SDL_Surface * to = NULL)=0;
};

class IStatusBar
{
public:
	virtual ~IStatusBar(){}; //d-tor
	virtual void print(std::string text)=0; //prints text and refreshes statusbar
	virtual void clear()=0;//clears statusbar and refreshes
	virtual void show()=0; //shows statusbar (with current text)
	virtual std::string getCurrent()=0;
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
	int bitmapOffset;
	int type; //advmapbutton=2
	bool abs;
	bool active;
	bool notFreeButton;
	CIntObject * ourObj; // "owner"
	int state;
	std::vector< std::vector<SDL_Surface*> > imgs;
	int curimg;
	virtual void show(SDL_Surface * to = NULL);
	virtual void activate()=0;
	virtual void deactivate()=0;
	CButtonBase();
	virtual ~CButtonBase();
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
{ //okno usuwa swoje komponenty w chwili zamkniecia 
public:
	CSCButton<CInfoWindow> okb;
	std::vector<SComponent*> components;
	virtual void okClicked(tribool down);
	virtual void close();
	CInfoWindow();
	virtual ~CInfoWindow();
};
class CSelWindow : public CInfoWindow //component selection window
{ //uwaga - to okno nie usuwa swoich komponentow przy usuwaniu, moga sie one jeszcze przydac skryptowi - tak wiec skrypt korzystajacyz tego okna musi je usunac
public:
	void selectionChange(CSelectableComponent * to);
	void okClicked(tribool down);
	void close();
	CSelWindow(){};
};

class CRClickPopup : public IShowable, public ClickableR
{
	virtual void activate()=0;
	virtual void deactivate()=0;
	virtual void close()=0;
	virtual void show()=0;
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
class CGarrisonInt;
class CGarrisonSlot : public ClickableL, public ClickableR, public Hoverable
{
public:
	CGarrisonInt *owner;
	const CCreature * creature;
	int count;
	int upg; //0 - up garrison, 1 - down garrison
	
	virtual void hover (bool on);
	void clickRight (tribool down);
	void clickLeft(tribool down);
	void activate();
	void deactivate();
	void show();
	CGarrisonSlot(CGarrisonInt *Owner, int x, int y, int IID, int Upg=0, const CCreature * Creature=NULL, int Count=0);
};

class CGarrisonInt :public CIntObject
{
public:
	int interx, intery;
	CGarrisonSlot *highlighted;

	SDL_Surface *sur;
	int offx, offy;
	bool ignoreEvent, update;

	const CCreatureSet *set1;
	const CCreatureSet *set2;

	std::vector<CGarrisonSlot*> *sup, *sdown;
	const CGObjectInstance *oup, *odown;

	void activate();
	void deactivate();
	void show();
	void activeteSlots();
	void deactiveteSlots();
	void deleteSlots();
	void createSlots();
	void recreateSlots();

	CGarrisonInt(int x, int y, int inx, int iny, SDL_Surface *pomsur, int OX, int OY, const CGObjectInstance *s1, const CGObjectInstance *s2=NULL);
	~CGarrisonInt();
};

class CPlayerInterface : public CGameInterface
{
public:
	bool makingTurn;
		SDL_Event * current;
	IActivable *curint;
	CAdvMapInt * adventureInt;
	CCastleInterface * castleInt;
	FPSmanager * mainFPSmng;
	IStatusBar *statusbar;
	//TODO: town interace, battle interface, other interfaces

	CCallback * cb;

	std::vector<ClickableL*> lclickable;
	std::vector<ClickableR*> rclickable;
	std::vector<Hoverable*> hoverable;
	std::vector<KeyInterested*> keyinterested;
	std::vector<MotionInterested*> motioninterested;
	std::vector<TimeInterested*> timeinterested;
	std::vector<IShowable*> objsToBlit;

	SDL_Surface * hInfo, *tInfo;
	std::vector<std::pair<int, int> > slotsPos;
	CDefEssential *luck22, *luck30, *luck42, *luck82,
		*morale22, *morale30, *morale42, *morale82,
		*halls, *forts, *bigTownPic;
	std::map<int,SDL_Surface*> heroWins;
	std::map<int,SDL_Surface*> townWins;

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
	void heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town);
	void garrisonChanged(const CGObjectInstance * obj);

	void showComp(SComponent comp);

	void openTownWindow(const CGTownInstance * town); //shows townscreen
	void openHeroWindow(const CGHeroInstance * hero); //shows hero window with given hero
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
class CStatusBar
	: public CIntObject, public IStatusBar
{
public:
	SDL_Surface * bg; //background
	int middlex, middley; //middle of statusbar
	std::string current; //text currently printed

	CStatusBar(int x, int y, std::string name="ADROLLVR.bmp", int maxw=-1); //c-tor
	~CStatusBar(); //d-tor
	void print(std::string text); //prints text and refreshes statusbar
	void clear();//clears statusbar and refreshes
	void show(); //shows statusbar (with current text)
	std::string getCurrent();
};

class CList 
	: public ClickableL, public ClickableR, public Hoverable, public KeyInterested, public virtual CIntObject, public MotionInterested
{
public:
	SDL_Surface * bg;
	CDefHandler *arrup, *arrdo;
	SDL_Surface *empty, *selection; 
	SDL_Rect arrupp, arrdop; //positions of arrows
	int posw, posh; //position width/height
	int selected, //id of selected position, <0 if none
		from; 
	const int SIZE;
	tribool pressed; //true=up; false=down; indeterminate=none

	CList(int Size = 5);
	void clickLeft(tribool down);
	void activate(); 
	void deactivate();
	virtual void mouseMoved (SDL_MouseMotionEvent & sEvent)=0;
	virtual void genList()=0;
	virtual void select(int which)=0;
	virtual void draw()=0;
};
class CHeroList 
	: public CList
{
public:
	CDefHandler *mobile, *mana;
	std::vector<std::pair<const CGHeroInstance*, CPath *> > items;
	int posmobx, posporx, posmanx, posmoby, pospory, posmany;

	CHeroList(int Size = 5);
	void genList();
	void select(int which);
	void mouseMoved (SDL_MouseMotionEvent & sEvent);
	void clickLeft(tribool down);
	void clickRight(tribool down);
	void hover (bool on);
	void keyPressed (SDL_KeyboardEvent & key);
	void updateHList();
	void updateMove(const CGHeroInstance* which); //draws move points bar
	void redrawAllOne(int which);
	void draw();
	void init();
};
template<typename T>
class CTownList 
	: public CList
{
public: 
	T* owner;
	void(T::*fun)();
	std::vector<const CGTownInstance*> items;
	int posporx,pospory;

	CTownList(int Size, SDL_Rect * Pos, int arupx, int arupy, int ardox, int ardoy);
	~CTownList();
	void genList();
	void select(int which);
	void mouseMoved (SDL_MouseMotionEvent & sEvent);
	void clickLeft(tribool down);
	void clickRight(tribool down);
	void hover (bool on);
	void keyPressed (SDL_KeyboardEvent & key);
	void draw();
};