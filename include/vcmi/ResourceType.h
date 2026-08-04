/*
 * ResourceType.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Entity.h"
#include "scripting/ApiTags.h"

class GameResID;

class DLL_LINKAGE ResourceType : public EntityT<GameResID>, public scripting::ApiRawPointer<ResourceType>
{
	virtual int getPrice() const = 0;
};

