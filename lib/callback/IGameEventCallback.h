/*
 * IGameEventCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../GameConstants.h"
#include "../networkPacks/ObjProperty.h"

VCMI_LIB_NAMESPACE_BEGIN

class int3;
struct GiveBonus;
struct CPackForClient;
struct SetMovePoints;
struct BattleLayout;
struct ArtifactLocation;
struct InfoWindow;
struct TeleportDialog;
struct BlockingDialog;
struct StackLocation;

class CStackBasicDescriptor;
class ResourceSet;
class CGHeroInstance;
class CArmedInstance;
class CGTownInstance;
class CCreatureSet;
class CGObjectInstance;
class IObjectInterface;

enum class EOpenWindowMode : uint8_t;

namespace Rewardable
{
struct Configuration;
}

namespace spells
{
class Caster;
}

namespace vstd
{
class RNG;
}

class DLL_LINKAGE IGameEventCallback
{
public:
	virtual void setObjPropertyValue(ObjectInstanceID objid, ObjProperty prop, int32_t value = 0) = 0;
	virtual void setRewardableObjectConfiguration(ObjectInstanceID mapObjectID, const Rewardable::Configuration & configuration) = 0;
	virtual void setRewardableObjectConfiguration(ObjectInstanceID townInstanceID, BuildingID buildingID, const Rewardable::Configuration & configuration) = 0;
	virtual void setObjPropertyID(ObjectInstanceID objid, ObjProperty prop, ObjPropertyID identifier) = 0;

	virtual void showInfoDialog(InfoWindow * iw) = 0;

	virtual void changeSpells(const CGHeroInstance * hero, bool give, const std::set<SpellID> &spells)=0;
	virtual void setResearchedSpells(const CGTownInstance * town, int level, const std::vector<SpellID> & spells, bool accepted)=0;
	virtual bool removeObject(const CGObjectInstance * obj, const PlayerColor & initiator) = 0;
	virtual void createBoat(const int3 & visitablePosition, BoatId type, PlayerColor initiator) = 0;
	virtual void setOwner(const CGObjectInstance * objid, PlayerColor owner)=0;
	virtual void giveExperience(const CGHeroInstance * hero, TExpType val) =0;
	virtual void changePrimSkill(const CGHeroInstance * hero, PrimarySkill which, si64 val, ChangeValueMode mode)=0;
	virtual void changeSecSkill(const CGHeroInstance * hero, SecondarySkill which, int val, ChangeValueMode mode)=0;
	virtual void showBlockingDialog(const IObjectInterface * caller, BlockingDialog *iw) =0;
	virtual void showGarrisonDialog(ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits) =0; //cb will be called when player closes garrison window
	virtual void showTeleportDialog(TeleportDialog *iw) =0;
	virtual void showObjectWindow(const CGObjectInstance * object, EOpenWindowMode window, const CGHeroInstance * visitor, bool addQuery) = 0;
	virtual void giveResource(PlayerColor player, GameResID which, int val)=0;
	virtual void giveResources(PlayerColor player, const ResourceSet & resources)=0;

	virtual void giveCreatures(const CGHeroInstance * h, const CCreatureSet &creatures) =0;
	virtual void giveCreatures(const CArmedInstance *objid, const CGHeroInstance * h, const CCreatureSet &creatures, bool remove) =0;
	virtual void takeCreatures(ObjectInstanceID objid, const std::vector<CStackBasicDescriptor> &creatures, bool forceRemoval = false) =0;
	virtual bool changeStackCount(const StackLocation &sl, TQuantity count, ChangeValueMode mode) =0;
	virtual bool changeStackType(const StackLocation &sl, const CCreature *c) =0;
	virtual bool insertNewStack(const StackLocation &sl, const CCreature *c, TQuantity count = -1) =0; //count -1 => moves whole stack
	virtual bool eraseStack(const StackLocation &sl, bool forceRemoval = false) =0;
	virtual bool swapStacks(const StackLocation &sl1, const StackLocation &sl2) =0;
	virtual bool addToSlot(const StackLocation &sl, const CCreature *c, TQuantity count) =0; //makes new stack or increases count of already existing
	virtual void tryJoiningArmy(const CArmedInstance *src, const CArmedInstance *dst, bool removeObjWhenFinished, bool allowMerging) =0; //merges army from src do dst or opens a garrison window
	virtual bool moveStack(const StackLocation &src, const StackLocation &dst, TQuantity count) = 0;

	virtual void removeAfterVisit(const ObjectInstanceID & id) = 0; //object will be destroyed when interaction is over. Do not call when interaction is not ongoing!

	virtual bool giveHeroNewArtifact(const CGHeroInstance * h, const ArtifactID & artId, const ArtifactPosition & pos) = 0;
	virtual bool giveHeroNewScroll(const CGHeroInstance * h, const SpellID & spellId, const ArtifactPosition & pos) = 0;
	virtual bool putArtifact(const ArtifactLocation & al, const ArtifactInstanceID & id, std::optional<bool> askAssemble = std::nullopt) = 0;
	virtual void removeArtifact(const ArtifactLocation& al) = 0;
	virtual bool moveArtifact(const PlayerColor & player, const ArtifactLocation & al1, const ArtifactLocation & al2) = 0;

	virtual void heroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero)=0;
	virtual void visitCastleObjects(const CGTownInstance * obj, const CGHeroInstance * hero)=0;
	virtual void stopHeroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero)=0;
	virtual void startBattle(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, const BattleLayout & layout, const CGTownInstance *town)=0; //use hero=nullptr for no hero
	virtual void startBattle(const CArmedInstance *army1, const CArmedInstance *army2)=0; //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle
	virtual bool moveHero(ObjectInstanceID hid, int3 dst, EMovementMode moveMove, bool transit = false, PlayerColor asker = PlayerColor::NEUTRAL)=0;
	virtual void giveHeroBonus(GiveBonus * bonus)=0;
	virtual void setMovePoints(SetMovePoints * smp)=0;
	virtual void setMovePoints(ObjectInstanceID hid, int val)=0;
	virtual void setManaPoints(ObjectInstanceID hid, int val)=0;
	virtual void giveHero(ObjectInstanceID id, PlayerColor player, ObjectInstanceID boatId = ObjectInstanceID()) = 0;
	virtual void changeObjPos(ObjectInstanceID objid, int3 newPos, const PlayerColor & initiator)=0;
	virtual void sendAndApply(CPackForClient & pack) = 0;
	virtual void heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2)=0; //when two heroes meet on adventure map
	virtual void changeFogOfWar(int3 center, ui32 radius, PlayerColor player, ETileVisibility mode) = 0;
	virtual void changeFogOfWar(const std::unordered_set<int3> &tiles, PlayerColor player, ETileVisibility mode) = 0;

	virtual void castSpell(const spells::Caster * caster, SpellID spellID, const int3 &pos) = 0;

	virtual bool isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero) = 0;

	/// Returns global random generator. TODO: remove, replace with IGameRanndomizer as separate parameter to such methods
	virtual vstd::RNG & getRandomGenerator() = 0;
};

VCMI_LIB_NAMESPACE_END
