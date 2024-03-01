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
#include "../constants/StringConstants.h"
#include "../TerrainHandler.h"
#include "../json/JsonUtils.h"
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
		node.setModScope(ModScope::scopeBuiltin());
	}
}

bool ContentTypeHandler::preloadModData(const std::string & modName, const std::vector<std::string> & fileList, bool validate)
{
	bool result = false;
	JsonNode data = JsonUtils::assembleFromFiles(fileList, result);
	data.setModScope(modName);

	ModInfo & modInfo = modData[modName];

	for(auto entry : data.Struct())
	{
		size_t colon = entry.first.find(':');

		if (colon == std::string::npos)
		{
			// normal object, local to this mod
			std::swap(modInfo.modData[entry.first], entry.second);
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

		if (data.getModScope() != modName)
		{
			// in this scenario, entire object record comes from another mod
			// normally, this is used to "patch" object from another mod (which is legal)
			// however in this case there is no object to patch. This might happen in such cases:
			// - another mod attempts to add object into this mod (technically can be supported, but might lead to weird edge cases)
			// - another mod attempts to edit object from this mod that no longer exist - DANGER since such patch likely has very incomplete data
			// so emit warning and skip such case
			logMod->warn("Mod '%s' attempts to edit object '%s' of type '%s' from mod '%s' but no such object exist!", data.getModScope(), name, objectName, modName);
			continue;
		}

		bool hasIndex = vstd::contains(data.Struct(), "index") && !data["index"].isNull();

		if (hasIndex && modName != "core")
			logMod->error("Mod %s is attempting to load original data! This option is reserved for built-in mod.", modName);

		if (hasIndex && modName == "core")
		{
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
	for (auto const & data : modData)
	{
		if (data.second.modData.isNull())
		{
			for (auto node : data.second.patches.Struct())
				logMod->warn("Mod '%s' have added patch for object '%s' from mod '%s', but this mod was not loaded or has no new objects.", node.second.getModScope(), node.first, data.first);
		}

		for(auto & otherMod : modData)
		{
			if (otherMod.first == data.first)
				continue;

			if (otherMod.second.modData.isNull())
				continue;

			for(auto & otherObject : otherMod.second.modData.Struct())
			{
				if (data.second.modData.Struct().count(otherObject.first))
				{
					logMod->warn("Mod '%s' have added object with name '%s' that is also available in mod '%s'", data.first, otherObject.first, otherMod.first);
					logMod->warn("Two objects with same name were loaded. Please use form '%s:%s' if mod '%s' needs to modify this object instead", otherMod.first, otherObject.first, data.first);
				}
			}
		}
	}

	handler->afterLoadFinalization();
}

void CContentHandler::init()
{
	handlers.insert(std::make_pair("heroClasses", ContentTypeHandler(VLC->heroclassesh.get(), "heroClass")));
	handlers.insert(std::make_pair("artifacts", ContentTypeHandler(VLC->arth.get(), "artifact")));
	handlers.insert(std::make_pair("creatures", ContentTypeHandler(VLC->creh.get(), "creature")));
	handlers.insert(std::make_pair("factions", ContentTypeHandler(VLC->townh.get(), "faction")));
	handlers.insert(std::make_pair("objects", ContentTypeHandler(VLC->objtypeh.get(), "object")));
	handlers.insert(std::make_pair("heroes", ContentTypeHandler(VLC->heroh.get(), "hero")));
	handlers.insert(std::make_pair("spells", ContentTypeHandler(VLC->spellh.get(), "spell")));
	handlers.insert(std::make_pair("skills", ContentTypeHandler(VLC->skillh.get(), "skill")));
	handlers.insert(std::make_pair("templates", ContentTypeHandler(VLC->tplh.get(), "template")));
#if SCRIPTING_ENABLED
	handlers.insert(std::make_pair("scripts", ContentTypeHandler(VLC->scriptHandler.get(), "script")));
#endif
	handlers.insert(std::make_pair("battlefields", ContentTypeHandler(VLC->battlefieldsHandler.get(), "battlefield")));
	handlers.insert(std::make_pair("terrains", ContentTypeHandler(VLC->terrainTypeHandler.get(), "terrain")));
	handlers.insert(std::make_pair("rivers", ContentTypeHandler(VLC->riverTypeHandler.get(), "river")));
	handlers.insert(std::make_pair("roads", ContentTypeHandler(VLC->roadTypeHandler.get(), "road")));
	handlers.insert(std::make_pair("obstacles", ContentTypeHandler(VLC->obstacleHandler.get(), "obstacle")));
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
	auto & info = mod.getVerificationInfo();
	logMod->info("\t\t[%08x]%s", info.checksum, info.name);

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
			logMod->info("\t\t[DONE] %s", mod.getVerificationInfo().name);
		else
			logMod->error("\t\t[FAIL] %s", mod.getVerificationInfo().name);
	}
	else
		logMod->info("\t\t[SKIP] %s", mod.getVerificationInfo().name);
}

const ContentTypeHandler & CContentHandler::operator[](const std::string & name) const
{
	return handlers.at(name);
}

VCMI_LIB_NAMESPACE_END
