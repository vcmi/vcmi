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
struct CustomEffectInfo;

VCMI_LIB_NAMESPACE_END

class BattleHero;
class Canvas;
class BattleResultWindow;
class StackQueue;
class CPlayerInterface;
class ClickableHex;
class CAnimation;
struct BattleEffect;
class IImage;
class StackQueue;

class BattleProjectileController;
class BattleSiegeController;
class BattleObstacleController;
class BattleFieldController;
class BattleRenderer;
class BattleControlPanel;
class BattleStacksController;
class BattleActionsController;
class BattleEffectsController;

/// Small struct which contains information about the id of the attacked stack, the damage dealt,...
struct StackAttackedInfo
{
	const CStack *defender;
	const CStack *attacker;

	int64_t  damageDealt;
	uint32_t amountKilled;

	EBattleEffect::EBattleEffect battleEffect;
	SpellID spellEffect;

	bool indirectAttack; //if true, stack was attacked indirectly - spell or ranged attack
	bool killed; //if true, stack has been killed
	bool rebirth; //if true, play rebirth animation after all
	bool cloneKilled;
};

struct StackAttackInfo
{
	const CStack *attacker;
	const CStack *defender;
	std::vector< const CStack *> secondaryDefender;

	//EBattleEffect::EBattleEffect battleEffect;
	SpellID spellEffect;
	BattleHex tile;

	bool indirectAttack;
	bool lucky;
	bool unlucky;
	bool deathBlow;
	bool lifeDrain;
};


/// Big class which handles the overall battle interface actions and it is also responsible for
/// drawing everything correctly.
class BattleInterface : public WindowBase
{
private:
	std::shared_ptr<BattleHero> attackingHero;
	std::shared_ptr<BattleHero> defendingHero;
	std::shared_ptr<StackQueue> queue;
	std::shared_ptr<BattleControlPanel> controlPanel;

	std::shared_ptr<CPlayerInterface> tacticianInterface; //used during tactics mode, points to the interface of player with higher tactics (can be either attacker or defender in hot-seat), valid onloy for human players
	std::shared_ptr<CPlayerInterface> attackerInt, defenderInt; //because LOCPLINT is not enough in hotSeat
	std::shared_ptr<CPlayerInterface> curInt; //current player interface

	const CCreatureSet *army1, *army2; //copy of initial armies (for result window)
	const CGHeroInstance *attackingHeroInstance, *defendingHeroInstance;

	ui8 animCount;

	bool tacticsMode;
	bool battleActionsStarted; //used for delaying battle actions until intro sound stops
	int battleIntroSoundChannel; //required as variable for disabling it via ESC key

	using AwaitingAnimationAction = std::function<void()>;

	struct AwaitingAnimationEvents {
		AwaitingAnimationAction action;
		EAnimationEvents event;
		bool eventState;
	};

	/// Conditional variables that are set depending on ongoing animations on the battlefield
	std::array< CondSh<bool>, static_cast<size_t>(EAnimationEvents::COUNT)> animationEvents;

	/// List of events that are waiting to be triggered
	std::vector<AwaitingAnimationEvents> awaitingEvents;

	void trySetActivePlayer( PlayerColor player ); // if in hotseat, will activate interface of chosen player
	void activateStack(); //sets activeStack to stackToActivate etc. //FIXME: No, it's not clear at all
	void requestAutofightingAIToTakeAction();

	void giveCommand(EActionType action, BattleHex tile = BattleHex(), si32 additional = -1);
	void sendCommand(BattleAction *& command, const CStack * actor = nullptr);

	const CGHeroInstance *getActiveHero(); //returns hero that can currently cast a spell

	void showInterface(SDL_Surface * to);

	void setHeroAnimation(ui8 side, int phase);

	void executeSpellCast(); //called when a hero casts a spell

public:
	std::unique_ptr<BattleProjectileController> projectilesController;
	std::unique_ptr<BattleSiegeController> siegeController;
	std::unique_ptr<BattleObstacleController> obstacleController;
	std::unique_ptr<BattleFieldController> fieldController;
	std::unique_ptr<BattleStacksController> stacksController;
	std::unique_ptr<BattleActionsController> actionsController;
	std::unique_ptr<BattleEffectsController> effectsController;

	static CondSh<BattleAction *> givenCommand; //data != nullptr if we have i.e. moved current unit

