#ifndef CPLAYERINTERFACE_H
#define CPLAYERINTERFACE_H
#include "global.h"
#include "CGameInterface.h"
#include "SDL_framerate.h"
#include <map>

class CDefEssential;
class AdventureMapButton;
class CDefHandler;
struct HeroMoveDetails;
class CDefEssential;
class CGHeroInstance;
class CAdvMapInt;
class CCastleInterface;
class CStack;
class SComponent;
class CCreature;
struct SDL_Surface;
struct CPath;
class CCreatureAnimation;
class CSelectableComponent;
class CCreatureSet;
class CGObjectInstance;
class CSlider;
namespace boost
{
	class mutex;
};

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
	virtual ~IActivable(){};
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
	virtual ~ClickableL(){};
	virtual void clickLeft (boost::logic::tribool down)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class ClickableR : public virtual CIntObject //for right-clicks
{
public:
	bool pressedR;
	ClickableR();
	virtual ~ClickableR(){};
	virtual void clickRight (boost::logic::tribool down)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class Hoverable  : public virtual CIntObject
{
public:
	Hoverable(){hovered=false;}
	virtual ~Hoverable(){};
	bool hovered;
	virtual void hover (bool on)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class KeyInterested : public virtual CIntObject
{
public:
	virtual ~KeyInterested(){};
	virtual void keyPressed (SDL_KeyboardEvent & key)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class MotionInterested: public virtual CIntObject
{
public:
	bool strongInterest; //if true - report all mouse movements, if not - only when hovered
	MotionInterested(){strongInterest=false;};
	virtual ~MotionInterested(){};
	virtual void mouseMoved (SDL_MouseMotionEvent & sEvent)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class TimeInterested: public virtual CIntObject
{
public:
	virtual ~TimeInterested(){};
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
	void(T::*func)(boost::logic::tribool);
	CSCButton(CDefHandler * img, CIntObject * obj, void(T::*poin)(boost::logic::tribool), T* Delg=NULL);
	void clickLeft (boost::logic::tribool down);
	void activate();
	void deactivate();
	void show(SDL_Surface * to = NULL);
};

class CInfoWindow : public CSimpleWindow //text + comp. + ok button
{ //window deletes its components when closed
public:
	std::vector<AdventureMapButton *> buttons;
	std::vector<SComponent*> components;
	virtual void close();
	virtual void show(SDL_Surface * to = NULL);
	void activate();
	void deactivate();
	CInfoWindow(std::string text, int player, int charperline, std::vector<SComponent*> &comps, std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons);
	CInfoWindow();
	~CInfoWindow();
};
class CSelWindow : public CInfoWindow //component selection window
{ //uwaga - to okno usuwa swoje komponenty przy zamykaniu
public:
	void selectionChange(unsigned to);
	void close();
	CSelWindow(std::string text, int player, int charperline, std::vector<CSelectableComponent*> &comps, std::vector<std::pair<std::string,boost::function<void()> > > &Buttons);
	CSelWindow(){};
};

class CRClickPopup : public IShowable, public ClickableR
{
public:
	virtual void activate();
	virtual void deactivate();
	virtual void close()=0;
	void clickRight (boost::logic::tribool down);
	virtual ~CRClickPopup(){};
};

class CInfoPopup : public CRClickPopup
{
public:
	bool free;
	SDL_Surface * bitmap;
	CInfoPopup(SDL_Surface * Bitmap, int x, int y, bool Free=false);
	void close();
	void show(SDL_Surface * to = NULL);
	CInfoPopup(){free=false;bitmap=NULL;}
	~CInfoPopup(){};
};

class SComponent : public ClickableR
{
public:
	enum Etype
	{
		primskill, secskill, resource, creature, artifact, experience, secskill44
	} type;
	int subtype;
	int val;

	std::string description; //r-click
	std::string subtitle;

	void init(Etype Type, int Subtype, int Val);
	SComponent(Etype Type, int Subtype, int Val);
	SComponent(const Component &c);

	void clickRight (boost::logic::tribool down);
	virtual SDL_Surface * getImg();
	virtual void show(SDL_Surface * to = NULL);
	virtual void activate();
	virtual void deactivate();
};

class CSelectableComponent : public SComponent, public ClickableL
{
public:
	bool selected;

	bool customB;
	SDL_Surface * border, *myBitmap;
	boost::function<void()> onSelect;


	void clickLeft(boost::logic::tribool down);
	void init(SDL_Surface * Border);
	CSelectableComponent(Etype Type, int Sub, int Val, boost::function<void()> OnSelect = 0, SDL_Surface * Border=NULL);
	CSelectableComponent(const Component &c, boost::function<void()> OnSelect = 0, SDL_Surface * Border=NULL);
	~CSelectableComponent();
	virtual void show(SDL_Surface * to = NULL);
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
	bool active;

	virtual void hover (bool on);
	const CArmedInstance * getObj();
	void clickRight (boost::logic::tribool down);
	void clickLeft(boost::logic::tribool down);
	void activate();
	void deactivate();
	void show();
	CGarrisonSlot(CGarrisonInt *Owner, int x, int y, int IID, int Upg=0, const CCreature * Creature=NULL, int Count=0);
	~CGarrisonSlot();
};

class CGarrisonInt :public CIntObject
{
public:
	int interx, intery;
	CGarrisonSlot *highlighted;

	SDL_Surface *sur;
	int offx, offy, p2;
	bool ignoreEvent, update, active, splitting, pb;

	const CCreatureSet *set1;
	const CCreatureSet *set2;

	std::vector<CGarrisonSlot*> *sup, *sdown;
	const CArmedInstance *oup, *odown;

	void activate();
	void deactivate();
	void show();
	void activeteSlots();
	void deactiveteSlots();
	void deleteSlots();
	void createSlots();
	void recreateSlots();

	void splitClick();
	void splitStacks(int am2);

	CGarrisonInt(int x, int y, int inx, int iny, SDL_Surface *pomsur, int OX, int OY, const CArmedInstance *s1, const CArmedInstance *s2=NULL);
	~CGarrisonInt();
};

class CPlayerInterface : public CGameInterface
{
public:
	//minor interfaces
	boost::mutex *pim;
	bool makingTurn;
	SDL_Event * current;
	IActivable *curint;
	CAdvMapInt * adventureInt;
	CCastleInterface * castleInt;
	FPSmanager * mainFPSmng;
	IStatusBar *statusbar;

	//to commucate with engine
	CCallback * cb;

	//GUI elements
	std::vector<ClickableL*> lclickable;
	std::vector<ClickableR*> rclickable;
	std::vector<Hoverable*> hoverable;
	std::vector<KeyInterested*> keyinterested;
	std::vector<MotionInterested*> motioninterested;
	std::vector<TimeInterested*> timeinterested;
	std::vector<IShowable*> objsToBlit;

	//overloaded funcs from CGameInterface
	void yourTurn();
	void heroMoved(const HeroMoveDetails & details);
	void tileRevealed(int3 pos);
	void tileHidden(int3 pos);
	void heroKilled(const CGHeroInstance* hero);
	void heroCreated(const CGHeroInstance* hero);
	void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val);
	void receivedResource(int type, int val);
	void showInfoDialog(std::string &text, std::vector<Component*> &components);
	void showSelDialog(std::string &text, std::vector<Component*> &components, ui32 askID);
	void heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town);
	void garrisonChanged(const CGObjectInstance * obj);
	void buildChanged(const CGTownInstance *town, int buildingID, int what); //what: 1 - built, 2 - demolished
	void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback);
	//for battles
	void battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side); //called by engine when battle starts; side=0 - left, side=1 - right
	void battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles); //called when battlefield is prepared, prior the battle beginning
	void battleNewRound(int round); //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	//void actionStarted(BattleAction action);//occurs BEFORE every action taken by any stack or by the hero
	//void actionFinished(BattleAction action);//occurs AFTER every action taken by any stack or by the hero
	BattleAction activeStack(int stackID); //called when it's turn of that stack
	void battleEnd(BattleResult *br);
	void battleStackMoved(int ID, int dest, bool startMoving, bool endMoving);
	void battleAttack(BattleAttack *ba);
	void battleStackKilled(int ID, int dmg, int killed, int IDby, bool byShooting);
	void battleStackIsShooting(int ID, int dest); //called when stack with id ID is shooting to hex dest


	//-------------//

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
	void removeObjToBlit(IShowable* obj);	
	void showInfoDialog(std::string &text, std::vector<SComponent*> & components);
	void showYesNoDialog(std::string &text, std::vector<SComponent*> & components, CFunctionList<void()> funcs[2]);

	CPlayerInterface(int Player, int serial);//c-tor
	~CPlayerInterface();//d-tor
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
	boost::logic::tribool pressed; //true=up; false=down; indeterminate=none

	CList(int Size = 5);
	void clickLeft(boost::logic::tribool down);
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
	void clickLeft(boost::logic::tribool down);
	void clickRight(boost::logic::tribool down);
	void hover (bool on);
	void keyPressed (SDL_KeyboardEvent & key);
	void updateHList();
	void updateMove(const CGHeroInstance* which); //draws move points bar
	void redrawAllOne(int which);
	void draw();
	void init();
};

