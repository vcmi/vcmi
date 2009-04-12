#ifndef __CPLAYERINTERFACE_H__
#define __CPLAYERINTERFACE_H__
#include "global.h"
#include "CGameInterface.h"
#include "SDL_framerate.h"
#include <map>
#include <list>
#include <algorithm>

#ifdef __GNUC__
#define sprintf_s snprintf 
#endif

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

class CDefEssential;
class AdventureMapButton;
class CHighlightableButtonsGroup;
class CDefHandler;
struct HeroMoveDetails;
class CDefEssential;
class CGHeroInstance;
class CAdvMapInt;
class CCastleInterface;
class CBattleInterface;
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
struct UpgradeInfo;
template <typename T> struct CondSh;
class CInGameConsole;

namespace boost
{
	class mutex;
	class recursive_mutex;
};

struct Point
{
	int x, y;

	//constructors
	Point(){};
	Point(int X, int Y)
		:x(X),y(Y)
	{};
	Point(const int3 &a)
		:x(a.x),y(a.y)
	{}

	Point operator+(const Point &b) const
	{
		return Point(x+b.x,y+b.y);
	}
	Point& operator+=(const Point &b)
	{
		x += b.x;
		y += b.y;
		return *this;
	}
	Point operator-(const Point &b) const
	{
		return Point(x+b.x,y+b.y);
	}
	Point& operator-=(const Point &b)
	{
		x -= b.x;
		y -= b.y;
		return *this;
	}
	bool operator<(const Point &b) const //product order
	{
		return x < b.x   &&   y < b.y;
	}
};

struct Rect : public SDL_Rect
{
	Rect()//default c-tor
	{
		x = y = w = h = -1;
	}
	Rect(int X, int Y, int W, int H) //c-tor
	{
		x = X;
		y = Y;
		w = W;
		h = H;
	}
	Rect(const SDL_Rect & r) //c-tor
	{
		x = r.x;
		y = r.y;
		w = r.w;
		h = r.h;
	}
	bool isIn(int qx, int qy) const //determines if given point lies inside rect
	{
		if (qx > x   &&   qx<x+w   &&   qy>y   &&   qy<y+h)
			return true;
		return false;
	}
	bool isIn(const Point &q) const //determines if given point lies inside rect
	{
		return isIn(q.x,q.y);
	}
	Point topLeft() const //top left corner of this rect
	{
		return Point(x,y);
	}
	Point topRight() const //top right corner of this rect
	{
		return Point(x+w,y);
	}
	Point bottomLeft() const //bottom left corner of this rect
	{
		return Point(x,y+h);
	}
	Point bottomRight() const //bottom right corner of this rect
	{
		return Point(x+w,y+h);
	}
	Rect operator+(const Rect &p) const //moves this rect by p's rect position
	{
		return Rect(x+p.x,y+p.y,w,h);
	}
	Rect operator+(const Point &p) const //moves this rect by p's point position
	{
		return Rect(x+p.x,y+p.y,w,h);
	}
	Rect& operator=(const Rect &p) //assignment operator
	{
		x = p.x;
		y = p.y;
		w = p.w;
		h = p.h;
		return *this;
	}
	Rect& operator+=(const Rect &p) //works as operator+
	{
		x += p.x;
		y += p.y;
		return *this;
	}
	Rect operator&(const Rect &p) const //rect intersection
	{
		bool intersect = true;

		if(p.topLeft().y < y && p.bottomLeft().y < y) //rect p is above *this
		{
			intersect = false;
		}
		else if(p.topLeft().y > y+h && p.bottomLeft().y > y+h) //rect p is below *this
		{
			intersect = false;
		}
		else if(p.topLeft().x > x+w && p.topRight().x > x+w) //rect p is on the right hand side of this
		{
			intersect = false;
		}
		else if(p.topLeft().x < x && p.topRight().x < x) //rect p is on the left hand side of this
		{
			intersect = false;
		}

		if(intersect)
		{
			Rect ret;
			ret.x = std::max(this->x, p.x);
			ret.y = std::max(this->y, p.y);
			Point bR; //bottomRight point of returned rect
			bR.x = std::min(this->w+this->x, p.w+p.x);
			bR.y = std::min(this->h+this->y, p.h+p.y);
			ret.w = bR.x - ret.x;
			ret.h = bR.y - ret.y;
			return ret;
		}
		else
		{
			return Rect();
		}
	}
};

