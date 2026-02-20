/*
 * CGameInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CBattleGameInterface.h"
#include "IGameEventsReceiver.h"
#include "../constants/Enumerations.h"

VCMI_LIB_NAMESPACE_BEGIN

class BattleStateInfoForRetreat;
struct ObjectPosInfo;

class CCallback;
class CCommanderInstance;

using TTeleportExitsList = std::vector<std::pair<ObjectInstanceID, int3>>;

/// Central class for managing human player / AI interface logic
class DLL_LINKAGE CGameInterface : public CBattleGameInterface, public IGameEventsReceiver
{
public:
	virtual ~CGameInterface() = default;
	virtual void initGameInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CCallback> CB){};
	virtual void yourTurn(QueryID askID){}; //called AFTER playerStartsTurn(player)

	//pskill is gained primary skill, interface has to choose one of given skills and call callback with selection id
	virtual void heroGotLevel(const CGHeroInstance *hero, PrimarySkill pskill, std::vector<SecondarySkill> &skills, QueryID queryID)=0;
	virtual void commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID)=0;

	// Show a dialog, player must take decision. If selection then he has to choose between one of given components,
	// if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called
	// with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	virtual void showBlockingDialog(const std::string &text, const std::vector<Component> &components, QueryID askID, const int soundID, bool selection, bool cancel, bool safeToAutoaccept) = 0;

	// all stacks operations between these objects become allowed, interface has to call onEnd when done
	virtual void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, QueryID queryID) = 0;
	virtual void showTeleportDialog(const CGHeroInstance * hero, TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID) = 0;
	virtual void showMapObjectSelectDialog(QueryID askID, const Component & icon, const MetaString & title, const MetaString & description, const std::vector<ObjectInstanceID> & objects) = 0;
	virtual void finish(){}; //if for some reason we want to end

	virtual void showWorldViewEx(const std::vector<ObjectPosInfo> & objectPositions, bool showTerrain){};

	virtual std::optional<BattleAction> makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState) = 0;

	/// Invalidates and destroys all paths for all heroes
	virtual void invalidatePaths(){};

	virtual void setColorScheme(ColorScheme scheme){};
};

VCMI_LIB_NAMESPACE_END
