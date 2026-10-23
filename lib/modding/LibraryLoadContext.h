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

	explicit LibraryLoadContext(GameLibrary & lib);
};
