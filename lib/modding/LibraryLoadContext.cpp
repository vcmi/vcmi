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

#include "CModHandler.h"
#include "IdentifierStorage.h"

#include "../CConfigHandler.h"
#include "../GameLibrary.h"

static EContentValidation readValidationSetting()
{
	const std::string & value = settings["mods"]["validation"].String();
	if (value == "off")
		return EContentValidation::OFF;
	if (value == "full")
		return EContentValidation::FULL;
	return EContentValidation::BASIC;
}

LibraryLoadContext::LibraryLoadContext(GameLibrary & lib)
	: lib(lib)
	, validation(readValidationSetting())
	, resolver(*lib.identifiersHandler)
	, content(*this)
{
	lib.modh->addFileFormats(validator);
}
