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

#include "api/Registry.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{

Registry::Registry() = default;

Registry * Registry::get()
{
	static std::unique_ptr<Registry> Instance = std::unique_ptr<Registry>(new Registry());
	return Instance.get();
}

void Registry::add(const std::string & name, std::shared_ptr<Registar> item)
{
	data[name] = item;
}

void Registry::addCore(const std::string & name, std::shared_ptr<Registar> item)
{
	coreData[name] = item;
}

const Registar * Registry::find(const std::string & name) const
{
	auto iter = data.find(name);
	if(iter == data.end())
		return nullptr;
	else
		return iter->second.get();
}

TypeRegistry::TypeRegistry()
	: nextIndex(0)
{

}

TypeRegistry * TypeRegistry::get()
{
	static std::unique_ptr<TypeRegistry> Instance = std::unique_ptr<TypeRegistry>(new TypeRegistry());
	return Instance.get();
}

const char * TypeRegistry::getKeyForType(const std::type_info & type)
{
	//std::type_index is unique and stable (because all bindings are in vcmiLua shared lib), but there is no way to convert it to Lua value
	//there is no guarantee that name is unique, but it is at least somewhat human readable, so we append unique number to name
	//TODO: name demangle

	std::type_index typeIndex(type);

	boost::unique_lock<boost::mutex> lock(mutex);

	auto iter = keys.find(typeIndex);

	if(iter == std::end(keys))
	{
		std::string newKey = type.name();
		newKey += "_";
		newKey += std::to_string(nextIndex++);

		keys[typeIndex] = std::move(newKey);

		return keys[typeIndex].c_str();
	}
	else
	{
		return iter->second.c_str();
	}
}


}
}

VCMI_LIB_NAMESPACE_END
