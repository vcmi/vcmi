/*
 * IObjectInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../GameCallbackHolder.h"
#include "../constants/EntityIdentifiers.h"
#include "../networkPacks/EInfoWindowMode.h"
#include "../networkPacks/ObjProperty.h"
#include "../serializer/Serializeable.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{
class RNG;
}

struct BattleResult;
struct UpgradeInfo;
class BoatId;
class CGObjectInstance;
class CStackInstance;
class CGHeroInstance;
class IGameCallback;
class ResourceSet;
class int3;
class MetaString;
class PlayerColor;

class DLL_LINKAGE IObjectInterface : public GameCallbackHolder, public virtual Serializeable
{
public:
	using GameCallbackHolder::GameCallbackHolder;

	virtual ~IObjectInterface() = default;

	virtual MapObjectID getObjGroupIndex() const = 0;
	virtual MapObjectSubID getObjTypeIndex() const = 0;

	virtual PlayerColor getOwner() const = 0;
	virtual int3 visitablePos() const = 0;
	virtual int3 getPosition() const = 0;

	virtual void onHeroVisit(const CGHeroInstance * h) const;
	virtual void onHeroLeave(const CGHeroInstance * h) const;

	/// Called on new turn by server. This method can not modify object state on its own
	/// Instead all changes must be propagated via netpacks
	virtual void newTurn(vstd::RNG & rand) const;
	virtual void initObj(vstd::RNG & rand); //synchr
	virtual void pickRandomObject(vstd::RNG & rand);
	virtual void setProperty(ObjProperty what, ObjPropertyID identifier);//synchr

	//Called when queries created DURING HERO VISIT are resolved
	//First parameter is always hero that visited object and triggered the query
	virtual void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const;
	virtual void blockingDialogAnswered(const CGHeroInstance *hero, int32_t answer) const;
	virtual void garrisonDialogClosed(const CGHeroInstance *hero) const;
	virtual void heroLevelUpDone(const CGHeroInstance *hero) const;

	//unified helper to show info dialog for object owner
	virtual void showInfoDialog(const ui32 txtID, const ui16 soundID = 0, EInfoWindowMode mode = EInfoWindowMode::AUTO) const;

	//unified interface, AI helpers
	virtual bool wasVisited (PlayerColor player) const;
	virtual bool wasVisited (const CGHeroInstance * h) const;

	static void preInit(); //called before objs receive their initObj
	static void postInit();//called after objs receive their initObj

	template <typename Handler> void serialize(Handler &h)
	{
		logGlobal->error("IObjectInterface serialized, unexpected, should not happen!");
	}
};

class DLL_LINKAGE ICreatureUpgrader
{
public:
	virtual void fillUpgradeInfo(UpgradeInfo & info, const CStackInstance &stack) const = 0;

	virtual ~ICreatureUpgrader() = default;
};

class DLL_LINKAGE IBoatGenerator
{
public:
	virtual ~IBoatGenerator() = default;

	virtual const IObjectInterface * getObject() const = 0;

	virtual BoatId getBoatType() const = 0; //0 - evil (if a ship can be evil...?), 1 - good, 2 - neutral
	virtual void getOutOffsets(std::vector<int3> & offsets) const = 0; //offsets to obj pos when we boat can be placed
	int3 bestLocation() const; //returns location when the boat should be placed

	enum EGeneratorState {GOOD, BOAT_ALREADY_BUILT, TILE_BLOCKED, NO_WATER, UNKNOWN};
	virtual EGeneratorState shipyardStatus() const;
	void getProblemText(MetaString &out, const CGHeroInstance *visitor = nullptr) const;
};

class DLL_LINKAGE IShipyard : public IBoatGenerator
{
public:
	virtual void getBoatCost(ResourceSet & cost) const;
};

VCMI_LIB_NAMESPACE_END
