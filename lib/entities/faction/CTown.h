/*
 * CTown.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../ConstTransitivePtr.h"
#include "../../Point.h"
#include "../../constants/EntityIdentifiers.h"
#include "../../constants/Enumerations.h"
#include "../../filesystem/ResourcePath.h"
#include "../../int3.h"

VCMI_LIB_NAMESPACE_BEGIN

class CBuilding;

/// This is structure used only by client
/// Consists of all gui-related data about town structures
/// Should be moved from lib to client
struct DLL_LINKAGE CStructure
{
	CBuilding * building;  // base building. If null - this structure will be always present on screen
	CBuilding * buildable; // building that will be used to determine built building and visible cost. Usually same as "building"

	int3 pos;
	AnimationPath defName;
	ImagePath borderName;
	ImagePath areaName;
	std::string identifier;

	bool hiddenUpgrade; // used only if "building" is upgrade, if true - structure on town screen will behave exactly like parent (mouse clicks, hover texts, etc)
};

class DLL_LINKAGE CTown
{
	friend class CTownHandler;
	size_t namesCount = 0;

public:
	CTown();
	~CTown();

	std::string getBuildingScope() const;
	std::set<si32> getAllBuildings() const;
	const CBuilding * getSpecialBuilding(BuildingSubID::EBuildingSubID subID) const;
	std::string getGreeting(BuildingSubID::EBuildingSubID subID) const;
	void setGreeting(BuildingSubID::EBuildingSubID subID, const std::string & message) const; //may affect only mutable field
	BuildingID getBuildingType(BuildingSubID::EBuildingSubID subID) const;

	std::string getRandomNameTextID(size_t index) const;
	size_t getRandomNamesCount() const;

	CFaction * faction;

	/// level -> list of creatures on this tier
	// TODO: replace with pointers to CCreature
	std::vector<std::vector<CreatureID> > creatures;

	std::map<BuildingID, ConstTransitivePtr<CBuilding> > buildings;

	std::vector<std::string> dwellings; //defs for adventure map dwellings for new towns, [0] means tier 1 creatures etc.
	std::vector<std::string> dwellingNames;

	// should be removed at least from configs in favor of auto-detection
	std::map<int,int> hordeLvl; //[0] - first horde building creature level; [1] - second horde building (-1 if not present)
	ui32 mageLevel; //max available mage guild level
	GameResID primaryRes;
	ArtifactID warMachine;
	SpellID moatAbility;

	// default chance for hero of specific class to appear in tavern, if field "tavern" was not set
	// resulting chance = sqrt(town.chance * heroClass.chance)
	ui32 defaultTavernChance;

	// Client-only data. Should be moved away from lib
	struct ClientInfo
	{
		//icons [fort is present?][build limit reached?] -> index of icon in def files
		int icons[2][2];
		std::string iconSmall[2][2]; /// icon names used during loading
		std::string iconLarge[2][2];
		VideoPath tavernVideo;
		std::vector<AudioPath> musicTheme;
		ImagePath townBackground;
		ImagePath guildBackground;
		ImagePath guildWindow;
		AnimationPath buildingsIcons;
		ImagePath hallBackground;
		/// vector[row][column] = list of buildings in this slot
		std::vector< std::vector< std::vector<BuildingID> > > hallSlots;

		/// list of town screen structures.
		/// NOTE: index in vector is meaningless. Vector used instead of list for a bit faster access
		std::vector<ConstTransitivePtr<CStructure> > structures;

		std::string siegePrefix;
		std::vector<Point> siegePositions;
		CreatureID siegeShooter; // shooter creature ID
		std::string towerIconSmall;
		std::string towerIconLarge;

	} clientInfo;

private:
	///generated bonusing buildings messages for all towns of this type.
	mutable std::map<BuildingSubID::EBuildingSubID, const std::string> specialMessages; //may be changed by CGTownBuilding::getVisitingBonusGreeting() const
};

VCMI_LIB_NAMESPACE_END
