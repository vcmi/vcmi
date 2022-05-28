/*
 * CRmgTemplateStorage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CRmgTemplateStorage.h"
#include "CRmgTemplate.h"

#include "../serializer/JsonDeserializer.h"

using namespace rmg;

void CRmgTemplateStorage::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	//unused
	loadObject(scope, name, data);
}

void CRmgTemplateStorage::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	try
	{
		JsonDeserializer handler(nullptr, data);
		auto fullKey = normalizeIdentifier(scope, "core", name); //actually it's not used
		templates[fullKey].setId(name);
		templates[fullKey].serializeJson(handler);
		templates[fullKey].validate();
	}
	catch(const std::exception & e)
	{
		logGlobal->error("Template %s has errors. Message: %s.", name, std::string(e.what()));
	}
}

std::vector<bool> CRmgTemplateStorage::getDefaultAllowed() const
{
	//all templates are allowed
	return std::vector<bool>();
}

std::vector<JsonNode> CRmgTemplateStorage::loadLegacyData(size_t dataSize)
{
	return std::vector<JsonNode>();
	//it would be cool to load old rmg.txt files
}

const CRmgTemplate * CRmgTemplateStorage::getTemplate(const std::string & templateName) const
{
	auto iter = templates.find(templateName);
	if(iter==templates.end())
		return nullptr;
	return &iter->second;
}

std::vector<const CRmgTemplate *> CRmgTemplateStorage::getTemplates() const
{
	std::vector<const CRmgTemplate *> result;
	for(auto i=templates.cbegin(); i!=templates.cend(); ++i)
	{
		result.push_back(&i->second);
	}
	return result;
}

std::vector<const CRmgTemplate *> CRmgTemplateStorage::getTemplates(const int3& filterSize, si8 filterPlayers, si8 filterHumanPlayers, si8 filterCpuPlayers) const
{
	std::vector<const CRmgTemplate *> result;
	for(auto i=templates.cbegin(); i!=templates.cend(); ++i)
	{
		auto& tmpl = i->second;
		
		if (!tmpl.matchesSize(filterSize))
			continue;
		
		if (filterPlayers != -1)
		{
			if (!tmpl.getPlayers().isInRange(filterPlayers))
				continue;
		}
		else
		{
			// Human players shouldn't be banned when playing with random player count
			if (filterHumanPlayers > *boost::min_element(tmpl.getPlayers().getNumbers()))
				continue;
		}
		
		if(filterCpuPlayers != -1)
		{
			if (!tmpl.getCpuPlayers().isInRange(filterCpuPlayers))
				continue;
		}
		
		result.push_back(&i->second);
	}
	return result;
}
