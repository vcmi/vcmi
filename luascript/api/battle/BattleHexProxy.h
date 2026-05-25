/*
 * BattleHexProxy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>
#include "../../../lib/battle/BattleHex.h"
#include "../../LuaWrapper.h"

VCMI_LIB_NAMESPACE_BEGIN

	namespace scripting
{
	namespace api
	{
		namespace battle
		{

			class BattleHexProxy : public CopyableWrapper<const BattleHex, BattleHexProxy>
			{
			public:
				using Wrapper = CopyableWrapper<const BattleHex, BattleHexProxy>;
				static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
			};

		}
	}
}

VCMI_LIB_NAMESPACE_END
