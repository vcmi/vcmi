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

VCMI_LIB_NAMESPACE_BEGIN

struct Bonus;
class FlaggableInstanceConstructor;

class DLL_LINKAGE FlaggableMapObject : public CGObjectInstance, public IOwnableObject
{
	std::shared_ptr<FlaggableInstanceConstructor> getFlaggableHandler() const;

	void giveBonusTo(const PlayerColor & player, bool onInit = false) const;
	void takeBonusFrom(const PlayerColor & player) const;

public:
	using CGObjectInstance::CGObjectInstance;

	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj(vstd::RNG & rand) override;

	const IOwnableObject * asOwnable() const final;
	ResourceSet dailyIncome() const override;
	std::vector<CreatureID> providedCreatures() const override;

protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

VCMI_LIB_NAMESPACE_END
