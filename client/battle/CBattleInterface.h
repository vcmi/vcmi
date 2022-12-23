/*
 * CBattleInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/spells/Magic.h>

#include "../../lib/ConstTransitivePtr.h" //may be redundant
#include "../../lib/GameConstants.h"

#include "CBattleAnimations.h"

#include "../../lib/spells/CSpellHandler.h" //CSpell::TAnimation
#include "../../lib/CCreatureHandler.h"
#include "../../lib/battle/CBattleInfoCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

class CCreatureSet;
class CGHeroInstance;
class CStack;
struct BattleResult;
struct BattleSpellCast;
struct CObstacleInstance;
template <typename T> struct CondSh;
struct SetStackEffect;
class BattleAction;
class CGTownInstance;
struct CatapultAttack;
struct BattleTriggerEffect;
struct BattleHex;
struct InfoAboutHero;
class CBattleGameInterface;
struct CustomEffectInfo;
class CSpell;

VCMI_LIB_NAMESPACE_END

class CLabel;
class CCallback;
class CButton;
class CToggleButton;
class CToggleGroup;
struct CatapultProjectileInfo;
class CBattleAnimation;
class CBattleHero;
class CBattleConsole;
class CBattleResultWindow;
class CStackQueue;
class CPlayerInterface;
class CCreatureAnimation;
struct ProjectileInfo;
class CClickableHex;
class CAnimation;
class IImage;
class CStackQueue;

/// Small struct which contains information about the id of the attacked stack, the damage dealt,...
struct StackAttackedInfo
{
	const CStack *defender; //attacked stack
	int64_t dmg; //damage dealt
	unsigned int amountKilled; //how many creatures in stack has been killed
	const CStack *attacker; //attacking stack
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
	std::shared_ptr<CAnimation> animation;
	int effectID; //uniqueID equal ot ID of appropriate CSpellEffectAnim
	BattleHex position; //Indicates if effect which hex the effect is drawn on
};

struct BattleObjectsByHex
{
	typedef std::vector<int> TWallList;
	typedef std::vector<const CStack *> TStackList;
	typedef std::vector<const BattleEffect *> TEffectList;
	typedef std::vector<std::shared_ptr<const CObstacleInstance>> TObstacleList;

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

enum class MouseHoveredHexContext
{
	UNOCCUPIED_HEX,
	OCCUPIED_HEX
};

/// Big class which handles the overall battle interface actions and it is also responsible for
/// drawing everything correctly.
class CBattleInterface : public WindowBase
{
private:
	SDL_Surface *background, *menu, *amountNormal, *amountNegative, *amountPositive, *amountEffNeutral, *cellBorders, *backgroundWithHexes;

	std::shared_ptr<CButton> bOptions;
	std::shared_ptr<CButton> bSurrender;
	std::shared_ptr<CButton> bFlee;
	std::shared_ptr<CButton> bAutofight;
	std::shared_ptr<CButton> bSpell;
	std::shared_ptr<CButton> bWait;
	std::shared_ptr<CButton> bDefence;
	std::shared_ptr<CButton> bConsoleUp;
	std::shared_ptr<CButton> bConsoleDown;
	std::shared_ptr<CButton> btactNext;
	std::shared_ptr<CButton> btactEnd;

	std::shared_ptr<CBattleConsole> console;
	std::shared_ptr<CBattleHero> attackingHero;
	std::shared_ptr<CBattleHero> defendingHero;
	std::shared_ptr<CStackQueue> queue;

	const CCreatureSet *army1, *army2; //copy of initial armies (for result window)
	const CGHeroInstance *attackingHeroInstance, *defendingHeroInstance;
	std::map<int32_t, std::shared_ptr<CCreatureAnimation>> creAnims; //animations of creatures from fighting armies (order by BattleInfo's stacks' ID)

	std::map<int, std::shared_ptr<CAnimation>> idToProjectile;
	std::map<int, std::vector<CCreature::CreatureAnimation::RayColor>> idToRay;

	std::map<std::string, std::shared_ptr<CAnimation>> animationsCache;
	std::map<si32, std::shared_ptr<CAnimation>> obstacleAnimations;

	std::map<int, bool> creDir; // <creatureID, if false reverse creature's animation> //TODO: move it to battle callback
	ui8 animCount;
	const CStack *activeStack; //number of active stack; nullptr - no one
	const CStack *mouseHoveredStack; // stack below mouse pointer, used for border animation
	const CStack *stackToActivate; //when animation is playing, we should wait till the end to make the next stack active; nullptr of none
	const CStack *selectedStack; //for Teleport / Sacrifice
	void activateStack(); //sets activeStack to stackToActivate etc. //FIXME: No, it's not clear at all
	std::vector<BattleHex> occupyableHexes, //hexes available for active stack
		attackableHexes; //hexes attackable by active stack
	std::array<bool, GameConstants::BFIELD_SIZE> stackCountOutsideHexes; // hexes that when in front of a unit cause it's amount box to move back
	BattleHex previouslyHoveredHex; //number of hex that was hovered by the cursor a while ago
	BattleHex currentlyHoveredHex; //number of hex that is supposed to be hovered (for a while it may be inappropriately set, but will be renewed soon)
	int attackingHex; //hex from which the stack would perform attack with current cursor

	std::shared_ptr<CPlayerInterface> tacticianInterface; //used during tactics mode, points to the interface of player with higher tactics (can be either attacker or defender in hot-seat), valid onloy for human players
	bool tacticsMode;
	bool stackCanCastSpell; //if true, active stack could possibly cast some target spell
	bool creatureCasting; //if true, stack currently aims to cats a spell
	bool spellDestSelectMode; //if true, player is choosing destination for his spell - only for GUI / console
	std::shared_ptr<BattleAction> spellToCast; //spell for which player is choosing destination
	const CSpell *sp; //spell pointer for convenience
	si32 creatureSpellToCast;
	std::vector<PossiblePlayerBattleAction> possibleActions; //all actions possible to call at the moment by player
	std::vector<PossiblePlayerBattleAction> localActions; //actions possible to take on hovered hex
	std::vector<PossiblePlayerBattleAction> illegalActions; //these actions display message in case of illegal target
	PossiblePlayerBattleAction currentAction; //action that will be performed on l-click
	PossiblePlayerBattleAction selectedAction; //last action chosen (and saved) by player
	PossiblePlayerBattleAction illegalAction; //most likely action that can't be performed here
	bool battleActionsStarted; //used for delaying battle actions until intro sound stops
	int battleIntroSoundChannel; //required as variable for disabling it via ESC key

	void setActiveStack(const CStack *stack);
	void setHoveredStack(const CStack *stack);

	void requestAutofightingAIToTakeAction();

	std::vector<PossiblePlayerBattleAction> getPossibleActionsForStack (const CStack *stack); //called when stack gets its turn
	void endCastingSpell(); //ends casting spell (eg. when spell has been cast or canceled)
	void reorderPossibleActionsPriority(const CStack * stack, MouseHoveredHexContext context);

	//force active stack to cast a spell if possible
	void enterCreatureCastingMode();

	std::list<ProjectileInfo> projectiles; //projectiles flying on battlefield
	void giveCommand(EActionType action, BattleHex tile = BattleHex(), si32 additional = -1);
	void sendCommand(BattleAction *& command, const CStack * actor = nullptr);

	bool isTileAttackable(const BattleHex & number) const; //returns true if tile 'number' is neighboring any tile from active stack's range or is one of these tiles
	bool isCatapultAttackable(BattleHex hex) const; //returns true if given tile can be attacked by catapult

	std::list<BattleEffect> battleEffects; //different animations to display on the screen like spell effects

	/// Class which is responsible for drawing the wall of a siege during battle
	class SiegeHelper
	{
	private:
		SDL_Surface* walls[18];
		const CBattleInterface *owner;
	public:
		const CGTownInstance *town; //besieged town

		SiegeHelper(const CGTownInstance *siegeTown, const CBattleInterface *_owner);
		~SiegeHelper();

		std::string getSiegeName(ui16 what) const;
		std::string getSiegeName(ui16 what, int state) const; // state uses EWallState enum

		void printPartOfWall(SDL_Surface *to, int what);

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
	} *siegeH;

	std::shared_ptr<CPlayerInterface> attackerInt, defenderInt; //because LOCPLINT is not enough in hotSeat
	std::shared_ptr<CPlayerInterface> curInt; //current player interface
	const CGHeroInstance *getActiveHero(); //returns hero that can currently cast a spell

	/** Methods for displaying battle screen */
	void showBackground(SDL_Surface *to);

	void showBackgroundImage(SDL_Surface *to);
	void showAbsoluteObstacles(SDL_Surface *to);
	void showHighlightedHexes(SDL_Surface *to);
	void showHighlightedHex(SDL_Surface *to, BattleHex hex, bool darkBorder = false);
	void showInterface(SDL_Surface *to);

	void showBattlefieldObjects(SDL_Surface *to);

	void showAliveStacks(SDL_Surface *to, std::vector<const CStack *> stacks);
	void showStacks(SDL_Surface *to, std::vector<const CStack *> stacks);
	void showObstacles(SDL_Surface *to, std::vector<std::shared_ptr<const CObstacleInstance>> &obstacles);
	void showPiecesOfWall(SDL_Surface *to, std::vector<int> pieces);

	void showBattleEffects(SDL_Surface *to, const std::vector<const BattleEffect *> &battleEffects);
	void showProjectiles(SDL_Surface *to);

	BattleObjectsByHex sortObjectsByHex();
	void updateBattleAnimations();

	std::shared_ptr<IImage> getObstacleImage(const CObstacleInstance & oi);

	Point getObstaclePosition(std::shared_ptr<IImage> image, const CObstacleInstance & obstacle);

	void redrawBackgroundWithHexes(const CStack *activeStack);
	/** End of battle screen blitting methods */

	void setHeroAnimation(ui8 side, int phase);
