/*
 * ContentTypeHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class IHandlerBase;
class ModDescription;

/// internal type to handle loading of one data type (e.g. artifacts, creatures)
class DLL_LINKAGE ContentTypeHandler
{
	JsonNode conflictList;

public:
	struct ModInfo
	{
		/// mod data from this mod and for this mod
		JsonNode modData;
		/// mod data for this mod from other mods (patches)
		JsonNode patches;
	};
	/// handler to which all data will be loaded
	IHandlerBase * handler;
	std::string entityName;

	/// contains all loaded H3 data
	std::vector<JsonNode> originalData;
	std::map<std::string, ModInfo> modData;

	ContentTypeHandler(IHandlerBase * handler, const std::string & objectName);

	/// local version of methods in ContentHandler
	/// returns true if loading was successful
	bool preloadModData(const std::string & modName, const JsonNode & fileList, bool validate);
	bool loadMod(const std::string & modName, bool validate);
	void loadCustom();
	void afterLoadFinalization();
};

/// class used to load all game data into handlers. Used only during loading
class DLL_LINKAGE CContentHandler
{
	std::map<std::string, ContentTypeHandler> handlers;

public:
	void init();

	/// preloads all data from fileList as data from modName.
	bool preloadData(const ModDescription & mod, bool validateMod);

	/// actually loads data in mod
	bool load(const ModDescription & mod, bool validateMod);

	void loadCustom();

	/// all data was loaded, time for final validation / integration
	void afterLoadFinalization();

	const ContentTypeHandler & operator[] (const std::string & name) const;
};


VCMI_LIB_NAMESPACE_END