	bool myTurn; //if true, interface is active (commands can be ordered)
	int moveSoundHander; // sound handler used when moving a unit

	const BattleResult *bresult; //result of a battle; if non-zero then display when all animations end

	BattleInterface(const CCreatureSet *army1, const CCreatureSet *army2, const CGHeroInstance *hero1, const CGHeroInstance *hero2, const SDL_Rect & myRect, std::shared_ptr<CPlayerInterface> att, std::shared_ptr<CPlayerInterface> defen, std::shared_ptr<CPlayerInterface> spectatorInt = nullptr);
	virtual ~BattleInterface();

	void setPrintCellBorders(bool set); //if true, cell borders will be printed
	void setPrintStackRange(bool set); //if true,range of active stack will be printed
	void setPrintMouseShadow(bool set); //if true, hex under mouse will be shaded
	void setAnimSpeed(int set); //speed of animation; range 1..100
	int getAnimSpeed() const; //speed of animation; range 1..100
	CPlayerInterface *getCurrentPlayerInterface() const;

	void tacticNextStack(const CStack *current);
	void tacticPhaseEnd();

	/// sets condition to targeted state and executes any awaiting actions
	void setAnimationCondition( EAnimationEvents event, bool state);

	/// returns current state of condition
	bool getAnimationCondition( EAnimationEvents event);

	/// locks execution until selected condition reached targeted state
	void waitForAnimationCondition( EAnimationEvents event, bool state);

	/// adds action that will be executed one selected condition reached targeted state
	void executeOnAnimationCondition( EAnimationEvents event, bool state, const AwaitingAnimationAction & action);

	//napisz tu klase odpowiadajaca za wyswietlanie bitwy i obsluge uzytkownika, polecenia ma przekazywac callbackiem
	void activate() override;
	void deactivate() override;
	void keyPressed(const SDL_KeyboardEvent & key) override;
	void mouseMoved(const SDL_MouseMotionEvent &sEvent) override;
	void clickRight(tribool down, bool previousState) override;

	void show(SDL_Surface *to) override;
	void showAll(SDL_Surface *to) override;

	void collectRenderableObjects(BattleRenderer & renderer);

	//call-ins
	void startAction(const BattleAction* action);
	void stackReset(const CStack * stack);
	void stackAdded(const CStack * stack); //new stack appeared on battlefield
	void stackRemoved(uint32_t stackID); //stack disappeared from batlefiled
	void stackActivated(const CStack *stack); //active stack has been changed
	void stackMoved(const CStack *stack, std::vector<BattleHex> destHex, int distance); //stack with id number moved to destHex
	void stacksAreAttacked(std::vector<StackAttackedInfo> attackedInfos); //called when a certain amount of stacks has been attacked
	void stackAttacking(const StackAttackInfo & attackInfo); //called when stack with id ID is attacking something on hex dest
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

	void displaySpellAnimationQueue(const CSpell::TAnimationQueue & q, BattleHex destinationTile, bool isHit);
	void displaySpellCast(SpellID spellID, BattleHex destinationTile); //displays spell`s cast animation
	void displaySpellEffect(SpellID spellID, BattleHex destinationTile); //displays spell`s affected animation
	void displaySpellHit(SpellID spellID, BattleHex destinationTile); //displays spell`s affected animation

	void endAction(const BattleAction* action);
	void hideQueue();
	void showQueue();

	void obstaclePlaced(const std::vector<std::shared_ptr<const CObstacleInstance>> oi);

	void gateStateChanged(const EGateState state);

	const CGHeroInstance *currentHero() const;
	InfoAboutHero enemyHero() const;

	// TODO: cleanup this list
	friend class CPlayerInterface;
	friend class CInGameConsole;
	friend class StackQueue;
	friend class BattleResultWindow;
	friend class BattleHero;
	friend class CBattleStackAnimation;
	friend class CReverseAnimation;
	friend class CDefenceAnimation;
	friend class CMovementAnimation;
	friend class CMovementStartAnimation;
	friend class CAttackAnimation;
	friend class CMeleeAttackAnimation;
	friend class CShootingAnimation;
	friend class CCastAnimation;
	friend class ClickableHex;
	friend class BattleProjectileController;
	friend class BattleSiegeController;
	friend class BattleObstacleController;
	friend class BattleFieldController;
	friend class BattleControlPanel;
	friend class BattleStacksController;
	friend class BattleActionsController;
	friend class BattleEffectsController;
};
