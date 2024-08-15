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
	struct BuildingRequirementsHelper
	{
		JsonNode json;
		CBuilding * building;
		CTown * town;
	};

	std::map<CTown *, JsonNode> warMachinesToLoad;
	std::vector<BuildingRequirementsHelper> requirementsToLoad;
	std::vector<BuildingRequirementsHelper> overriddenBidsToLoad; //list of buildings, which bonuses should be overridden.

	static const TPropagatorPtr & emptyPropagator();

	void initializeRequirements();
	void initializeOverridden();
	void initializeWarMachines();

	/// loads CBuilding's into town
	void loadBuildingRequirements(CBuilding * building, const JsonNode & source, std::vector<BuildingRequirementsHelper> & bidsToLoad) const;
	void loadBuilding(CTown * town, const std::string & stringID, const JsonNode & source);
	void loadBuildings(CTown * town, const JsonNode & source);

	std::shared_ptr<Bonus> createBonus(CBuilding * build, BonusType type, int val) const;
	std::shared_ptr<Bonus> createBonus(CBuilding * build, BonusType type, int val, BonusSubtypeID subtype) const;
	std::shared_ptr<Bonus> createBonus(CBuilding * build, BonusType type, int val, BonusSubtypeID subtype, const TPropagatorPtr & prop) const;

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
	CTown * randomTown;
	CFaction * randomFaction;

	CTownHandler();
	~CTownHandler();

	std::vector<JsonNode> loadLegacyData() override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;
	void addBonusesForVanilaBuilding(CBuilding * building) const;

	void loadCustom() override;
	void afterLoadFinalization() override;

	std::set<FactionID> getDefaultAllowed() const;
	std::set<FactionID> getAllowedFactions(bool withTown = true) const;

	static void loadSpecialBuildingBonuses(const JsonNode & source, BonusList & bonusList, CBuilding * building);

protected:
	const std::vector<std::string> & getTypeNames() const override;
	std::shared_ptr<CFaction> loadFromJson(const std::string & scope, const JsonNode & data, const std::string & identifier, size_t index) override;
};

VCMI_LIB_NAMESPACE_END
