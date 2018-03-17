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
	static const std::vector<typename Wrapper::RegType> REGISTER;

	static int addReplacement(lua_State * L, std::shared_ptr<InfoWindow> object);
	static int addText(lua_State * L, std::shared_ptr<InfoWindow> object);
};

}
}
}
