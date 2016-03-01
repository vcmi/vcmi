#pragma once

#include "../../lib/ConstTransitivePtr.h" //may be reundant
#include "../../lib/GameConstants.h"

#include "CBattleAnimations.h"

#include "../../lib/spells/CSpellHandler.h" //CSpell::TAnimation

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
class CButton;
class CToggleButton;
class CToggleGroup;
struct BattleResult;
struct BattleSpellCast;
struct CObstacleInstance;
template <typename T> struct CondSh;
struct SetStackEffect;
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
struct BattleAction;
class CBattleGameInterface;

/// Small struct which contains information about the id of the attacked stack, the damage dealt,...
struct StackAttackedInfo
{
	const CStack * defender; //attacked stack
	unsigned int dmg; //damage dealt
	unsigned int amountKilled; //how many creatures in stack has been killed
	const CStack * attacker; //attacking stack
	bool indirectAttack; //if true, stack was attacked indirectly - spell or ranged attack
	bool killed; //if true, stack has been killed
	bool rebirth; //if true, play rebirth animation after all
	bool cloneKilled;
};

/// Struct for battle effect animation e.g. morale, prayer, armageddon, bless,...
struct BattleEffect
{
	int x, y; //position on the screen
	float currentFrame;
	int maxFrame;
	CDefHandler * anim; //animation to display
	int effectID; //uniqueID equal ot ID of appropriate CSpellEffectAnim
	BattleHex position; //Indicates if effect which hex the effect is drawn on
};

struct BattleObjectsByHex
{
	typedef std::vector<int> TWallList;
	typedef std::vector<const CStack * > TStackList;
	typedef std::vector<const BattleEffect *> TEffectList;
	typedef std::vector<std::shared_ptr<const CObstacleInstance> > TObstacleList;

	struct HexData
	{
		TWallList walls;
		TStackList dead;
		TStackList alive;
		TEffectList effects;
		TObstacleList obstacles;
	};

	HexData beforeAll;
	HexData afterAll;
	std::array<HexData, GameConstants::BFIELD_SIZE> hex;
};

/// Small struct which is needed for drawing the parabolic trajectory of the catapult cannon
struct CatapultProjectileInfo
{
	CatapultProjectileInfo(Point from, Point dest);

	double facA, facB, facC;

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
	CButton * bOptions, * bSurrender, * bFlee, * bAutofight, * bSpell,
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

	std::map< int, bool > creDir; // <creatureID, if false reverse creature's animation> //TODO: move it to battle callback
	ui8 animCount;
	const CStack * activeStack; //number of active stack; nullptr - no one
	const CStack * mouseHoveredStack; // stack below mouse pointer, used for border animation
	const CStack * stackToActivate; //when animation is playing, we should wait till the end to make the next stack active; nullptr of none
	const CStack * selectedStack; //for Teleport / Sacrifice
	void activateStack(); //sets activeStack to stackToActivate etc. //FIXME: No, it's not clear at all
	std::vector<BattleHex> occupyableHexes, //hexes available for active stack
		attackableHexes; //hexes attackable by active stack
	bool stackCountOutsideHexes[GameConstants::BFIELD_SIZE]; // hexes that when in front of a unit cause it's amount box to move back
	BattleHex previouslyHoveredHex; //number of hex that was hovered by the cursor a while ago
	BattleHex currentlyHoveredHex; //number of hex that is supposed to be hovered (for a while it may be inappropriately set, but will be renewed soon)
	int attackingHex; //hex from which the stack would perform attack with current cursor

	std::shared_ptr<CPlayerInterface> tacticianInterface; //used during tactics mode, points to the interface of player with higher tactics (can be either attacker or defender in hot-seat), valid onloy for human players
	bool tacticsMode;
	bool stackCanCastSpell; //if true, active stack could possibly cast some target spell
	bool creatureCasting; //if true, stack currently aims to cats a spell
	bool spellDestSelectMode; //if true, player is choosing destination for his spell - only for GUI / console
	BattleAction * spellToCast; //spell for which player is choosing destination
	const CSpell * sp; //spell pointer for convenience
	si32 creatureSpellToCast;
	std::vector<PossibleActions> possibleActions; //all actions possible to call at the moment by player
	std::vector<PossibleActions> localActions; //actions possible to take on hovered hex
	std::vector<PossibleActions> illegalActions; //these actions display message in case of illegal target
	PossibleActions currentAction; //action that will be performed on l-click
	PossibleActions selectedAction; //last action chosen (and saved) by player
	PossibleActions illegalAction; //most likely action that can't be performed here

