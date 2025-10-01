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

VCMI_LIB_NAMESPACE_BEGIN

class GameResID;

class DLL_LINKAGE ResourceType : public EntityT<GameResID>
{
	virtual int getPrice() const = 0;
};


VCMI_LIB_NAMESPACE_END