class IShowable
{
public:
	virtual void show(SDL_Surface * to = NULL)=0;
	virtual ~IShowable(){};
};

class IStatusBar
{
public:
	virtual ~IStatusBar(){}; //d-tor
	virtual void print(const std::string & text)=0; //prints text and refreshes statusbar
	virtual void clear()=0;//clears statusbar and refreshes
	virtual void show()=0; //shows statusbar (with current text)
	virtual std::string getCurrent()=0; //returns currently displayed text
};

class IActivable
{
public:
	virtual void activate()=0;
	virtual void deactivate()=0;
	virtual ~IActivable(){};
};
class IShowActivable : public IShowable, public IActivable
{
public:
	virtual ~IShowActivable(){};
};
class CMainInterface : public IShowActivable
{
public:
	IShowActivable *subInt;
};
class CIntObject //interface object
{
public:
	Rect pos; //position of object on the screen
	int ID; //object uniqe ID, rarely (if at all) used

	//virtual bool isIn(int x, int y)
	//{
	//	return pos.isIn(x,y);
	//}
	virtual ~CIntObject(){}; //d-tor
};
class CSimpleWindow : public virtual CIntObject, public IShowable
{
public:
	SDL_Surface * bitmap;
	CIntObject * owner; //who made this window
	virtual void show(SDL_Surface * to = NULL);
	CSimpleWindow():bitmap(NULL),owner(NULL){}; //c-tor
	virtual ~CSimpleWindow(); //d-tor
};
class CButtonBase : public virtual CIntObject, public IShowable, public IActivable //basic buttton class
{
public:
	int bitmapOffset; //TODO: comment me
	int type; //advmapbutton=2 //TODO: comment me
	bool abs;//TODO: comment me
	bool active; //if true, this button is active and can be pressed
	bool notFreeButton; //TODO: comment me
	CIntObject * ourObj; // "owner"
	int state; //TODO: comment me
	std::vector< std::vector<SDL_Surface*> > imgs; //images for this button
	int curimg; //curently displayed image from imgs
	virtual void show(SDL_Surface * to = NULL);
	virtual void activate()=0;
	virtual void deactivate()=0;
	CButtonBase(); //c-tor
	virtual ~CButtonBase(); //d-tor
};
class ClickableL : public virtual CIntObject  //for left-clicks
{
public:
	bool pressedL; //for determining if object is L-pressed
	ClickableL(); //c-tor
	virtual ~ClickableL();//{};
	virtual void clickLeft (boost::logic::tribool down)=0;
	virtual void activate();
	virtual void deactivate();
};
class ClickableR : public virtual CIntObject //for right-clicks
{
public:
	bool pressedR; //for determining if object is R-pressed
	ClickableR(); //c-tor
	virtual ~ClickableR();//{};
	virtual void clickRight (boost::logic::tribool down)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class Hoverable  : public virtual CIntObject
{
public:
	Hoverable() : hovered(false){} //c-tor
	virtual ~Hoverable();//{}; //d-tor
	bool hovered;  //for determining if object is hovered
	virtual void hover (bool on)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class KeyInterested : public virtual CIntObject
{	
public:
	bool captureAllKeys; //if true, only this object should get info about pressed keys
	KeyInterested(): captureAllKeys(false){}
	virtual ~KeyInterested();//{};
	virtual void keyPressed(const SDL_KeyboardEvent & key)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};

class KeyShortcut : public KeyInterested, public ClickableL
{
public:
	std::set<int> assignedKeys;
	KeyShortcut(){}; //c-tor
	KeyShortcut(int key){assignedKeys.insert(key);}; //c-tor
	KeyShortcut(std::set<int> Keys):assignedKeys(Keys){}; //c-tor
	virtual void keyPressed(const SDL_KeyboardEvent & key); //call-in
};

class MotionInterested: public virtual CIntObject
{
public:
	bool strongInterest; //if true - report all mouse movements, if not - only when hovered
	MotionInterested(){strongInterest=false;};
	virtual ~MotionInterested(){};
	virtual void mouseMoved (const SDL_MouseMotionEvent & sEvent)=0;
	virtual void activate()=0;
	virtual void deactivate()=0;
};
class TimeInterested: public virtual CIntObject
{
public:
	virtual ~TimeInterested(){}; //d-tor
	int toNextTick;
	virtual void tick()=0;
	virtual void activate();
	virtual void deactivate();
};

class CInfoWindow : public CSimpleWindow //text + comp. + ok button
{ //window able to delete its components when closed
public:
	bool delComps; //whether comps will be deleted
	std::vector<AdventureMapButton *> buttons;
	std::vector<SComponent*> components;
	virtual void close();
	virtual void show(SDL_Surface * to = NULL);
	void activate();
	void deactivate();
	CInfoWindow(std::string text, int player, int charperline, const std::vector<SComponent*> &comps, std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons); //c-tor
	CInfoWindow(); //c-tor
	~CInfoWindow(); //d-tor
};
class CSelWindow : public CInfoWindow //component selection window
{ //warning - this window deletes its components by closing!
public:
	void selectionChange(unsigned to);
	void close();
	void madeChoice(); //looks for selected component and calls callback
	CSelWindow(const std::string& text, int player, int charperline ,const std::vector<CSelectableComponent*> &comps, const std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons, int askID); //c-tor
	CSelWindow(){}; //c-tor
	//notification - this class inherits important destructor from CInfoWindow
};

class CRClickPopup : public IShowable, public ClickableR //popup displayed on R-click
{
public:
	virtual void activate();
	virtual void deactivate();
	virtual void close()=0;
	void clickRight (boost::logic::tribool down);
	virtual ~CRClickPopup(){}; //d-tor
};

class CInfoPopup : public CRClickPopup
{
public:
	bool free; //TODO: comment me
	SDL_Surface * bitmap; //popup background
	CInfoPopup(SDL_Surface * Bitmap, int x, int y, bool Free=false); //c-tor
	void close();
	void show(SDL_Surface * to = NULL);
	CInfoPopup(){free=false;bitmap=NULL;} //default c-tor
	~CInfoPopup(){}; //d-tor
};

class SComponent : public ClickableR //common popup window component
{
public:
	enum Etype
	{
		primskill, secskill, resource, creature, artifact, experience, secskill44, spell, morale, luck
	} type; //component type
	int subtype; //TODO: comment me
	int val; //TODO: comment me

	std::string description; //r-click
	std::string subtitle; //TODO: comment me

	void init(Etype Type, int Subtype, int Val);
	SComponent(Etype Type, int Subtype, int Val); //c-tor
	SComponent(const Component &c); //c-tor
	SComponent(){}; //c-tor
	virtual ~SComponent(){}; //d-tor

	void clickRight (boost::logic::tribool down); //call-in
	virtual SDL_Surface * getImg();
	virtual void show(SDL_Surface * to = NULL);
	virtual void activate();
	virtual void deactivate();
};

class CCustomImgComponent :  public SComponent
{
public:
	bool free; //should surface be freed on delete
	SDL_Surface *bmp; //our image
	SDL_Surface * getImg();
	CCustomImgComponent(Etype Type, int Subtype, int Val, SDL_Surface *sur, bool freeSur); //c-tor
	~CCustomImgComponent(); //d-tor
};

class CSelectableComponent : public SComponent, public KeyShortcut
{
public:
	bool selected; //if true, this component is selected

	bool customB; //TODO: comment me
	SDL_Surface * border, *myBitmap;
	boost::function<void()> onSelect; //function called on selection change

	void clickLeft(boost::logic::tribool down); //call-in
	void init(SDL_Surface * Border);
	CSelectableComponent(Etype Type, int Sub, int Val, boost::function<void()> OnSelect = 0, SDL_Surface * Border=NULL); //c-tor
	CSelectableComponent(const Component &c, boost::function<void()> OnSelect = 0, SDL_Surface * Border=NULL); //c-tor
	~CSelectableComponent(); //d-tor
	virtual void show(SDL_Surface * to = NULL);
	void activate();
	void deactivate();
	void select(bool on);
	SDL_Surface * getImg(); //returns myBitmap
};
class CGarrisonInt;
class CGarrisonSlot : public ClickableL, public ClickableR, public Hoverable
{
public:
	CGarrisonInt *owner;
	const CCreature * creature; //creature in slot
	int count; //number of creatures
	int upg; //0 - up garrison, 1 - down garrison
	bool active; //TODO: comment me

	virtual void hover (bool on); //call-in
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
	int interx, intery; //intervals between slots
	CGarrisonSlot *highlighted; //choosen slot

	SDL_Surface *&sur; //TODO: comment me
	int offx, offy, p2; //TODO: comment me
	bool ignoreEvent, update, active, splitting, pb;

	const CCreatureSet *set1; //top set of creatures
	const CCreatureSet *set2; //bottom set of creatures

	std::vector<CGarrisonSlot*> *sup, *sdown; //TODO: comment me
	const CArmedInstance *oup, *odown; //TODO: comment me

	void activate();
	void deactivate();
	void show();
	void activeteSlots();
	void deactiveteSlots();
	void deleteSlots();
	void createSlots();
	void recreateSlots();

	void splitClick(); //handles click on split button
	void splitStacks(int am2); //TODO: comment me

	CGarrisonInt(int x, int y, int inx, int iny, SDL_Surface *&pomsur, int OX, int OY, const CArmedInstance *s1, const CArmedInstance *s2=NULL); //c-tor
	~CGarrisonInt(); //d-tor
};

class CPlayerInterface : public CGameInterface
{
public:
	//minor interfaces
	CondSh<bool> *showingDialog; //indicates if dialog box is displayed
	boost::recursive_mutex *pim; //locks read/write of this
	bool makingTurn; //indicates if player is already making his turn
	int heroMoveSpeed; //speed of player's hero movement
	void setHeroMoveSpeed(int newSpeed) {heroMoveSpeed = newSpeed;} //set for the member above
	int mapScrollingSpeed; //map scrolling speed
	void setMapScrollingSpeed(int newSpeed) {mapScrollingSpeed = newSpeed;} //set the member above
	SDL_Event * current; //current event
	CMainInterface *curint;
	CAdvMapInt * adventureInt;
	CCastleInterface * castleInt;
	CBattleInterface * battleInt;
	CInGameConsole * cingconsole;
	FPSmanager * mainFPSmng;
	IStatusBar *statusbar; //advmap statusbar; should it be used by other windows with statusbar?
	//to commucate with engine
	CCallback * cb;
	const BattleAction *curAction;
	bool stillMoveHero;

	std::list<CInfoWindow *> dialogs;

	//GUI elements
	std::list<ClickableL*> lclickable;
	std::list<ClickableR*> rclickable;
	std::list<Hoverable*> hoverable;
	std::list<KeyInterested*> keyinterested;
	std::list<MotionInterested*> motioninterested;
	std::list<TimeInterested*> timeinterested;
	std::vector<IShowable*> objsToBlit;

	//overloaded funcs from CGameInterface
	void buildChanged(const CGTownInstance *town, int buildingID, int what); //what: 1 - built, 2 - demolished
	void garrisonChanged(const CGObjectInstance * obj);
	void heroArtifactSetChanged(const CGHeroInstance*hero);
	void heroCreated(const CGHeroInstance* hero);
	void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback);
	void heroInGarrisonChange(const CGTownInstance *town);
	void heroKilled(const CGHeroInstance* hero);
	void heroMoved(const HeroMoveDetails & details);
	void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val);
	void heroManaPointsChanged(const CGHeroInstance * hero);
	void heroMovePointsChanged(const CGHeroInstance * hero);
	void heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town);
	void receivedResource(int type, int val);
	void showInfoDialog(const std::string &text, const std::vector<Component*> &components);
	//void showSelDialog(const std::string &text, const std::vector<Component*> &components, ui32 askID);
	//void showYesNoDialog(const std::string &text, const std::vector<Component*> &components, ui32 askID);
	void showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, bool selection, bool cancel); //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, boost::function<void()> &onEnd);
	void tileHidden(const std::set<int3> &pos);
	void tileRevealed(const std::set<int3> &pos);
	void yourTurn();
	void availableCreaturesChanged(const CGTownInstance *town);
	void heroBonusChanged(const CGHeroInstance *hero, const HeroBonus &bonus, bool gain);//if gain hero received bonus, else he lost it
	void serialize(COSer<CSaveFile> &h, const int version); //saving
	void serialize(CISer<CLoadFile> &h, const int version); //loading

	//for battles
	void actionFinished(const BattleAction* action);//occurs AFTER action taken by active stack or by the hero
	void actionStarted(const BattleAction* action);//occurs BEFORE action taken by active stack or by the hero
	BattleAction activeStack(int stackID); //called when it's turn of that stack
	void battleAttack(BattleAttack *ba); //stack performs attack
	void battleEnd(BattleResult *br); //end of battle
	void battleResultQuited();
	void battleNewRound(int round); //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	void battleStackMoved(int ID, int dest, int distance, bool end);
	void battleSpellCasted(SpellCasted *sc);
	void battleStacksEffectsSet(SetStackEffect & sse); //called when a specific effect is set to stacks
	void battleStacksAttacked(std::set<BattleStackAttacked> & bsa);
	void battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side); //called by engine when battle starts; side=0 - left, side=1 - right
	void battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles); //called when battlefield is prepared, prior the battle beginning


	//-------------//
	bool shiftPressed() const;
	void redrawHeroWin(const CGHeroInstance * hero);
	void updateWater();
	void showComp(SComponent comp); //TODO: comment me
	void openTownWindow(const CGTownInstance * town); //shows townscreen
	void openHeroWindow(const CGHeroInstance * hero); //shows hero window with given hero
	SDL_Surface * infoWin(const CGObjectInstance * specific); //specific=0 => draws info about selected town/hero
	void handleEvent(SDL_Event * sEvent);
	void handleKeyDown(SDL_Event *sEvent);
	void handleKeyUp(SDL_Event *sEvent);
	void handleMouseMotion(SDL_Event *sEvent);
	void init(ICallback * CB);
	int3 repairScreenPos(int3 pos); //returns position closest to pos we can center screen on
	void removeObjToBlit(IShowable* obj);
	void showInfoDialog(const std::string &text, const std::vector<SComponent*> & components, bool deactivateCur=true);
	void showYesNoDialog(const std::string &text, const std::vector<SComponent*> & components, CFunctionList<void()> onYes, CFunctionList<void()> onNo, bool deactivateCur, bool DelComps); //deactivateCur - whether current main interface should be deactivated; delComps - if components will be deleted on window close
	bool moveHero(const CGHeroInstance *h, CPath * path);

	CPlayerInterface(int Player, int serial);//c-tor
	~CPlayerInterface();//d-tor

	//////////////////////////////////////////////////////////////////////////

	template <typename Handler> void serializeTempl(Handler &h, const int version);
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
	void print(const std::string & text); //prints text and refreshes statusbar
	void clear();//clears statusbar and refreshes
	void show(); //shows statusbar (with current text)
	std::string getCurrent(); //getter for current
};

