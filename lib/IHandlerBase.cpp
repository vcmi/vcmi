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
#include "modding/IdentifierStorage.h"
#include "modding/ModScope.h"
#include "modding/CModHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

std::string IHandlerBase::getScopeBuiltin()
{
	return ModScope::scopeBuiltin();
}

void IHandlerBase::registerObject(const std::string & scope, const std::string & type_name, const std::string & name, si32 index)
{
	return VLC->modh->getIdentifiers().registerObject(scope, type_name, name, index);
}

VCMI_LIB_NAMESPACE_END
