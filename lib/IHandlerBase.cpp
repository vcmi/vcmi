/*
 * IHandlerBase.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "IHandlerBase.h"
#include "CModHandler.h"


void IHandlerBase::registerObject(std::string scope, std::string type_name, std::string name, si32 index)
{
	return VLC->modh->identifiers.registerObject(scope, type_name, name, index);
}