class CList
	: public ClickableL, public ClickableR, public Hoverable, public KeyInterested, public virtual CIntObject, public MotionInterested
{
public:
	SDL_Surface * bg; //background bitmap
	CDefHandler *arrup, *arrdo; //button arrows for scrolling list
	SDL_Surface *empty, *selection;
	SDL_Rect arrupp, arrdop; //positions of arrows
	int posw, posh; //position width/height
	int selected, //id of selected position, <0 if none
		from;
	const int SIZE; //size of list
	boost::logic::tribool pressed; //true=up; false=down; indeterminate=none

	CList(int Size = 5); //c-tor
	void clickLeft(boost::logic::tribool down);
	void activate();
	void deactivate();
	virtual void mouseMoved (const SDL_MouseMotionEvent & sEvent)=0; //call-in
	virtual void genList()=0;
	virtual void select(int which)=0;
	virtual void draw()=0;
};
class CHeroList
	: public CList
{
public:
	CDefHandler *mobile, *mana; //mana and movement indicators
	std::vector<std::pair<const CGHeroInstance*, CPath *> > items;
	int posmobx, posporx, posmanx, posmoby, pospory, posmany;

	CHeroList(int Size); //c-tor
	int getPosOfHero(const CArmedInstance* h); //hero's position on list
	void genList();
	void select(int which); //call-in
	void mouseMoved (const SDL_MouseMotionEvent & sEvent); //call-in
	void clickLeft(boost::logic::tribool down); //call-in
	void clickRight(boost::logic::tribool down); //call-in
	void hover (bool on); //call-in
	void keyPressed (const SDL_KeyboardEvent & key); //call-in
	void updateHList(const CGHeroInstance *toRemove=NULL); //removes specific hero from the list or recreates it
	void updateMove(const CGHeroInstance* which); //draws move points bar
	void redrawAllOne(int which); //not imeplemented
	void draw();
	void init();
};

