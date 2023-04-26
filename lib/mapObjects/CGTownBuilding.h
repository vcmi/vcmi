/*
 * CGTownBuilding.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CObjectHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGTownInstance;


class DLL_LINKAGE CGTownBuilding : public IObjectInterface
{
///basic class for town structures handled as map objects
public:
	si32 indexOnTV = 0; //identifies its index on towns vector
	
	CGTownInstance * town = nullptr;

	STRONG_INLINE
	BuildingSubID::EBuildingSubID getBuildingSubtype() const
	{
		return bType;
	}

	STRONG_INLINE
	const BuildingID & getBuildingType() const
	{
		return bID;
	}

	STRONG_INLINE
	void setBuildingSubtype(BuildingSubID::EBuildingSubID subId)
	{
		bType = subId;
	}

	PlayerColor getOwner() const override;
	int32_t getObjGroupIndex() const override;
	int32_t getObjTypeIndex() const override;

	int3 visitablePos() const override;
	int3 getPosition() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & bID;
		h & indexOnTV;
		h & bType;
	}

protected:
	BuildingID bID; //from buildig list
	BuildingSubID::EBuildingSubID bType = BuildingSubID::NONE;

	std::string getVisitingBonusGreeting() const;
	std::string getCustomBonusGreeting(const Bonus & bonus) const;
};

class DLL_LINKAGE COPWBonus : public CGTownBuilding
{///used for OPW bonusing structures
public:
	std::set<si32> visitors;
	void setProperty(ui8 what, ui32 val) override;
	void onHeroVisit (const CGHeroInstance * h) const override;

	COPWBonus(const BuildingID & index, BuildingSubID::EBuildingSubID subId, CGTownInstance * TOWN);
	COPWBonus() = default;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGTownBuilding&>(*this);
		h & visitors;
	}
};

class DLL_LINKAGE CTownBonus : public CGTownBuilding
{
///used for one-time bonusing structures
///feel free to merge inheritance tree
public:
	std::set<ObjectInstanceID> visitors;
	void setProperty(ui8 what, ui32 val) override;
	void onHeroVisit (const CGHeroInstance * h) const override;

	CTownBonus(const BuildingID & index, BuildingSubID::EBuildingSubID subId, CGTownInstance * TOWN);
	CTownBonus() = default;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGTownBuilding&>(*this);
		h & visitors;
	}

private:
	void applyBonuses(CGHeroInstance * h, const BonusList & bonuses) const;
};

VCMI_LIB_NAMESPACE_END
