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

VCMI_LIB_NAMESPACE_BEGIN

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

VCMI_LIB_NAMESPACE_END
