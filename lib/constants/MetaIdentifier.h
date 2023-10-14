/*
 * MetaIdentifier.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IdentifierBase.h"

VCMI_LIB_NAMESPACE_BEGIN

/// This class represents field that may contain value of multiple different identifer types
template<typename... Types>
class DLL_LINKAGE MetaIdentifier
{
	std::variant<Types...> value;
public:

	MetaIdentifier()
	{}

	template<typename IdentifierType>
	MetaIdentifier(const IdentifierType & identifier)
		: value(identifier)
	{}

	int32_t getNum() const
	{
		std::optional<int32_t> result;

		std::visit([&result] (const auto& v) { result = v.getNum(); }, value);

		assert(result.has_value());
		return result.value_or(-1);
	}

	std::string toString() const
	{
		std::optional<std::string> result;

		std::visit([&result] (const auto& v) { result = v.encode(v.getNum()); }, value);

		assert(result.has_value());
		return result.value_or("");
	}

	template<typename IdentifierType>
	IdentifierType as() const
	{
		auto * result = std::get_if<IdentifierType>(&value);
		assert(result);

		if (result)
			return *result;
		else
			return IdentifierType();
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & value;
	}

	bool operator == (const MetaIdentifier & other) const
	{
		return value == other.value;
	}
	bool operator != (const MetaIdentifier & other) const
	{
		return value != other.value;
	}
	bool operator < (const MetaIdentifier & other) const
	{
		return value < other.value;
	}
};

VCMI_LIB_NAMESPACE_END
