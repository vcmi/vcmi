/*
 * CAdventureAI.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CGlobalAI.h"

VCMI_LIB_NAMESPACE_BEGIN

//class to  be inherited by adventure-only AIs, it cedes battle actions to given battle-AI
class DLL_LINKAGE CAdventureAI : public CGlobalAI
{
public:
	CAdventureAI() = default;

	std::shared_ptr<CBattleGameInterface> battleAI;
	std::shared_ptr<CBattleCallback> cbc;

	virtual std::string getBattleAIName() const = 0; //has to return name of the battle AI to be used

	//battle interface
	void activeStack(const BattleID & battleID, const CStack * stack) override;
	void yourTacticPhase(const BattleID & battleID, int distance) override;

	void battleNewRound(const BattleID & battleID) override;
	void battleCatapultAttacked(const BattleID & battleID, const CatapultAttack & ca) override;
	void battleStart(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide side, bool replayAllowed) override;
	void battleStacksAttacked(const BattleID & battleID, const std::vector<BattleStackAttacked> & bsa, bool ranged) override;
	void actionStarted(const BattleID & battleID, const BattleAction &action) override;
	void battleNewRoundFirst(const BattleID & battleID) override;
	void actionFinished(const BattleID & battleID, const BattleAction &action) override;
	void battleStacksEffectsSet(const BattleID & battleID, const SetStackEffect & sse) override;
	void battleObstaclesChanged(const BattleID & battleID, const std::vector<ObstacleChanges> & obstacles) override;
	void battleStackMoved(const BattleID & battleID, const CStack * stack, const BattleHexArray & dest, int distance, bool teleport) override;
	void battleAttack(const BattleID & battleID, const BattleAttack *ba) override;
	void battleSpellCast(const BattleID & battleID, const BattleSpellCast *sc) override;
	void battleEnd(const BattleID & battleID, const BattleResult *br, QueryID queryID) override;
	void battleUnitsChanged(const BattleID & battleID, const std::vector<UnitChanges> & units) override;
};

VCMI_LIB_NAMESPACE_END
