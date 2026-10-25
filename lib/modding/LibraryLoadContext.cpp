/*
 * LibraryLoadContext.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "LibraryLoadContext.h"

#include "IdentifierStorage.h"

#include "../GameLibrary.h"

LibraryLoadContext::LibraryLoadContext(GameLibrary & lib)
	: lib(lib)
	, resolver(*lib.identifiersHandler)
	, content(*this)
{
}
