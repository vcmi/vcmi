/*
 * PlayerGotTurn.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/events/PlayerGotTurn.h>

#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace events
{

class DLL_LINKAGE CPlayerGotTurn : public PlayerGotTurn
{
public:
	CPlayerGotTurn();

	bool isEnabled() const override;

	PlayerColor getPlayer() const override;
	void setPlayer(const PlayerColor & value) override;

	int32_t getPlayerIndex() const override;
	void setPlayerIndex(int32_t value) override;
private:
	PlayerColor player;
};

}

VCMI_LIB_NAMESPACE_END
