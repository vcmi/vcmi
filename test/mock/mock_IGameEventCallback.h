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

class GameEventCallbackMock : public IGameEventCallback
{
public:
	using UpperCallback = ::ServerCallback;

	GameEventCallbackMock(UpperCallback * upperCallback_);
	virtual ~GameEventCallbackMock();

	void setObjPropertyValue(ObjectInstanceID objid, ObjProperty prop, int32_t value) override {}
	void setObjPropertyID(ObjectInstanceID objid, ObjProperty prop, ObjPropertyID identifier) override {}
	void setRewardableObjectConfiguration(ObjectInstanceID mapObjectID, const Rewardable::Configuration & configuration) override {}
	void setRewardableObjectConfiguration(ObjectInstanceID townInstanceID, BuildingID buildingID, const Rewardable::Configuration & configuration) override {}

	void showInfoDialog(InfoWindow * iw) override {}

	void changeSpells(const CGHeroInstance * hero, bool give, const std::set<SpellID> &spells) override {}
	void setResearchedSpells(const CGTownInstance * town, int level, const std::vector<SpellID> & spells, bool accepted) override {}
	bool removeObject(const CGObjectInstance * obj, const PlayerColor & initiator) override {return false;}
	void createBoat(const int3 & visitablePosition, BoatId type, PlayerColor initiator) override {}
	void setOwner(const CGObjectInstance * objid, PlayerColor owner) override {}
	void giveExperience(const CGHeroInstance * hero, TExpType val) override {}
	void changePrimSkill(const CGHeroInstance * hero, PrimarySkill which, si64 val, ChangeValueMode mode) override {}
	void changeSecSkill(const CGHeroInstance * hero, SecondarySkill which, int val, ChangeValueMode mode) override {}
	void showBlockingDialog(const IObjectInterface * caller, BlockingDialog *iw) override {}
	void showGarrisonDialog(ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits) override {} //cb will be called when player closes garrison window
	void showTeleportDialog(TeleportDialog *iw) override {}
	void showObjectWindow(const CGObjectInstance * object, EOpenWindowMode window, const CGHeroInstance * visitor, bool addQuery) override {};
	void giveResource(PlayerColor player, GameResID which, int val) override {}
	void giveResources(PlayerColor player, const ResourceSet & resources) override {}

	void giveCreatures(const CGHeroInstance * h, const CCreatureSet &creatures) override{}
	void giveCreatures(const CArmedInstance *objid, const CGHeroInstance * h, const CCreatureSet &creatures, bool remove) override {}
	void takeCreatures(ObjectInstanceID objid, const std::vector<CStackBasicDescriptor> &creatures, bool forceRemoval) override {}
	bool changeStackCount(const StackLocation &sl, TQuantity count, ChangeValueMode mode) override {return false;}
	bool changeStackType(const StackLocation &sl, const CCreature *c) override {return false;}
	bool insertNewStack(const StackLocation &sl, const CCreature *c, TQuantity count) override {return false;} //count -1 => moves whole stack
	bool eraseStack(const StackLocation &sl, bool forceRemoval) override {return false;}
	bool swapStacks(const StackLocation &sl1, const StackLocation &sl2) override {return false;}
	bool addToSlot(const StackLocation &sl, const CCreature *c, TQuantity count) override {return false;} //makes new stack or increases count of already existing
	void tryJoiningArmy(const CArmedInstance *src, const CArmedInstance *dst, bool removeObjWhenFinished, bool allowMerging) override {} //merges army from src do dst or opens a garrison window
	bool moveStack(const StackLocation &src, const StackLocation &dst, TQuantity count) override {return false;}

	void removeAfterVisit(const ObjectInstanceID & id) override {} //object will be destroyed when interaction is over. Do not call when interaction is not ongoing!

	bool giveHeroNewArtifact(const CGHeroInstance * h, const ArtifactID & artId, const ArtifactPosition & pos) override {return false;}
	bool giveHeroNewScroll(const CGHeroInstance * h, const SpellID & spellId, const ArtifactPosition & pos) override {return false;}
	bool putArtifact(const ArtifactLocation & al, const ArtifactInstanceID & id, std::optional<bool> askAssemble) override {return false;}
	void removeArtifact(const ArtifactLocation &al) override {}
	bool moveArtifact(const PlayerColor & player, const ArtifactLocation & al1, const ArtifactLocation & al2) override {return false;}

	void heroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override {}
	void stopHeroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override {}
	void visitCastleObjects(const CGTownInstance * obj, const CGHeroInstance * hero) override {}
	void startBattle(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, const BattleLayout & layout, const CGTownInstance *town) override {} //use hero=nullptr for no hero
	void startBattle(const CArmedInstance *army1, const CArmedInstance *army2) override {}
	bool moveHero(ObjectInstanceID hid, int3 dst, EMovementMode movementMode, bool transit, PlayerColor asker) override {return false;}
	void giveHeroBonus(GiveBonus * bonus) override {}
	void setMovePoints(SetMovePoints * smp) override {}
	void setMovePoints(ObjectInstanceID hid, int val) override {};
	void setManaPoints(ObjectInstanceID hid, int val) override {}
	void giveHero(ObjectInstanceID id, PlayerColor player, ObjectInstanceID boatId) override {}
	void changeObjPos(ObjectInstanceID objid, int3 newPos, const PlayerColor & initiator) override {}
	void heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2) override {} //when two heroes meet on adventure map
	void changeFogOfWar(int3 center, ui32 radius, PlayerColor player, ETileVisibility mode) override {}
	void changeFogOfWar(const std::unordered_set<int3> &tiles, PlayerColor player, ETileVisibility mode) override {}
	void castSpell(const spells::Caster * caster, SpellID spellID, const int3 &pos) override {}

	///useful callback methods
	void sendAndApply(CPackForClient & pack) override;

	bool isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero) override {return false;}

	vstd::RNG & getRandomGenerator() override;

private:
	UpperCallback * upperCallback;
};