class CTownList
	: public CList
{
public:
	boost::function<void()> fun; //function called on selection change
	std::vector<const CGTownInstance*> items; //towns on list
	int posporx,pospory;

	CTownList(int Size, int x, int y, std::string arrupg, std::string arrdog); //c-tor
	~CTownList(); //d-tor
	void genList();
	void select(int which); //call-in
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);  //call-in
	void clickLeft(boost::logic::tribool down);  //call-in
	void clickRight(boost::logic::tribool down); //call-in
	void hover (bool on);  //call-in
	void keyPressed (const SDL_KeyboardEvent & key);  //call-in
	void draw();
};

class CCreaturePic //draws picture with creature on background, use nextFrame=true to get animation
{
public:
	bool big; //big => 100x130; !big => 100x120
	CCreature *c; //which creature's picture
	CCreatureAnimation *anim; //displayed animation
	CCreaturePic(CCreature *cre, bool Big=true); //c-tor
	~CCreaturePic(); //d-tor
	int blitPic(SDL_Surface *to, int x, int y, bool nextFrame); //prints creature on screen
	SDL_Surface * getPic(bool nextFrame); //returns frame of animation
};

class CRecrutationWindow : public IShowable, public ClickableL, public ClickableR
{
public:
	struct creinfo
	{
		SDL_Rect pos;
		CCreaturePic *pic; //creature's animation
		int ID, amount; //creature ID and available amount
		std::vector<std::pair<int,int> > res; //res_id - cost_per_unit
	};
	std::vector<int> amounts; //how many creatures we can afford
	std::vector<creinfo> creatures; //recruitable creatures
	boost::function<void(int,int)> recruit; //void (int ID, int amount) <-- call to recruit creatures
	CSlider *slider; //for selecting amount
	AdventureMapButton *max, *buy, *cancel;
	SDL_Surface *bitmap; //background
	CStatusBar *bar;
	int which; //which creature is active

