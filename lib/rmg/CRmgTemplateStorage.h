
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

class JsonNode;

typedef std::vector<JsonNode> JsonVector;

/// The CRmgTemplateLoader is a abstract base class for loading templates.
class DLL_LINKAGE CRmgTemplateLoader
{
public:
	virtual ~CRmgTemplateLoader() { };
	virtual void loadTemplates() = 0;
	const std::map<std::string, CRmgTemplate *> & getTemplates() const;

protected:
	std::map<std::string, CRmgTemplate *> templates;
};

/// The CJsonRmgTemplateLoader loads templates from a JSON file.
class DLL_LINKAGE CJsonRmgTemplateLoader : public CRmgTemplateLoader
{
public:
	void loadTemplates() override;

private:
	CRmgTemplate::CSize parseMapTemplateSize(const std::string & text) const;
	CRmgTemplateZone::CTownInfo parseTemplateZoneTowns(const JsonNode & node) const;
	ETemplateZoneType::ETemplateZoneType parseZoneType(const std::string & type) const;
	std::set<TFaction> parseTownTypes(const JsonVector & townTypesVector, const std::set<TFaction> & defaultTownTypes) const;
	std::set<ETerrainType> parseTerrainTypes(const JsonVector & terTypeStrings, const std::set<ETerrainType> & defaultTerrainTypes) const;
	CRmgTemplate::CPlayerCountRange parsePlayers(const std::string & players) const;
};

/// The class CRmgTemplateStorage stores random map templates.
class DLL_LINKAGE CRmgTemplateStorage
{
public:
	CRmgTemplateStorage();
	~CRmgTemplateStorage();

	const std::map<std::string, CRmgTemplate *> & getTemplates() const;

private:
	std::map<std::string, CRmgTemplate *> templates; /// Key: Template name
};
