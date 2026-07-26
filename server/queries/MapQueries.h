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

class CGHeroInstance;
class CGObjectInstance;
class IObjectInterface;
class CArmedInstance;

//Created when player starts turn or when player puts game on [ause
//Removed when player accepts a turn or continur play
class TimerPauseQuery : public CQuery
{
public:
	static constexpr QueryType TYPE = QueryType::TimerPause;

	TimerPauseQuery(CGameHandler * owner, PlayerColor player);

	bool blocksPack(const CPackForServer *pack) const override;
	void onExposure(QueryPtr topQuery) override;
	void onAdding(PlayerColor color) override;
	void onRemoval(PlayerColor color) override;
	bool endsByPlayerAnswer() const override;
};

//Created when hero attempts move and something happens
//(not necessarily position change, could be just an object interaction).
class CHeroMovementQuery : public CQuery
{
public:
	static constexpr QueryType TYPE = QueryType::HeroMovement;

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
	static constexpr QueryType TYPE = QueryType::GarrisonDialog;

	std::array<const CArmedInstance *,2> exchangingArmies;

	CGarrisonDialogQuery(CGameHandler * owner, const CArmedInstance *up, const CArmedInstance *down);
	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const override;
	bool blocksPack(const CPackForServer *pack) const override;
};

//yes/no and component selection dialogs
class CBlockingDialogQuery : public CDialogQuery
{
public:
	static constexpr QueryType TYPE = QueryType::BlockingDialog;

	const IObjectInterface * caller;
	BlockingDialog bd; //copy of pack... debug purposes

	CBlockingDialogQuery(CGameHandler * owner, const IObjectInterface * caller, const BlockingDialog &bd);

	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const override;
};

class OpenWindowQuery : public CDialogQuery
{
	EOpenWindowMode mode;
public:
	static constexpr QueryType TYPE = QueryType::OpenWindow;

	OpenWindowQuery(CGameHandler * owner, const CGHeroInstance *hero, EOpenWindowMode mode);

	bool blocksPack(const CPackForServer *pack) const override;
	void onExposure(QueryPtr topQuery) override;
};

/// Keeps a paused map-script coroutine alive between blocking actions. Sits on the query stack between
/// the object-visit query and whatever child (dialog / battle) the script spawned; when that child is
/// removed this query is exposed and resumes the coroutine, popping itself once the script finishes.
class LuaScriptQuery : public CQuery
{
public:
	static constexpr QueryType TYPE = QueryType::LuaScript;

	LuaScriptQuery(CGameHandler * owner, PlayerColor player);

	void setCoroutine(int handle);
	void setPendingAnswer(std::optional<int32_t> answer);
	void setVisitingHero(ObjectInstanceID hero);

	void onExposure(QueryPtr topQuery) override;

private:
	int coroutineHandle = 0;
	std::optional<int32_t> pendingAnswer;
	ObjectInstanceID visitingHero; //if set and the hero is gone (lost a scripted combat), the coroutine is abandoned
};

class CTeleportDialogQuery : public CDialogQuery
{
public:
	static constexpr QueryType TYPE = QueryType::TeleportDialog;

	TeleportDialog td; //copy of pack... debug purposes

	CTeleportDialogQuery(CGameHandler * owner, const TeleportDialog & dialog);

	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const override;
};

class CHeroLevelUpDialogQuery : public CDialogQuery
{
public:
	static constexpr QueryType TYPE = QueryType::HeroLevelUpDialog;

	CHeroLevelUpDialogQuery(CGameHandler * owner, const HeroLevelUp &Hlu, const CGHeroInstance * Hero);

	void onRemoval(PlayerColor color) override;
	void onAdded(PlayerColor color) override;
	void onExposure(QueryPtr topQuery) override;
	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const override;

	HeroLevelUp hlu;
	const CGHeroInstance * hero;
	bool prompted = false;
};

class CCommanderLevelUpDialogQuery : public CDialogQuery
{
public:
	static constexpr QueryType TYPE = QueryType::CommanderLevelUpDialog;

	CCommanderLevelUpDialogQuery(CGameHandler * owner, const CommanderLevelUp &Clu, const CGHeroInstance * Hero);

	void onRemoval(PlayerColor color) override;
	void onExposure(QueryPtr topQuery) override;
	void onAdded(PlayerColor color) override;
	void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const override;

	CommanderLevelUp clu;
	const CGHeroInstance * hero;
	bool prompted = false;
};
