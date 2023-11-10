/*
 * RegisterTypes.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "RegisterTypesClientPacks.h"
#include "RegisterTypesLobbyPacks.h"
#include "RegisterTypesMapObjects.h"
#include "RegisterTypesServerPacks.h"

VCMI_LIB_NAMESPACE_BEGIN

template<typename Serializer>
void registerTypes(Serializer &s)
{
	registerTypesMapObjects(s);
	registerTypesClientPacks(s);
	registerTypesServerPacks(s);
	registerTypesLobbyPacks(s);
}

VCMI_LIB_NAMESPACE_END
