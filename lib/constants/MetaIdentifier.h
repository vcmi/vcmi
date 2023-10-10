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
class DLL_LINKAGE MetaIdentifier
{
	std::string stringForm;
	int32_t integerForm;

	void onDeserialized();
public:

	static const MetaIdentifier NONE;

	MetaIdentifier();
	MetaIdentifier(const std::string & entityType, const std::string & identifier);
	MetaIdentifier(const std::string & entityType, const std::string & identifier, int32_t value);

	template<typename IdentifierType>
	explicit MetaIdentifier(const IdentifierType & identifier)
		: integerForm(identifier.getNum())
	{
		static_assert(std::is_base_of<IdentifierBase, IdentifierType>::value, "MetaIdentifier can only be constructed from Identifer class");
	}

	int32_t getNum() const;
	std::string toString() const;

	template<typename IdentifierType>
	IdentifierType as() const
	{
		static_assert(std::is_base_of<IdentifierBase, IdentifierType>::value, "MetaIdentifier can only be converted to Identifer class");
		IdentifierType result(integerForm);
		return result;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & integerForm;

		if (!h.saving)
			onDeserialized();
	}

	bool operator == (const MetaIdentifier & other) const;
	bool operator != (const MetaIdentifier & other) const;
	bool operator < (const MetaIdentifier & other) const;
};

VCMI_LIB_NAMESPACE_END
