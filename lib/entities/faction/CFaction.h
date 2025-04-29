/*
 * CFaction.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/Faction.h>

#include "../../Point.h"
#include "../../constants/EntityIdentifiers.h"
#include "../../constants/Enumerations.h"
#include "../../filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

class CTown;

struct DLL_LINKAGE SPuzzleInfo
{
	Point position;
	ui16 number; //type of puzzle
	ui16 whenUncovered; //determines the sequence of discovering (the lesser it is the sooner puzzle will be discovered)
	ImagePath filename; //file with graphic of this puzzle
};

class DLL_LINKAGE CFaction : public Faction
{
	friend class CTownHandler;
	friend class CBuilding;
	friend class CTown;

	std::string modScope;
	std::string identifier;

	FactionID index = FactionID::NEUTRAL;

	FactionID getFactionID() const override; //This function should not be used

public:
	TerrainId nativeTerrain;
	EAlignment alignment = EAlignment::NEUTRAL;
	bool preferUndergroundPlacement = false;
	bool special = false;

	/// Boat that will be used by town shipyard (if any)
	/// and for placing heroes directly on boat (in map editor, water prisons & taverns)
	BoatId boatType = BoatId::CASTLE;

	std::unique_ptr<CTown> town; //NOTE: can be null

	ImagePath creatureBg120;
	ImagePath creatureBg130;

	std::vector<SPuzzleInfo> puzzleMap;

	CFaction();
	~CFaction();

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	std::string getModScope() const override;
	void registerIcons(const IconRegistar & cb) const override;
	FactionID getId() const override;

	std::string getNameTranslated() const override;
	std::string getNameTextID() const override;
	std::string getDescriptionTranslated() const;
	std::string getDescriptionTextID() const;

	bool hasTown() const override;
	TerrainId getNativeTerrain() const override;
	EAlignment getAlignment() const override;
	BoatId getBoatType() const override;

	void updateFrom(const JsonNode & data);
	void serializeJson(JsonSerializeFormat & handler);
};

VCMI_LIB_NAMESPACE_END
