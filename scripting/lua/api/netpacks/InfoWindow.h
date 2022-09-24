/*
 * api/netpacks/InfoWindow.h, part of VCMI engine
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

class InfoWindowProxy : public SharedWrapper<InfoWindow, InfoWindowProxy>
{
public:
	using Wrapper = SharedWrapper<InfoWindow, InfoWindowProxy>;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

	static int addReplacement(lua_State * L);
	static int addText(lua_State * L);
	static int setPlayer(lua_State * L);
};

}
}
}

VCMI_LIB_NAMESPACE_END
