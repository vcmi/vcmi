/*
 * ViewSpellInt.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

 #pragma once

 #include "../int3.h"
 #include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

 class CGObjectInstance;

 struct DLL_LINKAGE ObjectPosInfo
 {
 	int3 pos;
 	Obj id;
 	si32 subId;
 	PlayerColor owner;
 	ObjectPosInfo();
 	ObjectPosInfo(const CGObjectInstance * obj);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & pos;
		h & id;
		h & subId;
		h & owner;
	}
 };


VCMI_LIB_NAMESPACE_END
