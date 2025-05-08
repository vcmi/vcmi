/*
 * CTownHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/FactionService.h>

#include "CFaction.h"

#include "../../IHandlerBase.h"
#include "../../bonuses/Bonus.h"
#include "../../constants/EntityIdentifiers.h"
#include "../../json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class CBuilding;
class CTown;

class DLL_LINKAGE CTownHandler : public CHandlerBase<FactionID, Faction, CFaction, FactionService>
{
	JsonNode buildingsLibrary;

	struct BuildingRequirementsHelper
	{
		JsonNode json;
		CBuilding * building;
		CTown * town;
	};

	std::vector<BuildingRequirementsHelper> requirementsToLoad;
	std::vector<BuildingRequirementsHelper> overriddenBidsToLoad; //list of buildings, which bonuses should be overridden.

	static const TPropagatorPtr & emptyPropagator();

	void initializeRequirements();

	/// loads CBuilding's into town
	void loadBuildingRequirements(CBuilding * building, const JsonNode & source, std::vector<BuildingRequirementsHelper> & bidsToLoad) const;
	void loadBuilding(CTown * town, const std::string & stringID, const JsonNode & source);
	void loadBuildings(CTown * town, const JsonNode & source);

	/// loads CStructure's into town
	void loadStructure(CTown & town, const std::string & stringID, const JsonNode & source) const;
	void loadStructures(CTown & town, const JsonNode & source) const;

	/// loads town hall vector (hallSlots)
	void loadTownHall(CTown & town, const JsonNode & source) const;
	void loadSiegeScreen(CTown & town, const JsonNode & source) const;

	void loadClientData(CTown & town, const JsonNode & source) const;

	void loadTown(CTown * town, const JsonNode & source);

	void loadPuzzle(CFaction & faction, const JsonNode & source) const;

	void loadRandomFaction();

public:
	std::unique_ptr<CFaction> randomFaction;

	CTownHandler();
	~CTownHandler();

	std::vector<JsonNode> loadLegacyData() override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	void loadCustom() override;
	void afterLoadFinalization() override;
	void beforeValidate(JsonNode & object) override;

	std::set<FactionID> getDefaultAllowed() const;
	std::set<FactionID> getAllowedFactions(bool withTown = true) const;

protected:

	void loadBuildingBonuses(const JsonNode & source, BonusList & bonusList, CBuilding * building) const;
	const std::vector<std::string> & getTypeNames() const override;
	std::shared_ptr<CFaction> loadFromJson(const std::string & scope, const JsonNode & data, const std::string & identifier, size_t index) override;
};

VCMI_LIB_NAMESPACE_END
