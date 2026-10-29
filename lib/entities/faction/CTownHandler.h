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

	void initializeRequirements(LibraryLoadContext & context);

	/// loads CBuilding's into town
	void loadBuildingRequirements(CBuilding * building, const JsonNode & source, std::vector<BuildingRequirementsHelper> & bidsToLoad) const;
	void loadBuilding(LibraryLoadContext & context, CTown * town, const std::string & stringID, const JsonNode & source);
	void loadBuildings(LibraryLoadContext & context, CTown * town, const JsonNode & source);

	/// loads CStructure's into town
	void loadStructure(LibraryLoadContext & context, CTown & town, const std::string & stringID, const JsonNode & source) const;
	void loadStructures(LibraryLoadContext & context, CTown & town, const JsonNode & source) const;

	/// loads town hall vector (hallSlots)
	void loadTownHall(LibraryLoadContext & context, CTown & town, const JsonNode & source) const;
	void loadSiegeScreen(LibraryLoadContext & context, CTown & town, const JsonNode & source) const;

	void loadClientData(LibraryLoadContext & context, CTown & town, const JsonNode & source) const;

	void loadTown(LibraryLoadContext & context, CTown * town, const JsonNode & source);

	void loadPuzzle(LibraryLoadContext & context, CFaction & faction, const JsonNode & source) const;

	void loadRandomFaction(LibraryLoadContext & context);

public:
	std::unique_ptr<CFaction> randomFaction;

	CTownHandler();
	~CTownHandler();

	std::vector<JsonNode> loadLegacyData(LibraryLoadContext & context) override;

	void loadObject(LibraryLoadContext & context, const std::string & scope, const std::string & name, const JsonNode & data) override;
	void loadObject(LibraryLoadContext & context, const std::string & scope, const std::string & name, const JsonNode & data, size_t index) override;

	void loadCustom(LibraryLoadContext & context) override;
	void afterLoadFinalization(LibraryLoadContext & context) override;
	void beforeValidate(JsonNode & object) override;

	std::set<FactionID> getDefaultAllowed() const;
	std::set<FactionID> getAllowedFactions(bool withTown = true) const;

protected:

	void loadBuildingBonuses(LibraryLoadContext & context, const JsonNode & source, BonusList & bonusList, CBuilding * building) const;
	const std::vector<std::string> & getTypeNames() const override;
	std::shared_ptr<CFaction> loadFromJson(LibraryLoadContext & context, const std::string & scope, const JsonNode & data, const std::string & identifier, size_t index) override;
};