class CTownList
	: public CList
{
public:
	boost::function<void()> fun;
	std::vector<const CGTownInstance*> items;
	int posporx,pospory;

	CTownList(int Size, SDL_Rect * Pos, int arupx, int arupy, int ardox, int ardoy);
	~CTownList();
	void genList();
	void select(int which);
	void mouseMoved (SDL_MouseMotionEvent & sEvent);
	void clickLeft(boost::logic::tribool down);
	void clickRight(boost::logic::tribool down);
	void hover (bool on);
	void keyPressed (SDL_KeyboardEvent & key);
	void draw();
};

class CCreaturePic //draws 100x130 picture with creature on background, use nextFrame=true to get animation
{
public:
	CCreature *c;
	CCreatureAnimation *anim;
	CCreaturePic(CCreature *cre);
	~CCreaturePic();
	int blitPic(SDL_Surface *to, int x, int y, bool nextFrame);
	SDL_Surface * getPic(bool nextFrame);
};

class CRecrutationWindow : public IShowable, public ClickableL
{
public:
	struct creinfo
	{
		SDL_Rect pos;
		CCreaturePic *pic;
		int ID, amount; //creature ID and available amount
		std::vector<std::pair<int,int> > res; //res_id - cost_per_unit
	};
	std::vector<int> amounts; //how many creatures we can afford
	std::vector<creinfo> creatures;
	boost::function<void(int,int)> recruit; //void (int ID, int amount) <-- call to recruit creatures
	CSlider *slider;
	AdventureMapButton *max, *buy, *cancel;
	SDL_Surface *bitmap;
	int which; //which creature is active

