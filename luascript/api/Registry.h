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

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

class Registar
{
public:
	virtual ~Registar() = default;

	virtual void pushMetatable(lua_State * L) const = 0;
};

class Registry final : public boost::noncopyable
{
	using RegistryData = std::map<std::string, std::shared_ptr<Registar>>;

public:
	/// Returns global unique instance of type registry
	static const Registry * get();

	/// Returns type information for public type with provided registered name
	const Registar * find(const std::string & name) const;

	/// Returns all registered types including internal types
	const RegistryData & getAllTypes() const
	{
		return privateData;
	}

	/// Returns unique type name for a provided type
	/// TODO: consider offering demangled names instead, or those provided as part of registration
	template<typename T>
	const char * getTypeName() const
	{
		return typeid(T).name();
	}
private:
	RegistryData privateData;
	RegistryData publicData;

	Registry();

	void addPublic(const std::string & name, const std::shared_ptr<Registar> & item);
	void addPrivate(const std::string & name, const std::shared_ptr<Registar> & item);

	template<typename T>
	void registerPrivate(const std::string & name)
	{
		auto r = std::make_shared<T>();
		addPrivate(name, r);
	}
};

}

VCMI_LIB_NAMESPACE_END
