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

const std::map<std::string, CRmgTemplate *> & CRmgTemplateStorage::getTemplates() const
{
	return templates;
}

void CRmgTemplateStorage::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	//unused
	loadObject(scope, name, data);
}

void CRmgTemplateStorage::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto tpl = new CRmgTemplate();
	try
	{
		JsonDeserializer handler(nullptr, data);
		auto fullKey = normalizeIdentifier(scope, "core", name);
		tpl->setId(name);
		tpl->serializeJson(handler);
		tpl->validate();
		templates[fullKey] = tpl;
	}
	catch(const std::exception & e)
	{
		logGlobal->error("Template %s has errors. Message: %s.", tpl->getName(), std::string(e.what()));
	}
}

CRmgTemplateStorage::CRmgTemplateStorage()
{
}

CRmgTemplateStorage::~CRmgTemplateStorage()
{
	for (auto & pair : templates) delete pair.second;
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
