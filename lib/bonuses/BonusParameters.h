/*
 * BonusParameters.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../serializer/Serializeable.h"

#include "Bonus.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE BonusParameters final : public Serializeable
{
public:
	using storage_type = std::variant<int32_t, CreatureID, SpellID, std::vector<int32_t>>;

	BonusParameters() = default;

	template<typename DataType>
	BonusParameters(const DataType & value)
		:data_(value)
	{}

	std::string toString() const;
	JsonNode toJsonNode() const;

	CreatureID toCreature() const
	{
		return toCustom<CreatureID>();
	}

	SpellID toSpell() const
	{
		return toCustom<SpellID>();
	}

	int32_t toNumber() const
	{
		return toCustom<int32_t>();
	}

	const std::vector<int32_t> & toVector() const
	{
		return toCustom<std::vector<int32_t>>();
	}

	template<typename CustomType>
	const CustomType & toCustom() const
	{
		auto * result = std::get_if<CustomType>(&data_);
		if (result)
			return *result;
		throw std::runtime_error("Invalid addInfo type access!");
	}

	template <class H>
	void serialize(H& h)
	{
		h & data_;
	}

private:
	storage_type data_;
};

VCMI_LIB_NAMESPACE_END
