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

class IHandlerBase;
class LibraryLoadContext;
class ModDescription;

/// internal type to handle loading of one data type (e.g. artifacts, creatures)
class ContentTypeHandler
{
	LibraryLoadContext & context;
	JsonNode conflictList;

public:
	struct ModInfo
	{
		/// mod data from this mod and for this mod
		JsonNode modData;
		/// mod data for this mod from other mods (patches)
		/// patches[object name] -> list of patches from different mods
		std::map<std::string, std::vector<JsonNode>> patches;
	};
	/// handler to which all data will be loaded
	IHandlerBase * handler;
	std::string entityName;

	/// contains all loaded H3 data
	std::vector<JsonNode> originalData;
	std::map<std::string, ModInfo> modData;

	ContentTypeHandler(LibraryLoadContext & context, IHandlerBase * handler, const std::string & objectName);

	/// local version of methods in ContentHandler
	/// returns true if loading was successful
	void preloadModData(const std::string & modName, JsonNode & data);
	bool loadMod(const std::string & modName, bool validate);
	void loadCustom();
	void afterLoadFinalization();
};

/// class used to load all game data into handlers. Used only during loading
class DLL_LINKAGE CContentHandler
{
	LibraryLoadContext & context;
	std::map<std::string, ContentTypeHandler> handlers;

public:
	explicit CContentHandler(LibraryLoadContext & context);

	void init();

	/// Returns names of all types of content, e.g. 'creatures' or 'artifacts'
	std::vector<std::string> getContentTypeNames() const;

	/// preloads all data of a mod, taking ownership of contents of provided node
	bool preloadData(const ModDescription & mod, JsonNode & modContent, bool validateMod);

	/// actually loads data in mod
	bool load(const ModDescription & mod, bool validateMod);

	void loadCustom();

	/// all data was loaded, time for final validation / integration
	void afterLoadFinalization();

	const ContentTypeHandler & operator[] (const std::string & name) const;
};

