#ifndef __CBATTLEINTERFACE_H__
#define __CBATTLEINTERFACE_H__

#include "../global.h"
#include <list>
#include "GUIBase.h"

/*
 * CBattleInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CCreatureSet;
class CGHeroInstance;
class CDefHandler;
class CStack;
class CCallback;
class AdventureMapButton;
class CHighlightableButton;
class CHighlightableButtonsGroup;
struct BattleResult;
struct SpellCast;
template <typename T> struct CondSh;
struct SetStackEffect;;
struct BattleAction;
class CGTownInstance;

class CBattleInterface;

class CBattleHero : public CIntObject
{
public:
	bool flip; //false if it's attacking hero, true otherwise
	CDefHandler * dh, *flag; //animation and flag
	const CGHeroInstance * myHero; //this animation's hero instance
	const CBattleInterface * myOwner; //battle interface to which this animation is assigned
	int phase; //stage of animation
	int nextPhase; //stage of animation to be set after current phase is fully displayed
	int image; //frame of animation
	unsigned char flagAnim, flagAnimCount; //for flag animation
	void show(SDL_Surface * to); //prints next frame of animation to to
	void activate();
	void deactivate();
	void setPhase(int newPhase); //sets phase of hero animation
	void clickLeft(tribool down, bool previousState); //call-in
	CBattleHero(const std::string & defName, int phaseG, int imageG, bool filpG, unsigned char player, const CGHeroInstance * hero, const CBattleInterface * owner); //c-tor
	~CBattleHero(); //d-tor
};

class CBattleHex : public CIntObject
{
private:
	bool setAlterText; //if true, this hex has set alternative text in console and will clean it
public:
	unsigned int myNumber; //number of hex in commonly used format
	bool accesible; //if true, this hex is accessible for units
	//CStack * ourStack;
	bool hovered, strictHovered; //for determining if hex is hovered by mouse (this is different problem than hex's graphic hovering)
	CBattleInterface * myInterface; //interface that owns me
	static std::pair<int, int> getXYUnitAnim(const int & hexNum, const bool & attacker, const CStack * creature); //returns (x, y) of left top corner of animation
	//for user interactions
	void hover (bool on);
	void activate();
	void deactivate();
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	CBattleHex();
};

class CBattleObstacle
{
	std::vector<int> lockedHexes;
};

class CBattleConsole : public CIntObject
{
private:
	std::vector< std::string > texts; //a place where texts are stored
	int lastShown; //last shown line of text
public:
	std::string alterTxt; //if it's not empty, this text is displayed
	std::string ingcAlter; //alternative text set by in-game console - very important!
	int whoSetAlter; //who set alter text; 0 - battle interface or none, 1 - button
	CBattleConsole(); //c-tor
	~CBattleConsole(); //d-tor
	void show(SDL_Surface * to = 0);
	bool addText(const std::string & text); //adds text at the last position; returns false if failed (e.g. text longer than 70 characters)
	void eraseText(unsigned int pos); //erases added text at position pos
	void changeTextAt(const std::string & text, unsigned int pos); //if we have more than pos texts, pos-th is changed to given one
	void scrollUp(unsigned int by = 1); //scrolls console up by 'by' positions
	void scrollDown(unsigned int by = 1); //scrolls console up by 'by' positions
};

class CBattleResultWindow : public CIntObject
{
private:
	SDL_Surface * background;
	AdventureMapButton * exit;
public:
	CBattleResultWindow(const BattleResult & br, const SDL_Rect & pos, const CBattleInterface * owner); //c-tor
	~CBattleResultWindow(); //d-tor

	void bExitf(); //exit button callback

	void activate();
	void deactivate();
	void show(SDL_Surface * to = 0);
};

class CBattleOptionsWindow : public CIntObject
{
private:
	CBattleInterface * myInt;
	SDL_Surface * background;
	AdventureMapButton * setToDefault, * exit;
	CHighlightableButton * viewGrid, * movementShadow, * mouseShadow;
	CHighlightableButtonsGroup * animSpeeds;
public:
	CBattleOptionsWindow(const SDL_Rect & position, CBattleInterface * owner); //c-tor
	~CBattleOptionsWindow(); //d-tor

	void bDefaultf(); //dafault button callback
	void bExitf(); //exit button callback

	void activate();
	void deactivate();
	void show(SDL_Surface * to = 0);
};

struct BattleSettings
{
	BattleSettings()
	{
		printCellBorders = true;
		printStackRange = true;
		animSpeed = 2;
		printMouseShadow = true;
	}
	bool printCellBorders; //if true, cell borders will be printed
	bool printStackRange; //if true,range of active stack will be printed
	int animSpeed; //speed of animation; 1 - slowest, 2 - medium, 4 - fastest
	bool printMouseShadow; //if true, hex under mouse will be shaded


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & printCellBorders & printStackRange & printMouseShadow;
	}
};

class CBattleInterface : public CIntObject
{
private:
	SDL_Surface * background, * menu, * amountNormal, * amountNegative, * amountPositive, * amountEffNeutral, * cellBorders, * backgroundWithHexes;
	AdventureMapButton * bOptions, * bSurrender, * bFlee, * bAutofight, * bSpell,
		* bWait, * bDefence, * bConsoleUp, * bConsoleDown;
	CBattleConsole * console;
	CBattleHero * attackingHero, * defendingHero; //fighting heroes
	CCreatureSet * army1, * army2; //fighting armies
	CGHeroInstance * attackingHeroInstance, * defendingHeroInstance;
	std::map< int, CCreatureAnimation * > creAnims; //animations of creatures from fighting armies (order by BattleInfo's stacks' ID)
	std::map< int, CDefHandler * > idToProjectile; //projectiles of creaures (creatureID, defhandler)
	std::map< int, CDefHandler * > idToObstacle; //obstacles located on the battlefield
	std::map< int, bool > creDir; // <creatureID, if false reverse creature's animation>
	unsigned char animCount;
	int activeStack; //number of active stack; -1 - no one
	int mouseHoveredStack; //stack hovered by mouse; if -1 -> none
	std::vector<int> shadedHexes; //hexes available for active stack
	int previouslyHoveredHex; //number of hex that was hovered by the cursor a while ago
	int currentlyHoveredHex; //number of hex that is supposed to be hovered (for a while it may be inappropriately set, but will be renewed soon)
	float getAnimSpeedMultiplier() const; //returns multiplier for number of frames in a group
	std::map<int, int> standingFrame; //number of frame in standing animation by stack ID, helps in showing 'random moves'

	bool spellDestSelectMode; //if true, player is choosing destination for his spell
	int spellSelMode; //0 - any location, 1 - any firendly creature, 2 - any hostile creature, 3 - any creature, 4 - obstacle,z -1 - no location
	BattleAction * spellToCast; //spell for which player is choosing destination
	void endCastingSpell(); //ends casting spell (eg. when spell has been cast or cancelled)

	class CAttHelper
	{
	public:
		int ID; //attacking stack
		int IDby; //attacked stack
		int dest; //atacked hex
		int frame, maxframe; //frame of animation, number of frames of animation
		int hitCount; //for delaying animation
		bool reversing;
		int posShiftDueToDist;
		bool shooting;
		int shootingGroup; //if shooting is true, print this animation group
		int sh;			   // temporary sound handler
	} * attackingInfo;
	void attackingShowHelper();
	void showAliveStack(int ID, const std::map<int, CStack> & stacks, SDL_Surface * to); //helper function for function show
	void showPieceOfWall(SDL_Surface * to, int hex); //helper function for show
	void redrawBackgroundWithHexes(int activeStack);
	void printConsoleAttacked(int ID, int dmg, int killed, int IDby);

	struct SProjectileInfo
	{
		int x, y; //position on the screen
		int dx, dy; //change in position in one step
		int step, lastStep; //to know when finish showing this projectile
		int creID; //ID of creature that shot this projectile
		int frameNum; //frame to display form projectile animation
		bool spin; //if true, frameNum will be increased
		int animStartDelay; //how many times projectile must be attempted to be shown till it's really show (decremented after hit)
		bool reverse; //if true, projectile will be flipped by vertical asix
	};
	std::list<SProjectileInfo> projectiles; //projectiles flying on battlefield
	void projectileShowHelper(SDL_Surface * to); //prints projectiles present on the battlefield
	void giveCommand(ui8 action, ui16 tile, ui32 stack, si32 additional=-1);
	bool isTileAttackable(const int & number) const; //returns true if tile 'number' is neighbouring any tile from active stack's range or is one of these tiles
	bool blockedByObstacle(int hex) const;
	bool isCatapultAttackable(int hex) const; //returns true if given tile can be attacked by catapult

	void handleEndOfMove(int stackNumber, int destinationTile); //helper function

	struct SBattleEffect
	{
		int x, y; //position on the screen
		int frame, maxFrame;
		CDefHandler * anim; //animation to display
	};
	std::list<SBattleEffect> battleEffects; //different animations to display on the screen like spell effects

	class SiegeHelper
	{
	private:
		static std::string townTypeInfixes[F_NUMBER]; //for internal use only - to build filenames
		SDL_Surface * walls[13];
		const CBattleInterface * owner;
	public:
		const CGTownInstance * town; //besieged town
		SiegeHelper(const CGTownInstance * siegeTown, const CBattleInterface * _owner); //c-tor
		~SiegeHelper(); //d-tor

		//filename getters
		std::string getSiegeName(ui16 what, ui16 additInfo = 1) const; //what: 0 - background, 1 - background wall, 2 - keep, 3 - bottom tower, 4 - bottom wall, 5 - below gate, 6 - over gate, 7 - upper wall, 8 - uppert tower, 9 - gate, 10 - gate arch, 11 - bottom static wall, 12 - upper static wall, 13 - moat, 14 - mlip; additInfo: 1 - intact, 2 - damaged, 3 - destroyed

		void printPartOfWall(SDL_Surface * to, int what);//what: 1 - background wall, 2 - keep, 3 - bottom tower, 4 - bottom wall, 5 - below gate, 6 - over gate, 7 - upper wall, 8 - uppert tower, 9 - gate, 10 - gate arch, 11 - bottom static wall, 12 - upper static wall

		friend class CPlayerInterface;
	} * siegeH;
public:
	CBattleInterface(CCreatureSet * army1, CCreatureSet * army2, CGHeroInstance *hero1, CGHeroInstance *hero2, const SDL_Rect & myRect); //c-tor
	~CBattleInterface(); //d-tor

	//std::vector<TimeInterested*> timeinterested; //animation handling
	static BattleSettings settings;
	void setPrintCellBorders(bool set); //if true, cell borders will be printed
	void setPrintStackRange(bool set); //if true,range of active stack will be printed
	void setPrintMouseShadow(bool set); //if true, hex under mouse will be shaded
	void setAnimSpeed(int set); //speed of animation; 1 - slowest, 2 - medium, 4 - fastest
	int getAnimSpeed() const; //speed of animation; 1 - slowest, 2 - medium, 4 - fastest

	CBattleHex bfield[BFIELD_SIZE]; //11 lines, 17 hexes on each
	//std::vector< CBattleObstacle * > obstacles; //vector of obstacles on the battlefield
	SDL_Surface * cellBorder, * cellShade;
	CondSh<BattleAction *> *givenCommand; //data != NULL if we have i.e. moved current unit
	bool myTurn; //if true, interface is active (commands can be ordered
	CBattleResultWindow * resWindow; //window of end of battle
	bool showStackQueue; //if true, queue of stacks will be shown

	bool moveStarted; //if true, the creature that is already moving is going to make its first step
	int moveSh;		  // sound handler used when moving a unit

	//button handle funcs:
	void bOptionsf();
	void bSurrenderf();
	void bFleef();
	void reallyFlee(); //performs fleeing without asking player
	void bAutofightf();
	void bSpellf();
	void bWaitf();
	void bDefencef();
	void bConsoleUpf();
	void bConsoleDownf();
	//end of button handle funcs
	//napisz tu klase odpowiadajaca za wyswietlanie bitwy i obsluge uzytkownika, polecenia ma przekazywac callbackiem
	void activate();
	void deactivate();
	void show(SDL_Surface * to);
	void keyPressed(const SDL_KeyboardEvent & key);
	void mouseMoved(const SDL_MouseMotionEvent &sEvent);
	void clickRight(tribool down, bool previousState);

	bool reverseCreature(int number, int hex, bool wideTrick = false); //reverses animation of given creature playing animation of reversing
	void handleStartMoving(int number); //animation of starting move; some units don't have this animation (ie. halberdier)

	struct SStackAttackedInfo
	{
		int ID; //id of attacked stack
		int dmg; //damage dealt
		int amountKilled; //how many creatures in stack has been killed
		int IDby; //ID of attacking stack
		bool byShooting; //if true, stack has been attacked by shooting
		bool killed; //if true, stack has been killed
	};

	//call-ins
	void newStack(int stackID); //new stack appeared on battlefield
	void stackRemoved(int stackID); //stack disappeared from batlefiled
	//void stackKilled(int ID, int dmg, int killed, int IDby, bool byShooting); //stack has been killed (but corpses remain)
	void stackActivated(int number); //active stack has been changed
	void stackMoved(int number, int destHex, bool endMoving, int distance); //stack with id number moved to destHex
	void stacksAreAttacked(std::vector<SStackAttackedInfo> attackedInfos); //called when a certain amount of stacks has been attacked
	void stackAttacking(int ID, int dest); //called when stack with id ID is attacking something on hex dest
	void newRound(int number); //caled when round is ended; number is the number of round
	void hexLclicked(int whichOne); //hex only call-in
	void stackIsShooting(int ID, int dest); //called when stack with id ID is shooting to hex dest
	void battleFinished(const BattleResult& br); //called when battle is finished - battleresult window should be printed
	void spellCast(SpellCast * sc); //called when a hero casts a spell
	void battleStacksEffectsSet(const SetStackEffect & sse); //called when a specific effect is set to stacks
	void castThisSpell(int spellID); //called when player has chosen a spell from spellbook
	void displayEffect(ui32 effect, int destTile); //displays effect of a spell on the battlefield; affected: true - attacker. false - defender

	friend class CBattleHex;
	friend class CBattleResultWindow;
	friend class CPlayerInterface;
	friend class AdventureMapButton;
	friend class CInGameConsole;
};

#endif // __CBATTLEINTERFACE_H__