public:
	static CondSh<bool> animsAreDisplayed; //for waiting with the end of battle for end of anims
	static CondSh<BattleAction *> givenCommand; //data != nullptr if we have i.e. moved current unit

	std::list<std::pair<CBattleAnimation *, bool>> pendingAnims; //currently displayed animations <anim, initialized>
	void addNewAnim(CBattleAnimation *anim); //adds new anim to pendingAnims
	ui32 animIDhelper; //for giving IDs for animations


	CBattleInterface(const CCreatureSet *army1, const CCreatureSet *army2, const CGHeroInstance *hero1, const CGHeroInstance *hero2, const SDL_Rect & myRect, std::shared_ptr<CPlayerInterface> att, std::shared_ptr<CPlayerInterface> defen, std::shared_ptr<CPlayerInterface> spectatorInt = nullptr);
	virtual ~CBattleInterface();

	//std::vector<TimeInterested*> timeinterested; //animation handling
	void setPrintCellBorders(bool set); //if true, cell borders will be printed
	void setPrintStackRange(bool set); //if true,range of active stack will be printed
	void setPrintMouseShadow(bool set); //if true, hex under mouse will be shaded
	void setAnimSpeed(int set); //speed of animation; range 1..100
	int getAnimSpeed() const; //speed of animation; range 1..100
	CPlayerInterface *getCurrentPlayerInterface() const;
	bool shouldRotate(const CStack * stack, const BattleHex & oldPos, const BattleHex & nextHex);

	std::vector<std::shared_ptr<CClickableHex>> bfield; //11 lines, 17 hexes on each
	SDL_Surface *cellBorder, *cellShade;

	bool myTurn; //if true, interface is active (commands can be ordered)

	bool moveStarted; //if true, the creature that is already moving is going to make its first step
	int moveSoundHander; // sound handler used when moving a unit

	const BattleResult *bresult; //result of a battle; if non-zero then display when all animations end

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
	void unitAdded(const CStack * stack); //new stack appeared on battlefield
	void stackRemoved(uint32_t stackID); //stack disappeared from batlefiled
	void stackActivated(const CStack *stack); //active stack has been changed
	void stackMoved(const CStack *stack, std::vector<BattleHex> destHex, int distance); //stack with id number moved to destHex
	void waitForAnims();
	void stacksAreAttacked(std::vector<StackAttackedInfo> attackedInfos); //called when a certain amount of stacks has been attacked
	void stackAttacking(const CStack *attacker, BattleHex dest, const CStack *attacked, bool shooting); //called when stack with id ID is attacking something on hex dest
	void newRoundFirst( int round );
	void newRound(int number); //caled when round is ended; number is the number of round
	void hexLclicked(int whichOne); //hex only call-in
	void stackIsCatapulting(const CatapultAttack & ca); //called when a stack is attacking walls
	void battleFinished(const BattleResult& br); //called when battle is finished - battleresult window should be printed
	void displayBattleFinished(); //displays battle result
	void spellCast(const BattleSpellCast *sc); //called when a hero casts a spell
	void battleStacksEffectsSet(const SetStackEffect & sse); //called when a specific effect is set to stacks
	void castThisSpell(SpellID spellID); //called when player has chosen a spell from spellbook

	void displayBattleLog(const std::vector<MetaString> & battleLog);
	void displayCustomEffects(const std::vector<CustomEffectInfo> & customEffects);

	void displayEffect(ui32 effect, BattleHex destTile); //displays custom effect on the battlefield

	void displaySpellAnimationQueue(const CSpell::TAnimationQueue & q, BattleHex destinationTile);
	void displaySpellCast(SpellID spellID, BattleHex destinationTile); //displays spell`s cast animation
	void displaySpellEffect(SpellID spellID, BattleHex destinationTile); //displays spell`s affected animation
	void displaySpellHit(SpellID spellID, BattleHex destinationTile); //displays spell`s affected animation

	void battleTriggerEffect(const BattleTriggerEffect & bte);
	void setBattleCursor(const int myNumber); //really complex and messy, sets attackingHex
	void endAction(const BattleAction* action);
	void hideQueue();
	void showQueue();

	Rect hexPosition(BattleHex hex) const;

	void handleHex(BattleHex myNumber, int eventType);
	bool isCastingPossibleHere (const CStack *sactive, const CStack *shere, BattleHex myNumber);
	bool canStackMoveHere (const CStack *sactive, BattleHex MyNumber); //TODO: move to BattleState / callback

	BattleHex fromWhichHexAttack(BattleHex myNumber);
	void obstaclePlaced(const CObstacleInstance & oi);

	void gateStateChanged(const EGateState state);

	void initStackProjectile(const CStack * stack);

	const CGHeroInstance *currentHero() const;
	InfoAboutHero enemyHero() const;

	friend class CPlayerInterface;
	friend class CButton;
	friend class CInGameConsole;
	friend class CStackQueue;
	friend class CBattleResultWindow;
	friend class CBattleHero;
	friend class CEffectAnimation;
	friend class CBattleStackAnimation;
	friend class CReverseAnimation;
	friend class CDefenceAnimation;
	friend class CMovementAnimation;
	friend class CMovementStartAnimation;
	friend class CAttackAnimation;
	friend class CMeleeAttackAnimation;
	friend class CShootingAnimation;
	friend class CCastAnimation;
	friend class CClickableHex;
};
