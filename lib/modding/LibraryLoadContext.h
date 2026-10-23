/*
 * LibraryLoadContext.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ContentTypeHandler.h"
#include "IdentifierResolver.h"

class GameLibrary;

/// State of one library load, passed to all code that loads library content. Exists only while the library is loading
class DLL_LINKAGE LibraryLoadContext : boost::noncopyable
{
public:
	/// Library that is being loaded
	GameLibrary & lib;

	/// Resolves identifiers requested by loaded content once content of all mods is loaded
	DeferredIdentifierResolver resolver;

	/// Content of all entity types, with json of all loaded mods
	CContentHandler content;

	/// Contents of all files that mods list in their mod.json, in the same layout as mod.json itself
	std::map<std::string, JsonNode> modContent;

	/// Checksums of files of every active mod
	std::map<std::string, uint32_t> modChecksums;

	/// Mods that have no issues found while loading their content
	std::set<std::string> validationPassed;

	/// Mods that list a file that does not exist in their filesystem
	std::set<std::string> modsWithMissingFiles;

	explicit LibraryLoadContext(GameLibrary & lib);
};
