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

#include "../../lib/NetPacks.h"

class TurnTimerHandler;

//Created when player starts turn
//Removed when player accepts a turn
class PlayerStartsTurnQuery : public CGhQuery
{
public:	
	PlayerStartsTurnQuery(CGameHandler * owner, PlayerColor player);
	
	bool blocksPack(const CPack *pack) const override;
	void onAdding(PlayerColor color) override;
	void onRemoval(PlayerColor color) override;
	bool endsByPlayerAnswer() const override;
};

//Created when hero visits object.
//Removed when query above is resolved (or immediately after visit if no queries were created)
class CObjectVisitQuery : public CGhQuery
{
public:
	const CGObjectInstance *visitedObject;
	const CGHeroInstance *visitingHero;
	int3 tile; //may be different than hero pos -> eg. visit via teleport
	bool removeObjectAfterVisit;

	CObjectVisitQuery(CGameHandler * owner, const CGObjectInstance *Obj, const CGHeroInstance *Hero, int3 Tile);

	virtual bool blocksPack(const CPack *pack) const override;
	virtual void onRemoval(PlayerColor color) override;
	virtual void onExposure(QueryPtr topQuery) override;
};

//Created when hero attempts move and something happens
//(not necessarily position change, could be just an object interaction).
class CHeroMovementQuery : public CGhQuery
{
public:
	TryMoveHero tmh;
	bool visitDestAfterVictory; //if hero moved to guarded tile and it should be visited once guard is defeated
	const CGHeroInstance *hero;

	virtual void onExposure(QueryPtr topQuery) override;

	CHeroMovementQuery(CGameHandler * owner, const TryMoveHero & Tmh, const CGHeroInstance * Hero, bool VisitDestAfterVictory = false);
	virtual void onAdding(PlayerColor color) override;
	virtual void onRemoval(PlayerColor color) override;
};


class CGarrisonDialogQuery : public CDialogQuery //used also for hero exchange dialogs
{
public:
	std::array<const CArmedInstance *,2> exchangingArmies;

	CGarrisonDialogQuery(CGameHandler * owner, const CArmedInstance *up, const CArmedInstance *down);
	virtual void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const override;
	virtual bool blocksPack(const CPack *pack) const override;
};

//yes/no and component selection dialogs
class CBlockingDialogQuery : public CDialogQuery
{
public:
	BlockingDialog bd; //copy of pack... debug purposes

	CBlockingDialogQuery(CGameHandler * owner, const BlockingDialog &bd);

	virtual void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const override;
};

class CTeleportDialogQuery : public CDialogQuery
{
public:
	TeleportDialog td; //copy of pack... debug purposes

	CTeleportDialogQuery(CGameHandler * owner, const TeleportDialog &td);

	virtual void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const override;
};

class CHeroLevelUpDialogQuery : public CDialogQuery
{
public:
	CHeroLevelUpDialogQuery(CGameHandler * owner, const HeroLevelUp &Hlu, const CGHeroInstance * Hero);

	virtual void onRemoval(PlayerColor color) override;
	virtual void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const override;

	HeroLevelUp hlu;
	const CGHeroInstance * hero;
};

class CCommanderLevelUpDialogQuery : public CDialogQuery
{
public:
	CCommanderLevelUpDialogQuery(CGameHandler * owner, const CommanderLevelUp &Clu, const CGHeroInstance * Hero);

	virtual void onRemoval(PlayerColor color) override;
	virtual void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const override;

	CommanderLevelUp clu;
	const CGHeroInstance * hero;
};
