/*
 * IGameInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/EntityIdentifiers.h"
#include "../constants/Enumerations.h"

VCMI_LIB_NAMESPACE_BEGIN

class int3;
struct StartInfo;

class CGHeroInstance;
class CGObjectInstance;
class Player;

class DLL_LINKAGE IGameInfoCallback : boost::noncopyable
{
public:
	//TODO: all other public methods of CGameInfoCallback

	//	//various
	virtual int getDate(Date mode=Date::DAY) const = 0; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	virtual const StartInfo * getStartInfo() const = 0;
	virtual const StartInfo * getInitialStartInfo() const = 0;
	virtual bool isAllowed(SpellID id) const = 0;
	virtual bool isAllowed(ArtifactID id) const = 0;
	virtual bool isAllowed(SecondarySkill id) const = 0;

	//player
	virtual std::optional<PlayerColor> getPlayerID() const = 0;
	virtual const Player * getPlayer(PlayerColor color) const = 0;
	//	virtual int getResource(PlayerColor Player, EGameResID which) const = 0;
	//	bool isVisible(int3 pos) const;
	//	PlayerRelations getPlayerRelations(PlayerColor color1, PlayerColor color2) const;
	//	void getThievesGuildInfo(SThievesGuildInfo & thi, const CGObjectInstance * obj); //get thieves' guild info obtainable while visiting given object
	//	EPlayerStatus getPlayerStatus(PlayerColor player, bool verbose = true) const; //-1 if no such player
	//	const PlayerSettings * getPlayerSettings(PlayerColor color) const;


	//	//armed object
	//	void fillUpgradeInfo(const CArmedInstance *obj, SlotID stackPos, UpgradeInfo &out)const;

	//hero
	virtual const CGHeroInstance * getHero(ObjectInstanceID objid) const = 0;
	//	int getHeroCount(PlayerColor player, bool includeGarrisoned) const;
	//	bool getHeroInfo(const CGObjectInstance * hero, InfoAboutHero & dest, const CGObjectInstance * selectedObject = nullptr) const;
	//	int32_t getSpellCost(const spells::Spell * sp, const CGHeroInstance * caster) const; //when called during battle, takes into account creatures' spell cost reduction
	//	int64_t estimateSpellDamage(const CSpell * sp, const CGHeroInstance * hero) const; //estimates damage of given spell; returns 0 if spell causes no dmg
	//	const CArtifactInstance * getArtInstance(ArtifactInstanceID aid) const;
	//	const CGObjectInstance * getObjInstance(ObjectInstanceID oid) const;
	//	const CGObjectInstance * getArmyInstance(ObjectInstanceID oid) const;

	//objects
	virtual const CGObjectInstance * getObj(ObjectInstanceID objid, bool verbose = true) const = 0;
	//	std::vector <const CGObjectInstance * > getBlockingObjs(int3 pos) const;
	virtual std::vector<const CGObjectInstance *> getVisitableObjs(int3 pos, bool verbose = true) const = 0;
	//	std::vector <const CGObjectInstance * > getFlaggableObjects(int3 pos) const;
	//	const CGObjectInstance * getTopObj (int3 pos) const;
	//	PlayerColor getOwner(ObjectInstanceID heroID) const;

	//map
	//	int3 guardingCreaturePosition (int3 pos) const;
	//	std::vector<const CGObjectInstance*> getGuardingCreatures (int3 pos) const;
	//	const CMapHeader * getMapHeader()const;
	//	int3 getMapSize() const; //returns size of map - z is 1 for one - level map and 2 for two level map
	//	const TerrainTile * getTile(int3 tile, bool verbose = true) const;
	//	bool isInTheMap(const int3 &pos) const;

	//town
	//	const CGTownInstance* getTown(ObjectInstanceID objid) const;
	//	int howManyTowns(PlayerColor Player) const;
	//	const CGTownInstance * getTownInfo(int val, bool mode)const; //mode = 0 -> val = player town serial; mode = 1 -> val = object id (serial)
	//	std::vector<const CGHeroInstance *> getAvailableHeroes(const CGObjectInstance * townOrTavern) const; //heroes that can be recruited
	//	std::string getTavernRumor(const CGObjectInstance * townOrTavern) const;
	//	EBuildingState canBuildStructure(const CGTownInstance *t, BuildingID ID);//// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements
	//	virtual bool getTownInfo(const CGObjectInstance * town, InfoAboutTown & dest, const CGObjectInstance * selectedObject = nullptr) const;

	//from gs
	//	const TeamState *getTeam(TeamID teamID) const;
	//	const TeamState *getPlayerTeam(PlayerColor color) const;
	//	EBuildingState canBuildStructure(const CGTownInstance *t, BuildingID ID) const;// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements

	//teleport
	//	std::vector<ObjectInstanceID> getVisibleTeleportObjects(std::vector<ObjectInstanceID> ids, PlayerColor player)  const;
	//	std::vector<ObjectInstanceID> getTeleportChannelEntrances(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const;
	//	std::vector<ObjectInstanceID> getTeleportChannelExits(TeleportChannelID id, PlayerColor Player = PlayerColor::UNFLAGGABLE) const;
	//	ETeleportChannelType getTeleportChannelType(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
	//	bool isTeleportChannelImpassable(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
	//	bool isTeleportChannelBidirectional(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
	//	bool isTeleportChannelUnidirectional(TeleportChannelID id, PlayerColor player = PlayerColor::UNFLAGGABLE) const;
	//	bool isTeleportEntrancePassable(const CGTeleport * obj, PlayerColor player) const;
};

VCMI_LIB_NAMESPACE_END