	void close();
	void Max();
	void Buy();
	void Cancel();
	void sliderMoved(int to);
	void clickLeft(boost::logic::tribool down);
	void clickRight(boost::logic::tribool down);
	void activate();
	void deactivate();
	void show(SDL_Surface * to = NULL);
	CRecrutationWindow(const std::vector<std::pair<int,int> > & Creatures, const boost::function<void(int,int)> & Recruit); //creatures - pairs<creature_ID,amount> //c-tor
	~CRecrutationWindow(); //d-tor
};

class CSplitWindow : public IShowable, public KeyInterested, public ClickableL
{
public:
	CGarrisonInt *gar;
	CSlider *slider;
	CCreaturePic *anim; //creature's animation
	AdventureMapButton *ok, *cancel;
	SDL_Surface *bitmap; //background
	int a1, a2, c; //TODO: comment me
	bool which; //TODO: comment me
	int last; //0/1/2 - at least one creature must be in the src/dst/both stacks; -1 - no restrictions

	CSplitWindow(int cid, int max, CGarrisonInt *Owner, int Last = -1, int val=0); //c-tor; val - initial amount of second stack
	~CSplitWindow(); //d-tor
	void activate();
	void split();
	void close();
	void deactivate();
	void show(SDL_Surface * to = NULL);
	void clickLeft(boost::logic::tribool down); //call-in
	void keyPressed (const SDL_KeyboardEvent & key); //call-in
	void sliderMoved(int to);
};

