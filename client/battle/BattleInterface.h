/*
 * BattleInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleConstants.h"
#include "../gui/CIntObject.h"
#include "../../lib/spells/CSpellHandler.h" //CSpell::TAnimation
#include "../../lib/CondSh.h"

VCMI_LIB_NAMESPACE_BEGIN

class CCreatureSet;
class CGHeroInstance;
class CStack;
struct BattleResult;
struct BattleSpellCast;
struct CObstacleInstance;
struct SetStackEffect;
class BattleAction;
class CGTownInstance;
struct CatapultAttack;
struct BattleTriggerEffect;
struct BattleHex;
struct InfoAboutHero;
class ObstacleChanges;

VCMI_LIB_NAMESPACE_END

class BattleHero;
class Canvas;
class BattleResultWindow;
class StackQueue;
class CPlayerInterface;
class CAnimation;
struct BattleEffect;
class IImage;
class StackQueue;

class BattleProjectileController;
class BattleSiegeController;
class BattleObstacleController;
class BattleFieldController;
class BattleRenderer;
class BattleWindow;
class BattleStacksController;
class BattleActionsController;
class BattleEffectsController;
class BattleConsole;

/// Small struct which contains information about the id of the attacked stack, the damage dealt,...
struct StackAttackedInfo
{
	const CStack *defender;
	const CStack *attacker;

	int64_t  damageDealt;
	uint32_t amountKilled;
	SpellID spellEffect;

	bool indirectAttack; //if true, stack was attacked indirectly - spell or ranged attack
	bool killed; //if true, stack has been killed
	bool rebirth; //if true, play rebirth animation after all
	bool cloneKilled;
	bool fireShield;
};

struct StackAttackInfo
{
	const CStack *attacker;
	const CStack *defender;
	std::vector< const CStack *> secondaryDefender;

	SpellID spellEffect;
	BattleHex tile;

	bool indirectAttack;
	bool lucky;
	bool unlucky;
	bool deathBlow;
	bool lifeDrain;
};

/// Main class for battles, responsible for relaying information from server to various battle entities
class BattleInterface
{
	using AwaitingAnimationAction = std::function<void()>;

	struct AwaitingAnimationEvents {
		AwaitingAnimationAction action;
		EAnimationEvents event;
	};

	/// Conditional variables that are set depending on ongoing animations on the battlefield
	CondSh<bool> ongoingAnimationsState;

	/// List of events that are waiting to be triggered
	std::vector<AwaitingAnimationEvents> awaitingEvents;

	/// used during tactics mode, points to the interface of player with higher tactics (can be either attacker or defender in hot-seat), valid onloy for human players
	std::shared_ptr<CPlayerInterface> tacticianInterface;

	/// attacker interface, not null if attacker is human in our vcmiclient
	std::shared_ptr<CPlayerInterface> attackerInt;

	/// defender interface, not null if attacker is human in our vcmiclient
	std::shared_ptr<CPlayerInterface> defenderInt;

	/// if set to true, battle is still starting and waiting for intro sound to end / key press from player
	bool battleOpeningDelayActive;

	void playIntroSoundAndUnlockInterface();
	void onIntroSoundPlayed();
public:
	/// copy of initial armies (for result window)
	const CCreatureSet *army1;
	const CCreatureSet *army2;

	std::shared_ptr<BattleWindow> windowObject;
	std::shared_ptr<BattleConsole> console;

	/// currently active player interface
	std::shared_ptr<CPlayerInterface> curInt;

	const CGHeroInstance *attackingHeroInstance;
	const CGHeroInstance *defendingHeroInstance;

	bool tacticsMode;

	std::unique_ptr<BattleProjectileController> projectilesController;
	std::unique_ptr<BattleSiegeController> siegeController;
	std::unique_ptr<BattleObstacleController> obstacleController;
	std::unique_ptr<BattleFieldController> fieldController;
	std::unique_ptr<BattleStacksController> stacksController;
	std::unique_ptr<BattleActionsController> actionsController;
	std::unique_ptr<BattleEffectsController> effectsController;

	std::shared_ptr<BattleHero> attackingHero;
	std::shared_ptr<BattleHero> defendingHero;

	static CondSh<BattleAction *> givenCommand; //data != nullptr if we have i.e. moved current unit

	bool openingPlaying();
	void openingEnd();

	bool makingTurn() const;

	BattleInterface(const CCreatureSet *army1, const CCreatureSet *army2, const CGHeroInstance *hero1, const CGHeroInstance *hero2, std::shared_ptr<CPlayerInterface> att, std::shared_ptr<CPlayerInterface> defen, std::shared_ptr<CPlayerInterface> spectatorInt = nullptr);
	~BattleInterface();

	void trySetActivePlayer( PlayerColor player ); // if in hotseat, will activate interface of chosen player
	void activateStack(); //sets activeStack to stackToActivate etc. //FIXME: No, it's not clear at all
	void requestAutofightingAIToTakeAction();

	void giveCommand(EActionType action, BattleHex tile = BattleHex(), si32 additional = -1);
	void sendCommand(BattleAction *& command, const CStack * actor = nullptr);

	const CGHeroInstance *getActiveHero(); //returns hero that can currently cast a spell

	void showInterface(Canvas & to);

	void setHeroAnimation(ui8 side, EHeroAnimType phase);

	void executeSpellCast(); //called when a hero casts a spell

	void appendBattleLog(const std::string & newEntry);

	void redrawBattlefield(); //refresh GUI after changing stack range / grid settings
	CPlayerInterface *getCurrentPlayerInterface() const;

	void tacticNextStack(const CStack *current);
	void tacticPhaseEnd();

	void setBattleQueueVisibility(bool visible);

	void executeStagedAnimations();
	void executeAnimationStage( EAnimationEvents event);
	void onAnimationsStarted();
	void onAnimationsFinished();
	void waitForAnimations();
	bool hasAnimations();
	void checkForAnimations();
	void addToAnimationStage( EAnimationEvents event, const AwaitingAnimationAction & action);

	//call-ins
	void startAction(const BattleAction* action);
	void stackReset(const CStack * stack);
	void stackAdded(const CStack * stack); //new stack appeared on battlefield
	void stackRemoved(uint32_t stackID); //stack disappeared from batlefiled
	void stackActivated(const CStack *stack); //active stack has been changed
	void stackMoved(const CStack *stack, std::vector<BattleHex> destHex, int distance, bool teleport); //stack with id number moved to destHex
	void stacksAreAttacked(std::vector<StackAttackedInfo> attackedInfos); //called when a certain amount of stacks has been attacked
	void stackAttacking(const StackAttackInfo & attackInfo); //called when stack with id ID is attacking something on hex dest
	void newRoundFirst( int round );
	void newRound(int number); //caled when round is ended; number is the number of round
	void stackIsCatapulting(const CatapultAttack & ca); //called when a stack is attacking walls
	void battleFinished(const BattleResult& br, QueryID queryID); //called when battle is finished - battleresult window should be printed
	void spellCast(const BattleSpellCast *sc); //called when a hero casts a spell
	void battleStacksEffectsSet(const SetStackEffect & sse); //called when a specific effect is set to stacks
	void castThisSpell(SpellID spellID); //called when player has chosen a spell from spellbook

	void displayBattleLog(const std::vector<MetaString> & battleLog);

	void displaySpellAnimationQueue(const CSpell * spell, const CSpell::TAnimationQueue & q, BattleHex destinationTile, bool isHit);
	void displaySpellCast(const CSpell * spell, BattleHex destinationTile); //displays spell`s cast animation
	void displaySpellEffect(const CSpell * spell, BattleHex destinationTile); //displays spell`s affected animation
	void displaySpellHit(const CSpell * spell, BattleHex destinationTile); //displays spell`s affected animation

	void endAction(const BattleAction* action);

	void obstaclePlaced(const std::vector<std::shared_ptr<const CObstacleInstance>> oi);
	void obstacleRemoved(const std::vector<ObstacleChanges> & obstacles);

	void gateStateChanged(const EGateState state);

	const CGHeroInstance *currentHero() const;
	InfoAboutHero enemyHero() const;
};