	void close();
	void Max();
	void Buy();
	void Cancel();
	void sliderMoved(int to);
	void clickLeft(boost::logic::tribool down);
	void activate();
	void deactivate();
	void show(SDL_Surface * to = NULL);
	CRecrutationWindow(const std::vector<std::pair<int,int> > & Creatures, const boost::function<void(int,int)> & Recruit); //creatures - pairs<creature_ID,amount>
	~CRecrutationWindow();
};

class CSplitWindow : public IShowable, public KeyInterested
{
public:
	CGarrisonInt *gar;
	CSlider *slider;
	CCreaturePic *anim;
	AdventureMapButton *ok, *cancel;
	SDL_Surface *bitmap;
	int a1, a2, c;
	bool which;

	CSplitWindow(int cid, int max, CGarrisonInt *Owner);
	~CSplitWindow();
	void activate();
	void split();
	void close();
	void deactivate();
	void show(SDL_Surface * to = NULL);
	void keyPressed (SDL_KeyboardEvent & key);
	void sliderMoved(int to);
};

class CCreInfoWindow : public IShowable, public KeyInterested, public ClickableR
{
public:
	int type;//0 - rclick popup; 1 - normal window
	SDL_Surface *bitmap;
	bool anf;

	boost::function<void()> dsm;
	CCreaturePic *anim;
	CCreature *c;
	CInfoWindow *dependant; //it may be dialog asking whther upgrade/dismiss stack (if opened)

	AdventureMapButton *dismiss, *upgrade, *ok;
	CCreInfoWindow(int Cid, int Type, StackState *State, boost::function<void()> Upg, boost::function<void()> Dsm);
	~CCreInfoWindow();
	void activate();
	void close();
	void clickRight(boost::logic::tribool down);
	void dismissF();
	void keyPressed (SDL_KeyboardEvent & key);
	void deactivate();
	void show(SDL_Surface * to = NULL);
	void onUpgradeYes();
	void onUpgradeNo();
};

class CLevelWindow : public IShowable, public CIntObject
{
public:
	int heroType;
	SDL_Surface *bitmap;
	std::vector<CSelectableComponent *> comps; //skills to select
	AdventureMapButton *ok;
	boost::function<void(ui32)> cb;
	
	void close();
	CLevelWindow(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback);
	~CLevelWindow();
	void activate();
	void deactivate();
	void selectionChanged(unsigned to);
	void show(SDL_Surface * to = NULL);
};

extern CPlayerInterface * LOCPLINT;

#endif //CPLAYERINTERFACE_H
