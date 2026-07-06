/*
 * mock_IGameEventCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/ServerCallback.h>

#include "../../lib/callback/IGameEventCallback.h"
#include "../../lib/int3.h"
#include "../../lib/ResourceSet.h"
#include "../../lib/networkPacks/PacksForClient.h"

class GameEventCallbackMock : public IGameEventCallback
{
public:
	using UpperCallback = ::ServerCallback;

	GameEventCallbackMock(UpperCallback * upperCallback_);
	virtual ~GameEventCallbackMock();

	// Tests set this once gameState->init has populated CMap so the mock can
	// resolve object identifiers without owning the gameState itself.
	void setGameInfoCallback(IGameInfoCallback * cb) { infoCallback = cb; }

	// ---- captured dialog queue (consumed by QuestTest::answerDialog etc.) --
	// showBlockingDialog / showInfoDialog get pushed here instead of being
	// dispatched to a UI. Tests inspect text / components and drive the answer
	// via the returned struct's `answer` field.
	struct CapturedBlockingDialog
	{
		BlockingDialog dialog;
		const IObjectInterface * caller = nullptr;
	};
	std::vector<CapturedBlockingDialog> blockingDialogs;
	std::vector<InfoWindow>             infoWindows;
	std::vector<AddQuest>               addedQuests;

	// ---- non-empty overrides (defined in mock_IGameEventCallback.cpp) ----

	void setObjPropertyValue(ObjectInstanceID objid, ObjProperty prop, int32_t value) override;
	void setObjPropertyID(ObjectInstanceID objid, ObjProperty prop, ObjPropertyID identifier) override;
	void showInfoDialog(InfoWindow * iw) override;
	bool removeObject(const CGObjectInstance * obj, const PlayerColor & initiator) override;
	void giveExperience(const CGHeroInstance * hero, TExpType val) override;
	void showBlockingDialog(const IObjectInterface * caller, BlockingDialog * iw) override;
	void giveResource(PlayerColor player, GameResID which, int val) override;
	void giveResources(PlayerColor player, const ResourceSet & resources) override;
	void takeCreatures(ObjectInstanceID objid, const std::vector<CStackBasicDescriptor> & creatures, bool forceRemoval) override;
	bool changeStackCount(const StackLocation & sl, TQuantity count, ChangeValueMode mode) override;
	bool eraseStack(const StackLocation & sl, bool forceRemoval) override;
	void removeAfterVisit(const ObjectInstanceID & id) override;
	void removeArtifact(const ArtifactLocation & al) override;
	void sendAndApply(CPackForClient & pack) override;
	vstd::RNG & getRandomGenerator() override;

	// ---- no-op stubs ----------------------------------------------------

	void setRewardableObjectConfiguration(ObjectInstanceID, const Rewardable::Configuration &) override {}
	void setRewardableObjectConfiguration(ObjectInstanceID, BuildingID, const Rewardable::Configuration &) override {}
	void changeSpells(const CGHeroInstance *, bool, const std::set<SpellID> &) override {}
	void setResearchedSpells(const CGTownInstance *, int, const std::vector<SpellID> &, bool) override {}
	void createBoat(const int3 &, BoatId, PlayerColor) override {}
	void setOwner(const CGObjectInstance *, PlayerColor) override {}
	void changePrimSkill(const CGHeroInstance *, PrimarySkill, si64, ChangeValueMode) override {}
	void changeSecSkill(const CGHeroInstance *, SecondarySkill, int, ChangeValueMode) override {}
	void showGarrisonDialog(ObjectInstanceID, ObjectInstanceID, bool, const MetaString &) override {}
	void showTeleportDialog(TeleportDialog *) override {}
	void showObjectWindow(const CGObjectInstance *, EOpenWindowMode, const CGHeroInstance *, bool) override {}
	void giveCreatures(const CGHeroInstance *, const CCreatureSet &) override {}
	void giveCreatures(const CArmedInstance *, const CGHeroInstance *, const CCreatureSet &, bool) override {}
	bool changeStackType(const StackLocation &, const CCreature *) override { return false; }
	bool insertNewStack(const StackLocation &, const CCreature *, TQuantity) override { return false; }
	bool swapStacks(const StackLocation &, const StackLocation &) override { return false; }
	bool addToSlot(const StackLocation &, const CCreature *, TQuantity) override { return false; }
	void tryJoiningArmy(const CArmedInstance *, const CArmedInstance *, bool, bool) override {}
	bool moveStack(const StackLocation &, const StackLocation &, TQuantity) override { return false; }
	bool giveHeroNewArtifact(const CGHeroInstance *, const ArtifactID &, const ArtifactPosition &) override { return false; }
	bool giveHeroNewScroll(const CGHeroInstance *, const SpellID &, const ArtifactPosition &) override { return false; }
	bool putArtifact(const ArtifactLocation &, const ArtifactInstanceID &, std::optional<bool>) override { return false; }
	bool moveArtifact(const PlayerColor &, const ArtifactLocation &, const ArtifactLocation &) override { return false; }
	void heroVisitCastle(const CGTownInstance *, const CGHeroInstance *) override {}
	void stopHeroVisitCastle(const CGTownInstance *, const CGHeroInstance *) override {}
	void visitCastleObjects(const CGTownInstance *, const CGHeroInstance *) override {}
	void startBattle(const CArmedInstance *, const CArmedInstance *, int3, const CGHeroInstance *, const CGHeroInstance *, const BattleLayout &, const CGTownInstance *) override {}
	void startBattle(const CArmedInstance *, const CArmedInstance *) override {}
	bool moveHero(ObjectInstanceID, int3, EMovementMode, bool, PlayerColor, const EPathfindingLayer &) override { return false; }
	void giveHeroBonus(GiveBonus *) override {}
	void setMovePoints(SetMovePoints *) override {}
	void setMovePoints(ObjectInstanceID, int) override {}
	void setManaPoints(ObjectInstanceID, int) override {}
	void giveHero(ObjectInstanceID, PlayerColor, ObjectInstanceID) override {}
	void changeObjPos(ObjectInstanceID, int3, const PlayerColor &) override {}
	void heroExchange(ObjectInstanceID, ObjectInstanceID) override {}
	void changeFogOfWar(int3, ui32, PlayerColor, ETileVisibility) override {}
	void changeFogOfWar(const FowTilesType &, PlayerColor, ETileVisibility) override {}
	void castSpell(const spells::Caster *, SpellID, const int3 &) override {}
	bool isVisitCoveredByAnotherQuery(const CGObjectInstance *, const CGHeroInstance *) override { return false; }

private:
	UpperCallback *      upperCallback = nullptr;
	IGameInfoCallback *  infoCallback  = nullptr;
};
