/*
 * CBattleGameInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IBattleEventsReceiver.h"

#include "../battle/AutocombatPreferences.h"

static constexpr int AI_INTERFACE_VER = 1;

VCMI_LIB_NAMESPACE_BEGIN

class CBattleCallback;
class Environment;

class DLL_LINKAGE CBattleGameInterface : public IBattleEventsReceiver
{
public:
	bool human = false;
	PlayerColor playerID;
	std::string dllName;

	virtual ~CBattleGameInterface() {};
	virtual void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB){};
	virtual void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB, AutocombatPreferences autocombatPreferences){};

	//battle call-ins
	virtual void activeStack(const BattleID & battleID, const CStack * stack)=0; //called when it's turn of that stack
	virtual void yourTacticPhase(const BattleID & battleID, int distance)=0; //called when interface has opportunity to use Tactics skill -> use cb->battleMakeTacticAction from this function
};

VCMI_LIB_NAMESPACE_END
