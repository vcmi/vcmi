/*
 * Registry.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Registry.h"

#include "Catapult.h"
#include "Clone.h"
#include "Damage.h"
#include "DemonSummon.h"
#include "Dispel.h"
#include "Effect.h"
#include "Effects.h"
#include "Heal.h"
#include "Moat.h"
#include "LocationEffect.h"
#include "Obstacle.h"
#include "RemoveObstacle.h"
#include "Sacrifice.h"
#include "Summon.h"
#include "Teleport.h"
#include "Timed.h"
#include "UnitEffect.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

namespace detail
{
class RegistryImpl : public Registry
{
public:
	RegistryImpl()
	{
		add("core:catapult", std::make_shared<EffectFactory<Catapult>>());
		add("core:clone", std::make_shared<EffectFactory<Clone>>());
		add("core:damage", std::make_shared<EffectFactory<Damage>>());
		add("core:demonSummon", std::make_shared<EffectFactory<DemonSummon>>());
		add("core:dispel", std::make_shared<EffectFactory<Dispel>>());
		add("core:heal", std::make_shared<EffectFactory<Heal>>());
		add("core:moat", std::make_shared<EffectFactory<Moat>>());
		add("core:obstacle", std::make_shared<EffectFactory<Obstacle>>());
		add("core:removeObstacle", std::make_shared<EffectFactory<RemoveObstacle>>());
		add("core:sacrifice", std::make_shared<EffectFactory<Sacrifice>>());
		add("core:summon", std::make_shared<EffectFactory<Summon>>());
		add("core:teleport", std::make_shared<EffectFactory<Teleport>>());
		add("core:timed", std::make_shared<EffectFactory<Timed>>());
	}

	const IEffectFactory * find(const std::string & name) const override
	{
		auto iter = data.find(name);
		if(iter == data.end())
			return nullptr;
		else
			return iter->second.get();
	}

	void add(const std::string & name, FactoryPtr item) override
	{
		data[name] = item;
	}

private:
	std::map<std::string, FactoryPtr> data;
};

}

Registry * GlobalRegistry::get()
{
	static std::unique_ptr<Registry> Instance = std::make_unique<detail::RegistryImpl>();
	return Instance.get();
}

}
}

VCMI_LIB_NAMESPACE_END
