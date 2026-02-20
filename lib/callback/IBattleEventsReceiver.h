/*
 * IBattleEventsReceiver.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/EntityIdentifiers.h"
#include "../int3.h"

VCMI_LIB_NAMESPACE_BEGIN

class BattleAction;
struct BattleAttack;
struct BattleStackAttacked;
struct BattleResult;
struct BattleSpellCast;
struct SetStackEffect;
struct BattleTriggerEffect;
struct CatapultAttack;

class ObstacleChanges;
class UnitChanges;
class BattleHexArray;
class CCreatureSet;
class CGHeroInstance;
class MetaString;
class CStack;

enum class BattleSide : int8_t;
enum class EGateState : int8_t;

class DLL_LINKAGE IBattleEventsReceiver
{
public:
	virtual void actionFinished(const BattleID & battleID, const BattleAction &action){};//occurs AFTER every action taken by any stack or by the hero
	virtual void actionStarted(const BattleID & battleID, const BattleAction &action){};//occurs BEFORE every action taken by any stack or by the hero
	virtual void battleAttack(const BattleID & battleID, const BattleAttack *ba){}; //called when stack is performing attack
	virtual void battleStacksAttacked(const BattleID & battleID, const std::vector<BattleStackAttacked> & bsa, bool ranged){}; //called when stack receives damage (after battleAttack())
	virtual void battleEnd(const BattleID & battleID, const BattleResult *br, QueryID queryID){};
	virtual void battleNewRoundFirst(const BattleID & battleID){}; //called at the beginning of each turn before changes are applied;
	virtual void battleNewRound(const BattleID & battleID){}; //called at the beginning of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	virtual void battleLogMessage(const BattleID & battleID, const std::vector<MetaString> & lines){};
	virtual void battleStackMoved(const BattleID & battleID, const CStack * stack, const BattleHexArray & dest, int distance, bool teleport){};
	virtual void battleSpellCast(const BattleID & battleID, const BattleSpellCast *sc){};
	virtual void battleStacksEffectsSet(const BattleID & battleID, const SetStackEffect & sse){};//called when a specific effect is set to stacks
	virtual void battleTriggerEffect(const BattleID & battleID, const BattleTriggerEffect & bte){}; //called for various one-shot effects
	virtual void battleStartBefore(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2) {}; //called just before battle start
	virtual void battleStart(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide side, bool replayAllowed){}; //called by engine when battle starts; side=0 - left, side=1 - right
	virtual void battleUnitsChanged(const BattleID & battleID, const std::vector<UnitChanges> & units){};
	virtual void battleObstaclesChanged(const BattleID & battleID, const std::vector<ObstacleChanges> & obstacles){};
	virtual void battleCatapultAttacked(const BattleID & battleID, const CatapultAttack & ca){}; //called when catapult makes an attack
	virtual void battleGateStateChanged(const BattleID & battleID, const EGateState state){};
};

VCMI_LIB_NAMESPACE_END
