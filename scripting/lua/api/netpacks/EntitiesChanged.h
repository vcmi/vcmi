/*
 * api/netpacks/EntitiesChanged.h part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "PackForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
namespace netpacks
{

class EntitiesChangedProxy : public SharedWrapper<EntitiesChanged, EntitiesChangedProxy>
{
public:
	using Wrapper = SharedWrapper<EntitiesChanged, EntitiesChangedProxy>;

	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

	static int update(lua_State * L);
};

}
}
}

VCMI_LIB_NAMESPACE_END