class CCreInfoWindow : public IShowable, public KeyInterested, public ClickableR
{
public:
	bool active; //TODO: comment me
	int type;//0 - rclick popup; 1 - normal window
	SDL_Surface *bitmap; //background
	char anf; //TODO: comment me
	std::string count; //creature count in text format

	boost::function<void()> dsm; //TODO: comment me
	CCreaturePic *anim;
	CCreature *c;
	CInfoWindow *dependant; //it may be dialog asking whther upgrade/dismiss stack (if opened)
	std::vector<SComponent*> upgResCost; //cost of upgrade (if not possible then empty)

	AdventureMapButton *dismiss, *upgrade, *ok;
	CCreInfoWindow(int Cid, int Type, int creatureCount, StackState *State, boost::function<void()> Upg, boost::function<void()> Dsm, UpgradeInfo *ui); //c-tor
	~CCreInfoWindow(); //d-tor
	void activate();
	void close();
	void clickRight(boost::logic::tribool down); //call-in
	void dismissF();
	void keyPressed (const SDL_KeyboardEvent & key); //call-in
	void deactivate();
	void show(SDL_Surface * to = NULL);
	void onUpgradeYes();
	void onUpgradeNo();
};

class CLevelWindow : public IShowable, public CIntObject
{
public:
	int heroType;
	SDL_Surface *bitmap; //background
	std::vector<CSelectableComponent *> comps; //skills to select
	AdventureMapButton *ok;
	boost::function<void(ui32)> cb;

