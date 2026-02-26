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
#include "json/JsonNode.h"
#include "modding/IdentifierStorage.h"
#include "modding/ModScope.h"
#include "modding/CModHandler.h"
#include "GameLibrary.h"

VCMI_LIB_NAMESPACE_BEGIN

std::string IHandlerBase::getScopeBuiltin()
{
	return ModScope::scopeBuiltin();
}

void IHandlerBase::registerObject(const std::string & scope, const std::vector<std::string> & typeNames, const std::string & name, const JsonNode & data, si32 index)
{
	for(const auto & typeName : typeNames)
		registerObject(scope, typeName, name, data, index);
}

void IHandlerBase::registerObject(const std::string & scope, const std::string & typeName, const std::string & name, const JsonNode & data, si32 index)
{
	LIBRARY->identifiersHandler->registerObject(scope, typeName, name, index);
	for(const auto & compatID : data["compatibilityIdentifiers"].Vector())
	{
		if (name != compatID.String())
			LIBRARY->identifiersHandler->registerObject(scope, typeName, compatID.String(), index);
		else
			logMod->warn("Mod '%s' %s '%s': compatibility identifier has same name as object itself!", scope, typeName, name);
	}
}

VCMI_LIB_NAMESPACE_END
