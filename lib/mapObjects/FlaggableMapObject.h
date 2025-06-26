/*
 * FlaggableMapObject.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CGObjectInstance.h"
#include "IOwnableObject.h"
#include "../bonuses/CBonusSystemNode.h"

VCMI_LIB_NAMESPACE_BEGIN

struct Bonus;
class FlaggableInstanceConstructor;

class DLL_LINKAGE FlaggableMapObject final : public CGObjectInstance, public IOwnableObject, public CBonusSystemNode
{
	std::shared_ptr<FlaggableInstanceConstructor> getFlaggableHandler() const;

	void initBonuses();

public:
	using CGObjectInstance::CGObjectInstance;

	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void initObj(IGameRandomizer & gameRandomizer) override;

	const IOwnableObject * asOwnable() const final;
	ResourceSet dailyIncome() const override;
	std::vector<CreatureID> providedCreatures() const override;

	void attachToBonusSystem(CGameState & gs) override;
	void detachFromBonusSystem(CGameState & gs) override;
	void restoreBonusSystem(CGameState & gs) override;

	PlayerColor getOwner() const override
	{
		return CGObjectInstance::getOwner();
	}

	void serializeJsonOptions(JsonSerializeFormat & handler) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGObjectInstance&>(*this);

		if (h.version >= Handler::Version::FLAGGABLE_BONUS_SYSTEM_NODE)
			h & static_cast<CBonusSystemNode&>(*this);
		else
			initBonuses();
	}
};

VCMI_LIB_NAMESPACE_END