	void close();
	CLevelWindow(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback); //c-tor
	~CLevelWindow(); //d-tor
	void activate();
	void deactivate();
	void selectionChanged(unsigned to);
	void show(SDL_Surface * to = NULL);
};

class CMinorResDataBar : public IShowable, public CIntObject
{
public:
	SDL_Surface *bg; //background bitmap
	void show(SDL_Surface * to=NULL);
	CMinorResDataBar(); //c-tor
	~CMinorResDataBar(); //d-tor
};

class CMarketplaceWindow : public IShowActivable, public CIntObject
{
public:
	class CTradeableItem : public ClickableL
	{
	public:
		int type; //0 - res, 1 - artif big, 2 - artif small, 3 - player flag
		int id;
		bool left;
		CFunctionList<void()> callback;

		void activate();
		void deactivate();
		void show(SDL_Surface * to=NULL);
		void clickLeft(boost::logic::tribool down);
		SDL_Surface *getSurface();
		CTradeableItem(int Type, int ID, bool Left);
	};

	SDL_Surface *bg; //background
	std::vector<CTradeableItem*> left, right;
	std::vector<std::string> rSubs; //offer caption
	CTradeableItem *hLeft, *hRight; //highlighted items (NULL if no highlight)

	int mode,//0 - res<->res; 1 - res<->plauer; 2 - buy artifact; 3 - sell artifact
		r1, r2; //TODO: comment me
	AdventureMapButton *ok, *max, *deal;
	CSlider *slider;