	void setActiveStack(const CStack * stack);
	void setHoveredStack(const CStack * stack);

	void requestAutofightingAIToTakeAction();

	void getPossibleActionsForStack (const CStack * stack); //called when stack gets its turn
	void endCastingSpell(); //ends casting spell (eg. when spell has been cast or canceled)

	void printConsoleAttacked(const CStack * defender, int dmg, int killed, const CStack * attacker, bool Multiple);

	std::list<ProjectileInfo> projectiles; //projectiles flying on battlefield
	void giveCommand(Battle::ActionType action, BattleHex tile, ui32 stackID, si32 additional=-1, si32 selectedStack = -1);
	bool isTileAttackable(const BattleHex & number) const; //returns true if tile 'number' is neighboring any tile from active stack's range or is one of these tiles
	bool isCatapultAttackable(BattleHex hex) const; //returns true if given tile can be attacked by catapult

	std::list<BattleEffect> battleEffects; //different animations to display on the screen like spell effects

	/// Class which is responsible for drawing the wall of a siege during battle
	class SiegeHelper
	{
	private:
		SDL_Surface* walls[18];
		const CBattleInterface * owner;
	public:
		const CGTownInstance * town; //besieged town

		SiegeHelper(const CGTownInstance * siegeTown, const CBattleInterface * _owner); //c-tor
		~SiegeHelper(); //d-tor

		std::string getSiegeName(ui16 what) const;
		std::string getSiegeName(ui16 what, int state) const; // state uses EWallState enum

		void printPartOfWall(SDL_Surface * to, int what);

		enum EWallVisual
		{
			BACKGROUND = 0,
			BACKGROUND_WALL = 1,
			KEEP,
			BOTTOM_TOWER,
			BOTTOM_WALL,
			WALL_BELLOW_GATE,
			WALL_OVER_GATE,
			UPPER_WALL,
			UPPER_TOWER,
			GATE,
			GATE_ARCH,
			BOTTOM_STATIC_WALL,
			UPPER_STATIC_WALL,
			MOAT,
			BACKGROUND_MOAT,
			KEEP_BATTLEMENT,
			BOTTOM_BATTLEMENT,
			UPPER_BATTLEMENT
		};

		friend class CBattleInterface;
	} * siegeH;

	std::shared_ptr<CPlayerInterface> attackerInt, defenderInt; //because LOCPLINT is not enough in hotSeat
	std::shared_ptr<CPlayerInterface> curInt; //current player interface
	const CGHeroInstance * getActiveHero(); //returns hero that can currently cast a spell

	/** Methods for displaying battle screen */
	void showBackground(SDL_Surface * to);

	void showBackgroundImage(SDL_Surface * to);
	void showAbsoluteObstacles(SDL_Surface * to);
	void showHighlightedHexes(SDL_Surface * to);
	void showHighlightedHex(SDL_Surface * to, BattleHex hex);
	void showInterface(SDL_Surface * to);

	void showBattlefieldObjects(SDL_Surface * to);

	void showAliveStacks(SDL_Surface * to, std::vector<const CStack *> stacks);
	void showStacks(SDL_Surface * to, std::vector<const CStack *> stacks);
	void showObstacles(SDL_Surface *to, std::vector<std::shared_ptr<const CObstacleInstance> > &obstacles);
	void showPiecesOfWall(SDL_Surface * to, std::vector<int> pieces);

	void showBattleEffects(SDL_Surface *to, const std::vector<const BattleEffect *> &battleEffects);
	void showProjectiles(SDL_Surface * to);

