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

#include "../NetPacksBase.h"

VCMI_LIB_NAMESPACE_BEGIN

struct BattleResult;
struct UpgradeInfo;
class CGObjectInstance;
class CRandomGenerator;
class IGameCallback;
class ResourceSet;
class int3;
class MetaString;

class DLL_LINKAGE IObjectInterface
{
public:
	static IGameCallback *cb;

	virtual ~IObjectInterface() = default;

	virtual int32_t getObjGroupIndex() const = 0;
	virtual int32_t getObjTypeIndex() const = 0;

	virtual PlayerColor getOwner() const = 0;
	virtual int3 visitablePos() const = 0;
	virtual int3 getPosition() const = 0;

	virtual void onHeroVisit(const CGHeroInstance * h) const;
	virtual void onHeroLeave(const CGHeroInstance * h) const;
	virtual void newTurn(CRandomGenerator & rand) const;
	virtual void initObj(CRandomGenerator & rand); //synchr
	virtual void setProperty(ui8 what, ui32 val);//synchr

	//Called when queries created DURING HERO VISIT are resolved
	//First parameter is always hero that visited object and triggered the query
	virtual void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const;
	virtual void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const;
	virtual void garrisonDialogClosed(const CGHeroInstance *hero) const;
	virtual void heroLevelUpDone(const CGHeroInstance *hero) const;

	//unified helper to show info dialog for object owner
	virtual void showInfoDialog(const ui32 txtID, const ui16 soundID = 0, EInfoWindowMode mode = EInfoWindowMode::AUTO) const;

	//unified helper to show a specific window
	static void openWindow(const EOpenWindowMode type, const int id1, const int id2 = -1);

	//unified interface, AI helpers
	virtual bool wasVisited (PlayerColor player) const;
	virtual bool wasVisited (const CGHeroInstance * h) const;

	static void preInit(); //called before objs receive their initObj
	static void postInit();//called after objs receive their initObj

	template <typename Handler> void serialize(Handler &h, const int version)
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

	static const IShipyard *castFrom(const CGObjectInstance *obj);
};

VCMI_LIB_NAMESPACE_END
