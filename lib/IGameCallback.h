/*
 * IGameCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/Metatype.h>

#include "CGameInfoCallback.h" // for CGameInfoCallback
#include "networkPacks/ObjProperty.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{
class RNG;
}

struct SetMovePoints;
struct GiveBonus;
struct BlockingDialog;
struct TeleportDialog;
struct StackLocation;
struct ArtifactLocation;
struct BankConfig;
struct BattleLayout;
class CCreatureSet;
class CStackBasicDescriptor;
class CGCreature;
class IObjectInterface;
enum class EOpenWindowMode : uint8_t;

namespace spells
{
	class Caster;
}

namespace Rewardable
{
	struct Configuration;
}

#if SCRIPTING_ENABLED
namespace scripting
{
	class Pool;
}
#endif


class DLL_LINKAGE CPrivilegedInfoCallback : public CGameInfoCallback
{
public:
	using CGameInfoCallback::gameState; // make public

	//used for random spawns
	void getFreeTiles(std::vector<int3> &tiles) const;

	//mode 1 - only unrevealed tiles; mode 0 - all, mode -1 -  only revealed
	void getTilesInRange(std::unordered_set<int3> & tiles,
						 const int3 & pos,
						 int radius,
						 ETileVisibility mode,
						 std::optional<PlayerColor> player = std::optional<PlayerColor>(),
						 int3::EDistanceFormula formula = int3::DIST_2D) const;

	//returns all tiles on given level (-1 - both levels, otherwise number of level)
	void getAllTiles(std::unordered_set<int3> &tiles, std::optional<PlayerColor> player, int level, std::function<bool(const TerrainTile *)> filter) const;

	//gives 3 treasures, 3 minors, 1 major -> used by Black Market and Artifact Merchant
	void pickAllowedArtsSet(std::vector<ArtifactID> & out, vstd::RNG & rand);
	void getAllowedSpells(std::vector<SpellID> &out, std::optional<ui16> level = std::nullopt);
};

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
	virtual void changePrimSkill(const CGHeroInstance * hero, PrimarySkill which, si64 val, bool abs=false)=0;
	virtual void changeSecSkill(const CGHeroInstance * hero, SecondarySkill which, int val, bool abs=false)=0;
	virtual void showBlockingDialog(const IObjectInterface * caller, BlockingDialog *iw) =0;
	virtual void showGarrisonDialog(ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits) =0; //cb will be called when player closes garrison window
	virtual void showTeleportDialog(TeleportDialog *iw) =0;
	virtual void showObjectWindow(const CGObjectInstance * object, EOpenWindowMode window, const CGHeroInstance * visitor, bool addQuery) = 0;
	virtual void giveResource(PlayerColor player, GameResID which, int val)=0;
	virtual void giveResources(PlayerColor player, TResources resources)=0;

	virtual void giveCreatures(const CArmedInstance *objid, const CGHeroInstance * h, const CCreatureSet &creatures, bool remove) =0;
	virtual void takeCreatures(ObjectInstanceID objid, const std::vector<CStackBasicDescriptor> &creatures, bool forceRemoval = false) =0;
	virtual bool changeStackCount(const StackLocation &sl, TQuantity count, bool absoluteValue = false) =0;
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
	virtual bool swapGarrisonOnSiege(ObjectInstanceID tid)=0;
	virtual void giveHeroBonus(GiveBonus * bonus)=0;
	virtual void setMovePoints(SetMovePoints * smp)=0;
	virtual void setMovePoints(ObjectInstanceID hid, int val, bool absolute)=0;
	virtual void setManaPoints(ObjectInstanceID hid, int val)=0;
	virtual void giveHero(ObjectInstanceID id, PlayerColor player, ObjectInstanceID boatId = ObjectInstanceID()) = 0;
	virtual void changeObjPos(ObjectInstanceID objid, int3 newPos, const PlayerColor & initiator)=0;
	virtual void sendAndApply(CPackForClient & pack) = 0;
	virtual void heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2)=0; //when two heroes meet on adventure map
	virtual void changeFogOfWar(int3 center, ui32 radius, PlayerColor player, ETileVisibility mode) = 0;
	virtual void changeFogOfWar(const std::unordered_set<int3> &tiles, PlayerColor player, ETileVisibility mode) = 0;
	
	virtual void castSpell(const spells::Caster * caster, SpellID spellID, const int3 &pos) = 0;

	virtual vstd::RNG & getRandomGenerator() = 0;

};

class DLL_LINKAGE CNonConstInfoCallback : public CPrivilegedInfoCallback
{
public:
	//keep const version of callback accessible
	using CGameInfoCallback::getPlayerState;
	using CGameInfoCallback::getTeam;
	using CGameInfoCallback::getPlayerTeam;
	using CGameInfoCallback::getHero;
	using CGameInfoCallback::getTown;
	using CGameInfoCallback::getTile;
	using CGameInfoCallback::getArtInstance;
	using CGameInfoCallback::getObjInstance;
	using CGameInfoCallback::getArtSet;

	PlayerState * getPlayerState(const PlayerColor & color, bool verbose = true);
	TeamState * getTeam(const TeamID & teamID); //get team by team ID
	TeamState * getPlayerTeam(const PlayerColor & color); // get team by player color
	CGHeroInstance * getHero(const ObjectInstanceID & objid);
	CGTownInstance * getTown(const ObjectInstanceID & objid);
	TerrainTile * getTile(const int3 & pos);
	CArtifactInstance * getArtInstance(const ArtifactInstanceID & aid);
	CGObjectInstance * getObjInstance(const ObjectInstanceID & oid);
	CArmedInstance * getArmyInstance(const ObjectInstanceID & oid);
	CArtifactSet * getArtSet(const ArtifactLocation & loc);

	virtual void updateEntity(Metatype metatype, int32_t index, const JsonNode & data) = 0;
};

/// Interface class for handling general game logic and actions
class DLL_LINKAGE IGameCallback : public CPrivilegedInfoCallback, public IGameEventCallback
{
public:
	virtual ~IGameCallback(){};

#if SCRIPTING_ENABLED
	virtual scripting::Pool * getGlobalContextPool() const = 0;
#endif

	//get info
	virtual bool isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero);

	friend struct CPack;
	friend struct CPackForClient;
	friend struct CPackForServer;
};


VCMI_LIB_NAMESPACE_END
