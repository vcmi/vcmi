/*
 * VariantIdentifier.h, part of VCMI engine
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

/// This class represents field that may contain value of multiple different identifier types
template<typename... Types>
class VariantIdentifier
{
	using Type = std::variant<Types...>;
	Type value;

public:
	VariantIdentifier()
	{}

	template<typename IdentifierType>
	VariantIdentifier(const IdentifierType & identifier)
		: value(identifier)
	{}

	int32_t getNum() const
	{
		int32_t result;

		std::visit([&result] (const auto& v) { result = v.getNum(); }, value);

		return result;
	}

	std::string toString() const
	{
		std::string result;

		std::visit([&result] (const auto& v) { result = v.encode(v.getNum()); }, value);

		return result;
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

	template <typename Handler> void serialize(Handler &h)
	{
		h & value;
	}

	bool operator == (const VariantIdentifier & other) const
	{
		return value == other.value;
	}
	bool operator != (const VariantIdentifier & other) const
	{
		return value != other.value;
	}
	bool operator < (const VariantIdentifier & other) const
	{
		return value < other.value;
	}
};

VCMI_LIB_NAMESPACE_END
