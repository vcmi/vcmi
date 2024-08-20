/*
 * CBuilding.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../constants/EntityIdentifiers.h"
#include "../../LogicalExpression.h"
#include "../../ResourceSet.h"
#include "../../bonuses/BonusList.h"
#include "../../rewardable/Info.h"

VCMI_LIB_NAMESPACE_BEGIN

class CTown;

/// a typical building encountered in every castle ;]
/// this is structure available to both client and server
/// contains all mechanics-related data about town structures
class DLL_LINKAGE CBuilding
{
	std::string modScope;
	std::string identifier;

public:
	using TRequired = LogicalExpression<BuildingID>;

	CTown * town; // town this building belongs to
	TResources resources;
	TResources produce;
	TRequired requirements;

	BuildingID bid; //structure ID
	BuildingID upgrade; /// indicates that building "upgrade" can be improved by this, -1 = empty
	BuildingSubID::EBuildingSubID subId; /// subtype for special buildings, -1 = the building is not special
	bool upgradeReplacesBonuses = false;
	BonusList buildingBonuses;

	Rewardable::Info rewardableObjectInfo; ///configurable rewards for special buildings

	enum EBuildMode
	{
		BUILD_NORMAL,  // 0 - normal, default
		BUILD_AUTO,    // 1 - auto - building appears when all requirements are built
		BUILD_SPECIAL, // 2 - special - building can not be built normally
		BUILD_GRAIL    // 3 - grail - building requires grail to be built
	} mode;

	enum ETowerHeight // for lookup towers and some grails
	{
		HEIGHT_NO_TOWER = 5, // building has not 'lookout tower' ability
		HEIGHT_LOW = 10,     // low lookout tower, but castle without lookout tower gives radius 5
		HEIGHT_AVERAGE = 15,
		HEIGHT_HIGH = 20,    // such tower is in the Tower town
		HEIGHT_SKYSHIP = std::numeric_limits<int>::max()  // grail, open entire map
	} height;

	static const std::map<std::string, CBuilding::EBuildMode> MODES;
	static const std::map<std::string, CBuilding::ETowerHeight> TOWER_TYPES;

	CBuilding() : town(nullptr), mode(BUILD_NORMAL) {};

	BuildingTypeUniqueID getUniqueTypeID() const;

	std::string getJsonKey() const;

	std::string getNameTranslated() const;
	std::string getDescriptionTranslated() const;

	std::string getBaseTextID() const;
	std::string getNameTextID() const;
	std::string getDescriptionTextID() const;

	//return base of upgrade(s) or this
	BuildingID getBase() const;

	// returns how many times build has to be upgraded to become build
	si32 getDistance(const BuildingID & build) const;

	STRONG_INLINE
		bool IsTradeBuilding() const
	{
		return bid == BuildingID::MARKETPLACE || subId == BuildingSubID::ARTIFACT_MERCHANT || subId == BuildingSubID::FREELANCERS_GUILD;
	}

	void addNewBonus(const std::shared_ptr<Bonus> & b, BonusList & bonusList) const;

	friend class CTownHandler;
};

VCMI_LIB_NAMESPACE_END
