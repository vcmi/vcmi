/*
 * ObjectVisitEnded.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/events/ObjectVisitEnded.h>

#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace events
{

class DLL_LINKAGE CObjectVisitEnded : public ObjectVisitEnded
{
public:
	CObjectVisitEnded(const PlayerColor & player_, const ObjectInstanceID & heroId_);

	PlayerColor getPlayer() const override;
	ObjectInstanceID getHero() const override;
	bool isEnabled() const override;
private:
	PlayerColor player;
	ObjectInstanceID heroId;
};

}

VCMI_LIB_NAMESPACE_END
