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

#include "../GameLibrary.h"
#include "../modding/LibraryLoadContext.h"
#include "../serializer/JsonDeserializer.h"
#include "../texts/CGeneralTextHandler.h"

using namespace rmg;

void CRmgTemplateStorage::loadObject(LibraryLoadContext & context, const std::string & scope, const std::string & name, const JsonNode & data, size_t index)
{
	//unused
	loadObject(context, scope, name, data);
}

void CRmgTemplateStorage::afterLoadFinalization(LibraryLoadContext & context)
{
	for (auto& temp : templates)
	{
		temp.second->afterLoad(context.lib);
	}
}

void CRmgTemplateStorage::loadObject(LibraryLoadContext & context, const std::string & scope, const std::string & name, const JsonNode & data)
{
	try
	{
		JsonDeserializer handler(nullptr, context.resolver, data);
		auto fullKey = scope + ":" + name; //actually it's not used
		auto loadedTemplate = std::make_shared<CRmgTemplate>();
		loadedTemplate->setId(fullKey);
		loadedTemplate->serializeJson(handler, *context.lib.objtypeh);
		loadedTemplate->setName(name);
		loadedTemplate->validate();

		if (!loadedTemplate->getDescription().empty())
			context.lib.generaltexth->registerString(scope, loadedTemplate->getDescriptionTextID(), data["description"]);

		templates[fullKey] = loadedTemplate;
	}
	catch(const std::exception & e)
	{
		logMod->error("Mod '%s': random map template '%s' is invalid and will not be loaded: %s", scope, name, e.what());
	}
}

std::vector<JsonNode> CRmgTemplateStorage::loadLegacyData(LibraryLoadContext & context)
{
	return std::vector<JsonNode>();
	//it would be cool to load old rmg.txt files
}

const CRmgTemplate * CRmgTemplateStorage::getTemplate(const std::string & templateName) const
{
	auto iter = templates.find(templateName);
	if(iter==templates.end())
		return nullptr;
	return iter->second.get();
}

std::vector<const CRmgTemplate *> CRmgTemplateStorage::getTemplates() const
{
	std::vector<const CRmgTemplate *> result;
	result.reserve(templates.size());
	for(const auto & i : templates)
	{
		result.push_back(i.second.get());
	}
	return result;
}
