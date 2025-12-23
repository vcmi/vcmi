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
#include "ModDescription.h"
#include "ModManager.h"
#include "ModScope.h"

#include "../BattleFieldHandler.h"
#include "../CCreatureHandler.h"
#include "../CConfigHandler.h"
#include "../campaign/CampaignRegionsHandler.h"
#include "../entities/artifact/CArtHandler.h"
#include "../entities/faction/CTownHandler.h"
#include "../entities/hero/CHeroClassHandler.h"
#include "../entities/hero/CHeroHandler.h"
#include "../entities/ResourceTypeHandler.h"
#include "../texts/CGeneralTextHandler.h"
#include "../CBonusTypeHandler.h"
#include "../CSkillHandler.h"
#include "../CStopWatch.h"
#include "../IGameSettings.h"
#include "../IHandlerBase.h"
#include "../ObstacleHandler.h"
#include "../mapObjects/ObstacleSetHandler.h"
#include "../RiverHandler.h"
#include "../RoadHandler.h"
#include "../ScriptHandler.h"
#include "../constants/StringConstants.h"
#include "../TerrainHandler.h"
#include "../json/JsonUtils.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../rmg/CRmgTemplateStorage.h"
#include "../spells/CSpellHandler.h"
#include "../spells/SpellSchoolHandler.h"
#include "../GameLibrary.h"

VCMI_LIB_NAMESPACE_BEGIN

ContentTypeHandler::ContentTypeHandler(IHandlerBase * handler, const std::string & entityName):
	handler(handler),
	entityName(entityName),
	originalData(handler->loadLegacyData())
{
	for(auto & node : originalData)
	{
		node.setModScope(ModScope::scopeBuiltin());
	}
}

bool ContentTypeHandler::preloadModData(const std::string & modName, const JsonNode & fileList, bool validate)
{
	bool result = true;
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

			if (!remoteConf.isNull() && settings["mods"]["validation"].String() != "off")
				JsonUtils::detectConflicts(conflictList, remoteConf, entry.second, objectName);

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
			result &= JsonUtils::validate(data, "vcmi:" + entityName, name);
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
			logMod->warn("Mod '%s' attempts to edit object '%s' of type '%s' from mod '%s' but no such object exist!", data.getModScope(), name, entityName, modName);
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
			performValidate(data, name);
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
	if (settings["mods"]["validation"].String() != "off")
	{
		for (auto const & data : modData)
		{
			if (data.second.modData.isNull())
			{
				for (const auto & node : data.second.patches.Struct())
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

		for (const auto& [conflictPath, conflictModData] : conflictList.Struct())
		{
			std::set<std::string> conflictingMods;
			std::set<std::string> resolvedConflicts;

			for (auto const & conflictModEntry: conflictModData.Struct())
				conflictingMods.insert(conflictModEntry.first);

			for (auto const & modID : conflictingMods)
			{
				resolvedConflicts.merge(LIBRARY->modh->getModDependencies(modID));
				resolvedConflicts.merge(LIBRARY->modh->getModEnabledSoftDependencies(modID));
			}

			vstd::erase_if(conflictingMods, [&resolvedConflicts](const std::string & entry){ return resolvedConflicts.count(entry);});

			if (conflictingMods.size() < 2)
				continue; // all conflicts were resolved - either via compatibility patch (mod that depends on 2 conflicting mods) or simple mod that depends on another one

			bool allEqual = true;

			for (auto const & modID : conflictingMods)
			{
				if (conflictModData[modID] != conflictModData[*conflictingMods.begin()])
				{
					allEqual = false;
					break;
				}
			}

			if (allEqual)
				continue; // conflict still present, but all mods use the same value for conflicting entry - permit it

			logMod->warn("Potential confict in '%s'", conflictPath);

			for (auto const & modID : conflictingMods)
				logMod->warn("Mod '%s' - value set to %s", modID, conflictModData[modID].toCompactString());
		}
	}

	handler->afterLoadFinalization();
}

void CContentHandler::init()
{
	handlers.insert(std::make_pair("heroClasses", ContentTypeHandler(LIBRARY->heroclassesh.get(), "heroClass")));
	handlers.insert(std::make_pair("artifacts", ContentTypeHandler(LIBRARY->arth.get(), "artifact")));
	handlers.insert(std::make_pair("bonuses", ContentTypeHandler(LIBRARY->bth.get(), "bonus")));
	handlers.insert(std::make_pair("creatures", ContentTypeHandler(LIBRARY->creh.get(), "creature")));
	handlers.insert(std::make_pair("campaignRegions", ContentTypeHandler(LIBRARY->campaignRegions.get(), "campaignRegion")));
	handlers.insert(std::make_pair("factions", ContentTypeHandler(LIBRARY->townh.get(), "faction")));
	handlers.insert(std::make_pair("objects", ContentTypeHandler(LIBRARY->objtypeh.get(), "object")));
	handlers.insert(std::make_pair("heroes", ContentTypeHandler(LIBRARY->heroh.get(), "hero")));
	handlers.insert(std::make_pair("spells", ContentTypeHandler(LIBRARY->spellh.get(), "spell")));
	handlers.insert(std::make_pair("spellSchools", ContentTypeHandler(LIBRARY->spellSchoolHandler.get(), "spellSchool")));
	handlers.insert(std::make_pair("skills", ContentTypeHandler(LIBRARY->skillh.get(), "skill")));
	handlers.insert(std::make_pair("templates", ContentTypeHandler(LIBRARY->tplh.get(), "template")));
#if SCRIPTING_ENABLED
	handlers.insert(std::make_pair("scripts", ContentTypeHandler(LIBRARY->scriptHandler.get(), "script")));
#endif
	handlers.insert(std::make_pair("battlefields", ContentTypeHandler(LIBRARY->battlefieldsHandler.get(), "battlefield")));
	handlers.insert(std::make_pair("terrains", ContentTypeHandler(LIBRARY->terrainTypeHandler.get(), "terrain")));
	handlers.insert(std::make_pair("rivers", ContentTypeHandler(LIBRARY->riverTypeHandler.get(), "river")));
	handlers.insert(std::make_pair("roads", ContentTypeHandler(LIBRARY->roadTypeHandler.get(), "road")));
	handlers.insert(std::make_pair("obstacles", ContentTypeHandler(LIBRARY->obstacleHandler.get(), "obstacle")));
	handlers.insert(std::make_pair("biomes", ContentTypeHandler(LIBRARY->biomeHandler.get(), "biome")));
	handlers.insert(std::make_pair("resources", ContentTypeHandler(LIBRARY->resourceTypeHandler.get(), "resources")));
}

bool CContentHandler::preloadData(const ModDescription & mod, bool validate)
{
	bool result = true;

	if (!JsonUtils::validate(mod.getLocalConfig(), "vcmi:mod", mod.getID()))
		result = false;

	for(auto & handler : handlers)
	{
		result &= handler.second.preloadModData(mod.getID(), mod.getLocalValue(handler.first), validate);
	}
	return result;
}

bool CContentHandler::load(const ModDescription & mod, bool validate)
{
	bool result = true;
	for(auto & handler : handlers)
	{
		result &= handler.second.loadMod(mod.getID(), validate);
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

const ContentTypeHandler & CContentHandler::operator[](const std::string & name) const
{
	return handlers.at(name);
}

VCMI_LIB_NAMESPACE_END
