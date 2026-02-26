/*
 * Registry.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <typeinfo>
#include <typeindex>

#define VCMI_REGISTER_SCRIPT_API(Type, Name) \
namespace\
{\
RegisterAPI<Type> _register ## Type (Name);\
}\
\

#define VCMI_REGISTER_CORE_SCRIPT_API(Type, Name) \
namespace\
{\
RegisterCoreAPI<Type> _register ## Type (Name);\
}\
\

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
class TypeRegistry;

class Registar
{
public:
	virtual ~Registar() = default;

	virtual void pushMetatable(lua_State * L) const = 0;
};

class Registry : public boost::noncopyable
{
public:
	using RegistryData = std::map<std::string, std::shared_ptr<Registar>>;
	static Registry * get();

	const Registar * find(const std::string & name) const;
	void add(const std::string & name, std::shared_ptr<Registar> item);
	void addCore(const std::string & name, std::shared_ptr<Registar> item);

	const RegistryData & getCoreData() const
	{
		return coreData;
	}
private:
	RegistryData data;
	RegistryData coreData;

	Registry();
};

template<typename T>
class RegisterAPI
{
public:
	RegisterAPI(const std::string & name)
	{
		auto r = std::make_shared<T>();
		Registry::get()->add(name, r);
	}
};

template<typename T>
class RegisterCoreAPI
{
public:
	RegisterCoreAPI(const std::string & name)
	{
		auto r = std::make_shared<T>();
		Registry::get()->addCore(name, r);
	}
};

class TypeRegistry : public boost::noncopyable
{
public:
	template<typename T>
	const char * getKey()
	{
		return getKeyForType(typeid(T));
	}

	static TypeRegistry * get();
private:
	size_t nextIndex;

	std::mutex mutex;

	std::map<std::type_index, std::string> keys;

	TypeRegistry();

	const char * getKeyForType(const std::type_info & type);
};

}
}

VCMI_LIB_NAMESPACE_END
