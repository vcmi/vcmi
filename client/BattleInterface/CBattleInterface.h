#pragma once


#include "../../lib/CCreatureSet.h"
#include "../../lib/ConstTransitivePtr.h" //may be reundant
#include "../CAnimation.h"
#include "../../lib/GameConstants.h"

/*
 * CBattleInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CLabel;
class CCreatureSet;
class CGHeroInstance;
class CDefHandler;
class CStack;
class CCallback;
class CAdventureMapButton;
class CHighlightableButton;
class CHighlightableButtonsGroup;
struct BattleResult;
struct BattleSpellCast;
struct CObstacleInstance;
template <typename T> struct CondSh;
struct SetStackEffect;;
struct BattleAction;
class CGTownInstance;
struct CatapultAttack;
struct CatapultProjectileInfo;
struct BattleTriggerEffect;
class CBattleAnimation;
class CBattleHero;
class CBattleConsole;
class CBattleResultWindow;
class CStackQueue;
class CPlayerInterface;
class CCreatureAnimation;
struct ProjectileInfo;
class CClickableHex;
struct BattleHex;
struct InfoAboutHero;

/// Class which manages the locked hex fields that are blocked e.g. by obstacles
class CBattleObstacle
{
	std::vector<int> lockedHexes;
};

/// Small struct which contains information about the id of the attacked stack, the damage dealt,...
struct StackAttackedInfo
{
	const CStack * defender; //attacked stack
	unsigned int dmg; //damage dealt
	unsigned int amountKilled; //how many creatures in stack has been killed
	const CStack * attacker; //attacking stack
	bool byShooting; //if true, stack has been attacked by shooting
	bool killed; //if true, stack has been killed
	bool rebirth; //if true, play rebirth animation after all
	bool cloneKilled;
};

/// Struct for battle effect animation e.g. morale, prayer, armageddon, bless,...
struct BattleEffect
{
	int x, y; //position on the screen
	int frame, maxFrame;
	CDefHandler * anim; //animation to display
	int effectID; //uniqueID equal ot ID of appropriate CSpellEffectAnim
};

/// Small struct which is needed for drawing the parabolic trajectory of the catapult cannon
struct CatapultProjectileInfo
{
	const double facA, facB, facC;
	const int fromX, toX;

	CatapultProjectileInfo() : facA(0), facB(0), facC(0), fromX(0), toX(0) { };
	CatapultProjectileInfo(double factorA, double factorB, double factorC, int fromXX, int toXX) : facA(factorA), facB(factorB), facC(factorC),
		fromX(fromXX), toX(toXX) { };

	double calculateY(double x);
};

/// Big class which handles the overall battle interface actions and it is also responsible for
/// drawing everything correctly.
class CBattleInterface : public CIntObject
{
	enum PossibleActions // actions performed at l-click
	{
		INVALID = -1, CREATURE_INFO,
		MOVE_TACTICS, CHOOSE_TACTICS_STACK,
		MOVE_STACK, ATTACK, WALK_AND_ATTACK, ATTACK_AND_RETURN, SHOOT, //OPEN_GATE, //we can open castle gate during siege
		NO_LOCATION, ANY_LOCATION, FRIENDLY_CREATURE_SPELL, HOSTILE_CREATURE_SPELL, RISING_SPELL, ANY_CREATURE, OBSTACLE, TELEPORT, SACRIFICE, RANDOM_GENIE_SPELL,
		FREE_LOCATION, //used with Force Field and Fire Wall - all tiles affected by spell must be free
		CATAPULT, HEAL, RISE_DEMONS
	};
private:
	SDL_Surface * background, * menu, * amountNormal, * amountNegative, * amountPositive, * amountEffNeutral, * cellBorders, * backgroundWithHexes;
	CAdventureMapButton * bOptions, * bSurrender, * bFlee, * bAutofight, * bSpell,
		* bWait, * bDefence, * bConsoleUp, * bConsoleDown, *btactNext, *btactEnd;
	CBattleConsole * console;
	CBattleHero * attackingHero, * defendingHero; //fighting heroes
	CStackQueue *queue;
	const CCreatureSet *army1, *army2; //copy of initial armies (for result window)
	const CGHeroInstance * attackingHeroInstance, * defendingHeroInstance;
	std::map< int, CCreatureAnimation * > creAnims; //animations of creatures from fighting armies (order by BattleInfo's stacks' ID)
	std::map< int, CDefHandler * > idToProjectile; //projectiles of creatures (creatureID, defhandler)
	std::map< int, CDefHandler * > idToObstacle; //obstacles located on the battlefield
	std::map< int, SDL_Surface * > idToAbsoluteObstacle; //obstacles located on the battlefield

	//TODO these should be loaded only when needed (and freed then) but I believe it's rather work for resource manager,
	//so I didn't implement that (having ongoing RM development)
	CDefHandler *landMine;
	CDefHandler *quicksand;
	CDefHandler *fireWall;
	CDefHandler *smallForceField[2], *bigForceField[2]; // [side]

	std::map< int, bool > creDir; // <creatureID, if false reverse creature's animation>
	ui8 animCount;
	const CStack * activeStack; //number of active stack; NULL - no one
	const CStack * stackToActivate; //when animation is playing, we should wait till the end to make the next stack active; NULL of none
	const CStack * selectedStack; //for Teleport / Sacrifice
	void activateStack(); //sets activeStack to stackToActivate etc.
	int mouseHoveredStack; //stack hovered by mouse; if -1 -> none
    time_t lastMouseHoveredStackAnimationTime; // time when last mouse hovered animation occurred
    static const time_t HOVER_ANIM_DELTA;
	std::vector<BattleHex> occupyableHexes, //hexes available for active stack
		attackableHexes; //hexes attackable by active stack
    bool stackCountOutsideHexes[GameConstants::BFIELD_SIZE]; // hexes that when in front of a unit cause it's amount box to move back
	int previouslyHoveredHex; //number of hex that was hovered by the cursor a while ago
	int currentlyHoveredHex; //number of hex that is supposed to be hovered (for a while it may be inappropriately set, but will be renewed soon)
	int attackingHex; //hex from which the stack would perform attack with current cursor
	double getAnimSpeedMultiplier() const; //returns multiplier for number of frames in a group
	std::map<int, int> standingFrame; //number of frame in standing animation by stack ID, helps in showing 'random moves'

	CPlayerInterface * tacticianInterface; //used during tactics mode, points to the interface of player with higher tactics (can be either attacker or defender in hot-seat), valid onloy for human players
	bool tacticsMode;
	bool stackCanCastSpell; //if true, active stack could possibly cats some target spell
	bool creatureCasting; //if true, stack currently aims to cats a spell
	bool spellDestSelectMode; //if true, player is choosing destination for his spell - only for GUI / console
	PossibleActions spellSelMode;
	BattleAction * spellToCast; //spell for which player is choosing destination
	const CSpell * sp; //spell pointer for convenience
	si32 creatureSpellToCast;
	std::vector<PossibleActions> possibleActions; //all actions possible to call at the moment by player
	std::vector<PossibleActions> localActions; //actions possible to take on hovered hex
	std::vector<PossibleActions> illegalActions; //these actions display message in case of illegal target
	PossibleActions currentAction; //action that will be performed on l-click
	PossibleActions selectedAction; //last action chosen (and saved) by player
	PossibleActions illegalAction; //most likely action that can't be performed here

	void getPossibleActionsForStack (const CStack * stack); //called when stack gets its turn
	void endCastingSpell(); //ends casting spell (eg. when spell has been cast or canceled)

	void showAliveStack(const CStack *stack, SDL_Surface * to); //helper function for function show
	void showAliveStacks(std::vector<const CStack *> *aliveStacks, int hex, std::vector<const CStack *> *flyingStacks, SDL_Surface *to); // loops through all stacks at a given hex position
	void showPieceOfWall(SDL_Surface * to, int hex, const std::vector<const CStack*> & stacks); //helper function for show
	void showObstacles(std::multimap<BattleHex, int> *hexToObstacle, std::vector<shared_ptr<const CObstacleInstance> > &obstacles, int hex, SDL_Surface *to); // show all obstacles at a given hex position
	SDL_Surface *imageOfObstacle(const CObstacleInstance &oi) const;
	Point whereToBlitObstacleImage(SDL_Surface *image, const CObstacleInstance &obstacle) const;
	void redrawBackgroundWithHexes(const CStack * activeStack);
	void printConsoleAttacked(const CStack * defender, int dmg, int killed, const CStack * attacker, bool Multiple);

	std::list<ProjectileInfo> projectiles; //projectiles flying on battlefield
	void projectileShowHelper(SDL_Surface * to); //prints projectiles present on the battlefield
	void giveCommand(ui8 action, BattleHex tile, ui32 stackID, si32 additional=-1, si32 selectedStack = -1);
	bool isTileAttackable(const BattleHex & number) const; //returns true if tile 'number' is neighboring any tile from active stack's range or is one of these tiles
	bool isCatapultAttackable(BattleHex hex) const; //returns true if given tile can be attacked by catapult

	std::list<BattleEffect> battleEffects; //different animations to display on the screen like spell effects

	/// Class which is responsible for drawing the wall of a siege during battle
	class SiegeHelper
	{
	private:
		static std::string townTypeInfixes[GameConstants::F_NUMBER]; //for internal use only - to build filenames
		SDL_Surface* walls[18];
		const CBattleInterface * owner;
	public:
		const CGTownInstance * town; //besieged town
		
		SiegeHelper(const CGTownInstance * siegeTown, const CBattleInterface * _owner); //c-tor
		~SiegeHelper(); //d-tor

		//filename getters
		std::string getSiegeName(ui16 what, ui16 additInfo = 1) const; //what: 0 - background, 1 - background wall, 2 - keep, 3 - bottom tower, 4 - bottom wall, 5 - below gate, 6 - over gate, 7 - upper wall, 8 - uppert tower, 9 - gate, 10 - gate arch, 11 - bottom static wall, 12 - upper static wall, 13 - moat, 14 - mlip, 15 - keep creature cover, 16 - bottom turret creature cover, 17 - upper turret creature cover; additInfo: 1 - intact, 2 - damaged, 3 - destroyed

		void printPartOfWall(SDL_Surface * to, int what);//what: 1 - background wall, 2 - keep, 3 - bottom tower, 4 - bottom wall, 5 - below gate, 6 - over gate, 7 - upper wall, 8 - uppert tower, 9 - gate, 10 - gate arch, 11 - bottom static wall, 12 - upper static wall, 15 - keep creature cover, 16 - bottom turret creature cover, 17 - upper turret creature cover

		friend class CBattleInterface;
	} * siegeH;

	CPlayerInterface * attackerInt, * defenderInt; //because LOCPLINT is not enough in hotSeat
	const CGHeroInstance * getActiveHero(); //returns hero that can currently cast a spell
public:
	CPlayerInterface * curInt; //current player interface
	std::list<std::pair<CBattleAnimation *, bool> > pendingAnims; //currently displayed animations <anim, initialized>
	void addNewAnim(CBattleAnimation * anim); //adds new anim to pendingAnims
	ui32 animIDhelper; //for giving IDs for animations
	static CondSh<bool> animsAreDisplayed; //for waiting with the end of battle for end of anims

	CBattleInterface(const CCreatureSet * army1, const CCreatureSet * army2, CGHeroInstance *hero1, CGHeroInstance *hero2, const SDL_Rect & myRect, CPlayerInterface * att, CPlayerInterface * defen); //c-tor
	~CBattleInterface(); //d-tor

	//std::vector<TimeInterested*> timeinterested; //animation handling
	void setPrintCellBorders(bool set); //if true, cell borders will be printed
	void setPrintStackRange(bool set); //if true,range of active stack will be printed
	void setPrintMouseShadow(bool set); //if true, hex under mouse will be shaded
	void setAnimSpeed(int set); //speed of animation; 1 - slowest, 2 - medium, 4 - fastest
	int getAnimSpeed() const; //speed of animation; 1 - slowest, 2 - medium, 4 - fastest

	std::vector<CClickableHex*> bfield; //11 lines, 17 hexes on each
	//std::vector< CBattleObstacle * > obstacles; //vector of obstacles on the battlefield
	SDL_Surface * cellBorder, * cellShade;
	CondSh<BattleAction *> *givenCommand; //data != NULL if we have i.e. moved current unit
	bool myTurn; //if true, interface is active (commands can be ordered)
	CBattleResultWindow * resWindow; //window of end of battle

	bool moveStarted; //if true, the creature that is already moving is going to make its first step
	int moveSh;		  // sound handler used when moving a unit

	//button handle funcs:
	void bOptionsf();
	void bSurrenderf();
	void bFleef();
	void reallyFlee(); //performs fleeing without asking player
	void reallySurrender(); //performs surrendering without asking player
	void bAutofightf();
	void bSpellf();
	void bWaitf();
	void bDefencef();
	void bConsoleUpf();
	void bConsoleDownf();
	void bTacticNextStack(const CStack *current = NULL);
	void bEndTacticPhase();
	//end of button handle funcs
	//napisz tu klase odpowiadajaca za wyswietlanie bitwy i obsluge uzytkownika, polecenia ma przekazywac callbackiem
	void activate();
	void deactivate();
	void showAll(SDL_Surface * to);
	void show(SDL_Surface * to);
	void keyPressed(const SDL_KeyboardEvent & key);
	void mouseMoved(const SDL_MouseMotionEvent &sEvent);
	void clickRight(tribool down, bool previousState);

	//call-ins
	void startAction(const BattleAction* action);
	void newStack(const CStack * stack); //new stack appeared on battlefield
	void stackRemoved(int stackID); //stack disappeared from batlefiled
	void stackActivated(const CStack * stack); //active stack has been changed
	void stackMoved(const CStack * stack, std::vector<BattleHex> destHex, int distance); //stack with id number moved to destHex
	void waitForAnims();
	void stacksAreAttacked(std::vector<StackAttackedInfo> attackedInfos); //called when a certain amount of stacks has been attacked
	void stackAttacking(const CStack * attacker, BattleHex dest, const CStack * attacked, bool shooting); //called when stack with id ID is attacking something on hex dest
	void newRoundFirst( int round );
	void newRound(int number); //caled when round is ended; number is the number of round
	void hexLclicked(int whichOne); //hex only call-in
	void stackIsCatapulting(const CatapultAttack & ca); //called when a stack is attacking walls
	void battleFinished(const BattleResult& br); //called when battle is finished - battleresult window should be printed
	const BattleResult * bresult; //result of a battle; if non-zero then display when all animations end
	void displayBattleFinished(); //displays battle result
	void spellCast(const BattleSpellCast * sc); //called when a hero casts a spell
	void battleStacksEffectsSet(const SetStackEffect & sse); //called when a specific effect is set to stacks
	void castThisSpell(int spellID); //called when player has chosen a spell from spellbook
	void displayEffect(ui32 effect, int destTile); //displays effect of a spell on the battlefield; affected: true - attacker. false - defender
	void battleTriggerEffect(const BattleTriggerEffect & bte);
	void setBattleCursor(const int myNumber); //really complex and messy, sets attackingHex
	void endAction(const BattleAction* action);
	void hideQueue();
	void showQueue();
	PossibleActions selectionTypeByPositiveness(const CSpell & spell);
	Rect hexPosition(BattleHex hex) const;

	void handleHex(BattleHex myNumber, int eventType);
	bool isCastingPossibleHere (const CStack * sactive, const CStack * shere, BattleHex myNumber);
	bool canStackMoveHere (const CStack * sactive, BattleHex MyNumber); //TODO: move to BattleState / callback

	BattleHex fromWhichHexAttack(BattleHex myNumber);
	void obstaclePlaced(const CObstacleInstance & oi);

	const CGHeroInstance * currentHero() const;
	InfoAboutHero enemyHero() const;

	friend class CPlayerInterface;
	friend class CAdventureMapButton;
	friend class CInGameConsole;
	
	friend class CBattleResultWindow;
	friend class CBattleHero;
	friend class CSpellEffectAnimation;
	friend class CBattleStackAnimation;
	friend class CReverseAnimation;
	friend class CDefenceAnimation;
	friend class CMovementAnimation;
	friend class CMovementStartAnimation;
	friend class CAttackAnimation;
	friend class CMeleeAttackAnimation;
	friend class CShootingAnimation;
	friend class CClickableHex;
};
