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

namespace spells
{
namespace effects
{

std::unique_ptr<Registry> Instance;

namespace detail
{
class RegistryImpl : public Registry
{
	using DataPtr = std::shared_ptr<IEffectFactory>;
public:
	RegistryImpl() = default;
	~RegistryImpl() = default;

	const IEffectFactory * find(const std::string & name) const override
	{
		auto iter = data.find(name);
		if(iter == data.end())
			return nullptr;
		else
			return iter->second.get();
	}

	void add(const std::string & name, IEffectFactory * item) override
	{
		data[name].reset(item);
	}

private:
	std::map<std::string, DataPtr> data;
};

}

Registry::Registry() = default;

Registry::~Registry() = default;

Registry * Registry::get()
{
	if(!Instance)
		Instance = make_unique<detail::RegistryImpl>();
	return Instance.get();
}

}
}
