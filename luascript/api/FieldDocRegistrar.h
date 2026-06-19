/*
 * FieldDocRegistrar.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "SignatureOf.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

template<typename E> struct EnumGroup;

/// Field-shaped counterpart to DocRegistrar.
/// Doubles as a serializer for ApiSerializable types: its templated call operator matches
/// the lambda shape that `T::serializeScript(s)` invokes (`s(name, value, description)`).
/// Plain fields land in `entries`; EnumGroup<E> values land in `enumGroups` so the doc
/// emitter can render them as `---@enum` blocks instead of opaque table fields.
class FieldDocRegistrar final
{
public:
	struct Entry
	{
		std::string name;
		std::string type;
		std::string description;
	};

	struct EnumKey
	{
		std::string key;
		long long   value;
		std::string description;
	};

	struct EnumGroupEntry
	{
		std::string name;
		std::string description;
		std::vector<EnumKey> keys;
	};

	template<typename Field>
	void operator()(const std::string & name, const Field &, std::string_view description)
	{
		entries.push_back({ name, luaTypeName<Field>(), std::string(description) });
	}

	template<typename E>
	void operator()(const std::string & name, const EnumGroup<E> & group, std::string_view description)
	{
		EnumGroupEntry out{ name, std::string(description), {} };
		for(const auto & item : group.items)
			out.keys.push_back({ item.key, static_cast<long long>(item.value), item.description });
		enumGroups.push_back(std::move(out));
	}

	const std::vector<Entry> & get() const { return entries; }
	const std::vector<EnumGroupEntry> & getEnumGroups() const { return enumGroups; }

private:
	std::vector<Entry> entries;
	std::vector<EnumGroupEntry> enumGroups;
};

}

VCMI_LIB_NAMESPACE_END
