/*
 * MapQueries.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CQuery.h"
#include "../../lib/networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN
class CGHeroInstance;
class CGObjectInstance;
class IObjectInterface;
class CArmedInstance;
VCMI_LIB_NAMESPACE_END

//Created when player starts turn or when player puts game on [ause
//Removed when player accepts a turn or continur play
class TimerPauseQuery : public CQuery
{
public:	
	TimerPauseQuery(CGameHandler * owner, PlayerColor player);
	
	bool blocksPack(const CPackForServer *pack) const override;
	void onAdding(PlayerColor color) override;
	void onRemoval(PlayerColor color) override;
	bool endsByPlayerAnswer() const override;
};

//Created when hero attempts move and something happens
//(not necessarily position change, could be just an object interaction).
class CHeroMovementQuery : public CQuery
{
public:
	TryMoveHero tmh;
	bool visitDestAfterVictory; //if hero moved to guarded tile and it should be visited once guard is defeated
	const CGHeroInstance *hero;

	void onExposure(QueryPtr topQuery) override;

	CHeroMovementQuery(CGameHandler * owner, const TryMoveHero & Tmh, const CGHeroInstance * Hero, bool VisitDestAfterVictory = false);
	void onAdding(PlayerColor color) override;
	void onRemoval(PlayerColor color) override;
};

class CGarrisonDialogQuery : public CDialogQuery //used also for hero exchange dialogs
{
public:
	std::array<const CArmedInstance *,2> exchangingArmies;

	CGarrisonDialogQuery(CGameHandler * owner, const CArmedInstance *up, const CArmedInstance *down);
	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const override;
	bool blocksPack(const CPackForServer *pack) const override;
};

//yes/no and component selection dialogs
class CBlockingDialogQuery : public CDialogQuery
{
public:
	const IObjectInterface * caller;
	BlockingDialog bd; //copy of pack... debug purposes

	CBlockingDialogQuery(CGameHandler * owner, const IObjectInterface * caller, const BlockingDialog &bd);

	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const override;
};

class OpenWindowQuery : public CDialogQuery
{
	EOpenWindowMode mode;
public:
	OpenWindowQuery(CGameHandler * owner, const CGHeroInstance *hero, EOpenWindowMode mode);

	bool blocksPack(const CPackForServer *pack) const override;
	void onExposure(QueryPtr topQuery) override;
};

class CTeleportDialogQuery : public CDialogQuery
{
public:
	TeleportDialog td; //copy of pack... debug purposes

	CTeleportDialogQuery(CGameHandler * owner, const TeleportDialog &td);

	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const override;
};

class CHeroLevelUpDialogQuery : public CDialogQuery
{
public:
	CHeroLevelUpDialogQuery(CGameHandler * owner, const HeroLevelUp &Hlu, const CGHeroInstance * Hero);

	void onRemoval(PlayerColor color) override;
	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const override;

	HeroLevelUp hlu;
	const CGHeroInstance * hero;
};

class CCommanderLevelUpDialogQuery : public CDialogQuery
{
public:
	CCommanderLevelUpDialogQuery(CGameHandler * owner, const CommanderLevelUp &Clu, const CGHeroInstance * Hero);

	void onRemoval(PlayerColor color) override;
	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const override;

	CommanderLevelUp clu;
	const CGHeroInstance * hero;
};
