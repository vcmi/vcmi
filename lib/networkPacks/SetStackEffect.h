/*
 * SetStackEffect.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "NetPacksBase.h"

#include "../bonuses/Bonus.h"

VCMI_LIB_NAMESPACE_BEGIN

class IBattleState;

struct DLL_LINKAGE SetStackEffect : public CPackForClient
{
	BattleID battleID = BattleID::NONE;
	std::vector<std::pair<ui32, std::vector<Bonus>>> toAdd;
	std::vector<std::pair<ui32, std::vector<Bonus>>> toUpdate;
	std::vector<std::pair<ui32, std::vector<Bonus>>> toRemove;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & toAdd;
		h & toUpdate;
		h & toRemove;
		assert(battleID != BattleID::NONE);
	}
};

VCMI_LIB_NAMESPACE_END