	BattleObjectsByHex sortObjectsByHex();
	void updateBattleAnimations();

	SDL_Surface *getObstacleImage(const CObstacleInstance &oi);
	Point getObstaclePosition(SDL_Surface *image, const CObstacleInstance &obstacle);
	void redrawBackgroundWithHexes(const CStack * activeStack);
	/** End of battle screen blitting methods */

public:
	std::list<std::pair<CBattleAnimation *, bool> > pendingAnims; //currently displayed animations <anim, initialized>
	void addNewAnim(CBattleAnimation * anim); //adds new anim to pendingAnims
	ui32 animIDhelper; //for giving IDs for animations
	static CondSh<bool> animsAreDisplayed; //for waiting with the end of battle for end of anims

	CBattleInterface(const CCreatureSet * army1, const CCreatureSet * army2, const CGHeroInstance *hero1, const CGHeroInstance *hero2, const SDL_Rect & myRect, std::shared_ptr<CPlayerInterface> att, std::shared_ptr<CPlayerInterface> defen); //c-tor
	~CBattleInterface(); //d-tor

	//std::vector<TimeInterested*> timeinterested; //animation handling
	void setPrintCellBorders(bool set); //if true, cell borders will be printed
	void setPrintStackRange(bool set); //if true,range of active stack will be printed
	void setPrintMouseShadow(bool set); //if true, hex under mouse will be shaded
	void setAnimSpeed(int set); //speed of animation; range 1..100
	int getAnimSpeed() const; //speed of animation; range 1..100
	CPlayerInterface * getCurrentPlayerInterface() const;

	std::vector<CClickableHex*> bfield; //11 lines, 17 hexes on each
	SDL_Surface * cellBorder, * cellShade;
	CondSh<BattleAction *> *givenCommand; //data != nullptr if we have i.e. moved current unit
	bool myTurn; //if true, interface is active (commands can be ordered)
	CBattleResultWindow * resWindow; //window of end of battle

	bool moveStarted; //if true, the creature that is already moving is going to make its first step
	int moveSoundHander; // sound handler used when moving a unit

	const BattleResult * bresult; //result of a battle; if non-zero then display when all animations end

	// block all UI elements, e.g. during enemy turn
	// unlike activate/deactivate this method will correctly grey-out all elements
	void blockUI(bool on);

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
	void bTacticNextStack(const CStack *current = nullptr);
	void bEndTacticPhase();
	//end of button handle funcs
	//napisz tu klase odpowiadajaca za wyswietlanie bitwy i obsluge uzytkownika, polecenia ma przekazywac callbackiem
	void activate() override;
	void deactivate() override;
	void keyPressed(const SDL_KeyboardEvent & key) override;
	void mouseMoved(const SDL_MouseMotionEvent &sEvent) override;
	void clickRight(tribool down, bool previousState) override;

	void show(SDL_Surface *to) override;
	void showAll(SDL_Surface *to) override;

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
	void displayBattleFinished(); //displays battle result
	void spellCast(const BattleSpellCast * sc); //called when a hero casts a spell
	void battleStacksEffectsSet(const SetStackEffect & sse); //called when a specific effect is set to stacks
	void castThisSpell(SpellID spellID); //called when player has chosen a spell from spellbook
	void displayEffect(ui32 effect, int destTile, bool areaEffect = true); //displays custom effect on the battlefield

	void displaySpellCast(SpellID spellID, BattleHex destinationTile, bool areaEffect = true); //displays spell`s cast animation
	void displaySpellEffect(SpellID spellID, BattleHex destinationTile, bool areaEffect = true); //displays spell`s affected animation
	void displaySpellHit(SpellID spellID, BattleHex destinationTile, bool areaEffect = true); //displays spell`s affected animation

	void displaySpellAnimation(const CSpell::TAnimation & animation, BattleHex destinationTile, bool areaEffect = true);

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

	void gateStateChanged(const EGateState state);

	const CGHeroInstance * currentHero() const;
	InfoAboutHero enemyHero() const;

	friend class CPlayerInterface;
	friend class CButton;
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
