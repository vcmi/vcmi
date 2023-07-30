/*
 * ContentTypeHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ContentTypeHandler.h"

#include "CModHandler.h"
#include "CModInfo.h"
#include "ModScope.h"

#include "../BattleFieldHandler.h"
#include "../CArtHandler.h"
#include "../CCreatureHandler.h"
#include "../CGeneralTextHandler.h"
#include "../CHeroHandler.h"
#include "../CSkillHandler.h"
#include "../CStopWatch.h"
#include "../CTownHandler.h"
#include "../GameSettings.h"
#include "../IHandlerBase.h"
#include "../Languages.h"
#include "../ObstacleHandler.h"
#include "../RiverHandler.h"
#include "../RoadHandler.h"
#include "../ScriptHandler.h"
#include "../StringConstants.h"
#include "../TerrainHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../rmg/CRmgTemplateStorage.h"
#include "../spells/CSpellHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

ContentTypeHandler::ContentTypeHandler(IHandlerBase * handler, const std::string & objectName):
	handler(handler),
	objectName(objectName),
	originalData(handler->loadLegacyData())
{
	for(auto & node : originalData)
	{
		node.setMeta(ModScope::scopeBuiltin());
	}
}

bool ContentTypeHandler::preloadModData(const std::string & modName, const std::vector<std::string> & fileList, bool validate)
{
	bool result = false;
	JsonNode data = JsonUtils::assembleFromFiles(fileList, result);
	data.setMeta(modName);

	ModInfo & modInfo = modData[modName];

	for(auto entry : data.Struct())
	{
		size_t colon = entry.first.find(':');

		if (colon == std::string::npos)
		{
			// normal object, local to this mod
			modInfo.modData[entry.first].swap(entry.second);
		}
		else
		{
			std::string remoteName = entry.first.substr(0, colon);
			std::string objectName = entry.first.substr(colon + 1);

			// patching this mod? Send warning and continue - this situation can be handled normally
			if (remoteName == modName)
				logMod->warn("Redundant namespace definition for %s", objectName);

			logMod->trace("Patching object %s (%s) from %s", objectName, remoteName, modName);
			JsonNode & remoteConf = modData[remoteName].patches[objectName];

			JsonUtils::merge(remoteConf, entry.second);
		}
	}
	return result;
}

bool ContentTypeHandler::loadMod(const std::string & modName, bool validate)
{
	ModInfo & modInfo = modData[modName];
	bool result = true;

	auto performValidate = [&,this](JsonNode & data, const std::string & name){
		handler->beforeValidate(data);
		if (validate)
			result &= JsonUtils::validate(data, "vcmi:" + objectName, name);
	};

	// apply patches
	if (!modInfo.patches.isNull())
		JsonUtils::merge(modInfo.modData, modInfo.patches);

	for(auto & entry : modInfo.modData.Struct())
	{
		const std::string & name = entry.first;
		JsonNode & data = entry.second;

		if (data.meta != modName)
			logMod->warn("Mod %s is attempting to inject object %s into mod %s! This may not be supported in future versions!", data.meta, name, modName);

		if (vstd::contains(data.Struct(), "index") && !data["index"].isNull())
		{
			if (modName != "core")
				logMod->warn("Mod %s is attempting to load original data! This should be reserved for built-in mod.", modName);

			// try to add H3 object data
			size_t index = static_cast<size_t>(data["index"].Float());

			if(originalData.size() > index)
			{
				logMod->trace("found original data in loadMod(%s) at index %d", name, index);
				JsonUtils::merge(originalData[index], data);
				std::swap(originalData[index], data);
				originalData[index].clear(); // do not use same data twice (same ID)
			}
			else
			{
				logMod->trace("no original data in loadMod(%s) at index %d", name, index);
			}
			performValidate(data, name);
			handler->loadObject(modName, name, data, index);
		}
		else
		{
			// normal new object
			logMod->trace("no index in loadMod(%s)", name);
			performValidate(data,name);
			handler->loadObject(modName, name, data);
		}
	}
	return result;
}

void ContentTypeHandler::loadCustom()
{
	handler->loadCustom();
}

void ContentTypeHandler::afterLoadFinalization()
{
	handler->afterLoadFinalization();
}

void CContentHandler::init()
{
	handlers.insert(std::make_pair("heroClasses", ContentTypeHandler(&VLC->heroh->classes, "heroClass")));
	handlers.insert(std::make_pair("artifacts", ContentTypeHandler(VLC->arth, "artifact")));
	handlers.insert(std::make_pair("creatures", ContentTypeHandler(VLC->creh, "creature")));
	handlers.insert(std::make_pair("factions", ContentTypeHandler(VLC->townh, "faction")));
	handlers.insert(std::make_pair("objects", ContentTypeHandler(VLC->objtypeh, "object")));
	handlers.insert(std::make_pair("heroes", ContentTypeHandler(VLC->heroh, "hero")));
	handlers.insert(std::make_pair("spells", ContentTypeHandler(VLC->spellh, "spell")));
	handlers.insert(std::make_pair("skills", ContentTypeHandler(VLC->skillh, "skill")));
	handlers.insert(std::make_pair("templates", ContentTypeHandler(VLC->tplh, "template")));
#if SCRIPTING_ENABLED
	handlers.insert(std::make_pair("scripts", ContentTypeHandler(VLC->scriptHandler, "script")));
#endif
	handlers.insert(std::make_pair("battlefields", ContentTypeHandler(VLC->battlefieldsHandler, "battlefield")));
	handlers.insert(std::make_pair("terrains", ContentTypeHandler(VLC->terrainTypeHandler, "terrain")));
	handlers.insert(std::make_pair("rivers", ContentTypeHandler(VLC->riverTypeHandler, "river")));
	handlers.insert(std::make_pair("roads", ContentTypeHandler(VLC->roadTypeHandler, "road")));
	handlers.insert(std::make_pair("obstacles", ContentTypeHandler(VLC->obstacleHandler, "obstacle")));
	//TODO: any other types of moddables?
}

bool CContentHandler::preloadModData(const std::string & modName, JsonNode modConfig, bool validate)
{
	bool result = true;
	for(auto & handler : handlers)
	{
		result &= handler.second.preloadModData(modName, modConfig[handler.first].convertTo<std::vector<std::string> >(), validate);
	}
	return result;
}

bool CContentHandler::loadMod(const std::string & modName, bool validate)
{
	bool result = true;
	for(auto & handler : handlers)
	{
		result &= handler.second.loadMod(modName, validate);
	}
	return result;
}

void CContentHandler::loadCustom()
{
	for(auto & handler : handlers)
	{
		handler.second.loadCustom();
	}
}

void CContentHandler::afterLoadFinalization()
{
	for(auto & handler : handlers)
	{
		handler.second.afterLoadFinalization();
	}
}

void CContentHandler::preloadData(CModInfo & mod)
{
	bool validate = (mod.validation != CModInfo::PASSED);

	// print message in format [<8-symbols checksum>] <modname>
	logMod->info("\t\t[%08x]%s", mod.checksum, mod.name);

	if (validate && mod.identifier != ModScope::scopeBuiltin())
	{
		if (!JsonUtils::validate(mod.config, "vcmi:mod", mod.identifier))
			mod.validation = CModInfo::FAILED;
	}
	if (!preloadModData(mod.identifier, mod.config, validate))
		mod.validation = CModInfo::FAILED;
}

void CContentHandler::load(CModInfo & mod)
{
	bool validate = (mod.validation != CModInfo::PASSED);

	if (!loadMod(mod.identifier, validate))
		mod.validation = CModInfo::FAILED;

	if (validate)
	{
		if (mod.validation != CModInfo::FAILED)
			logMod->info("\t\t[DONE] %s", mod.name);
		else
			logMod->error("\t\t[FAIL] %s", mod.name);
	}
	else
		logMod->info("\t\t[SKIP] %s", mod.name);
}

const ContentTypeHandler & CContentHandler::operator[](const std::string & name) const
{
	return handlers.at(name);
}

VCMI_LIB_NAMESPACE_END
