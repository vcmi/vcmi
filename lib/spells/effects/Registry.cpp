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
	RegistryImpl() = default;
	~RegistryImpl() override = default;

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