	void activate();
	void deactivate();
	void show(SDL_Surface * to=NULL);
	void setMax();
	void sliderMoved(int to);
	void makeDeal();
	void selectionChanged(bool side); //true == left
	CMarketplaceWindow(int Mode=0); //c-tor
	~CMarketplaceWindow(); //d-tor
	void setMode(int mode); //mode setter
	void clear();
};

class CSystemOptionsWindow : public IShowActivable, public CIntObject
{
private:
	SDL_Surface * background; //background of window
	AdventureMapButton *load, *save, *restart, *mainMenu, * quitGame, * backToMap; //load, restart and main menu are not used yet
	CHighlightableButtonsGroup * heroMoveSpeed;
	CHighlightableButtonsGroup * mapScrollSpeed;
public:
	CSystemOptionsWindow(const SDL_Rect & pos, CPlayerInterface * owner); //c-tor
	~CSystemOptionsWindow(); //d-tor

	//functions bound to buttons
	void bsavef(); //save game
	void bquitf(); //quit game
	void breturnf(); //return to game

	void activate();
	void deactivate();
	void show(SDL_Surface * to = NULL);
};

class CTavernWindow : public IShowActivable, public CIntObject
{
public:
	class HeroPortrait : public ClickableL, public ClickableR, public Hoverable
	{
	public:
		std::string hoverName;
		vstd::assigner<int,int> as;
		const CGHeroInstance *h;
		void activate();
		void deactivate();
		void clickLeft(boost::logic::tribool down);
		void clickRight(boost::logic::tribool down);
		void hover (bool on);
		HeroPortrait(int &sel, int id, int x, int y, const CGHeroInstance *H);
		void show(SDL_Surface * to = NULL);
	} h1, h2; //recruitable heroes

	SDL_Surface *bg; //background
	CStatusBar *bar;
	int selected;//0 (left) or 1 (right)

	AdventureMapButton *thiefGuild, *cancel, *recruit;

	CTavernWindow(const CGHeroInstance *H1, const CGHeroInstance *H2, const std::string &gossip); //c-tor
	~CTavernWindow(); //d-tor

	void recruitb();
	void close();
	void activate();
	void deactivate();
	void show(SDL_Surface * to = NULL);
};

class CInGameConsole : public IShowActivable, public KeyInterested
{
private:
	std::list< std::pair< std::string, int > > texts; //<text to show, time of add>
	std::vector< std::string > previouslyEntered; //previously entered texts, for up/down arrows to work
	int prevEntDisp; //displayed entry from previouslyEntered - if none it's -1
	int defaultTimeout; //timeout for new texts (in ms)
	int maxDisplayedTexts; //hiw many texts can be displayed simultaneously
public:
	std::string enteredText;
	void activate();
	void deactivate();
	void show(SDL_Surface * to = NULL);
	void print(const std::string &txt);
	void keyPressed (const SDL_KeyboardEvent & key); //call-in

	void startEnteringText();
	void endEnteringText(bool printEnteredText);
	void refreshEnteredText();

	CInGameConsole(); //c-tor
};

class CGarrisonWindow : public IShowActivable, public CIntObject
{
public:
	CGarrisonInt *garr;
	SDL_Surface *bg;
	AdventureMapButton *split, *quit;

	void close();
	void activate();
	void deactivate();
	void show(SDL_Surface * to = NULL);
	CGarrisonWindow(const CArmedInstance *up, const CGHeroInstance *down);
	~CGarrisonWindow();
};

extern CPlayerInterface * LOCPLINT;


#endif // __CPLAYERINTERFACE_H__
