/*
 * CRmgTemplateStorage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CRmgTemplate.h"
#include "CRmgTemplateZone.h"
#include "../IHandlerBase.h"

class JsonNode;

typedef std::vector<JsonNode> JsonVector;

/// The CJsonRmgTemplateLoader loads templates from a JSON file.
class DLL_LINKAGE CRmgTemplateStorage : public IHandlerBase
{
public:
	CRmgTemplateStorage();
	~CRmgTemplateStorage();

	const std::map<std::string, CRmgTemplate *> & getTemplates() const;

	std::vector<bool> getDefaultAllowed() const override;
	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	/// loads single object into game. Scope is namespace of this object, same as name of source mod
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

private:
	CRmgTemplate::CSize parseMapTemplateSize(const std::string & text) const;
	CRmgTemplateZone::CTownInfo parseTemplateZoneTowns(const JsonNode & node) const;
	ETemplateZoneType::ETemplateZoneType parseZoneType(const std::string & type) const;
	std::set<TFaction> parseTownTypes(const JsonVector & townTypesVector, const std::set<TFaction> & defaultTownTypes) const;
	std::set<ETerrainType> parseTerrainTypes(const JsonVector & terTypeStrings, const std::set<ETerrainType> & defaultTerrainTypes) const;
	CRmgTemplate::CPlayerCountRange parsePlayers(const std::string & players) const;

protected:
	std::map<std::string, CRmgTemplate *> templates;
};

