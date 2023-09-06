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

#include "../JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class IHandlerBase;
class CModInfo;

/// internal type to handle loading of one data type (e.g. artifacts, creatures)
class DLL_LINKAGE ContentTypeHandler
{
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
	std::string objectName;

	/// contains all loaded H3 data
	std::vector<JsonNode> originalData;
	std::map<std::string, ModInfo> modData;

	ContentTypeHandler(IHandlerBase * handler, const std::string & objectName);

	/// local version of methods in ContentHandler
	/// returns true if loading was successful
	bool preloadModData(const std::string & modName, const std::vector<std::string> & fileList, bool validate);
	bool loadMod(const std::string & modName, bool validate);
	void loadCustom();
	void afterLoadFinalization();
};

/// class used to load all game data into handlers. Used only during loading
class DLL_LINKAGE CContentHandler
{
	/// preloads all data from fileList as data from modName.
	bool preloadModData(const std::string & modName, const JsonNode & modConfig, bool validate);

	/// actually loads data in mod
	bool loadMod(const std::string & modName, bool validate);

	std::map<std::string, ContentTypeHandler> handlers;

public:
	void init();

	/// preloads all data from fileList as data from modName.
	void preloadData(CModInfo & mod);

	/// actually loads data in mod
	void load(CModInfo & mod);

	void loadCustom();

	/// all data was loaded, time for final validation / integration
	void afterLoadFinalization();

	const ContentTypeHandler & operator[] (const std::string & name) const;
};


VCMI_LIB_NAMESPACE_END
