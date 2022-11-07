/*
 * ObjectVisitStarted.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/events/ObjectVisitStarted.h>

#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace events
{

class DLL_LINKAGE CObjectVisitStarted : public ObjectVisitStarted
{
public:
	CObjectVisitStarted(const PlayerColor & player_, const ObjectInstanceID & heroId_, const ObjectInstanceID & objId_);

	PlayerColor getPlayer() const override;
	ObjectInstanceID getHero() const override;
	ObjectInstanceID getObject() const override;

	bool isEnabled() const override;
	void setEnabled(bool enable) override;
private:
	PlayerColor player;
	ObjectInstanceID heroId;
	ObjectInstanceID objId;
	bool enabled;
};

}

VCMI_LIB_NAMESPACE_END
