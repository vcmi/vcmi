/*
 * element.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "AI/MMAI/common.h" // IWYU pragma: keep

#include "BAI/v15/encoder_v15.h"
#include "schema/v15/graph.h"

namespace MMAI::BAI::V15::Graph
{
namespace S15 = Schema::V15;

template<typename Interface, typename EncTraits>
class Element : public Interface
{
public:
	using encoding_traits = EncTraits;
	using Attribute = typename EncTraits::A;

	S15::Graph::ElementType getType() const override
	{
		return EncTraits::element_type;
	}
	std::vector<int> rawAttributes() const override
	{
		return std::vector(attrs.begin(), attrs.end());
	}
	int encode(std::span<float> out) const override
	{
		return Encoder::Encode<EncTraits>(attrs, out);
	}

	Element()
	{
		attrs.fill(0);
	}

	int attr(Attribute a) const
	{
		assert(guardflags.test(EU(a)));
		return attrs.at(EU(a));
	}

	void setattr(Attribute a, int value)
	{
		assert(!guardflags.test(EU(a)));
		guardflags.set(EU(a));
		attrs.at(EU(a)) = value;
	}

	std::string name() const override
	{
		return std::string(EncTraits::name);
	}

	void verify() const
	{
		if(!guardflags.all())
			throw std::runtime_error(std::string(EncTraits::name) + ": verify: " + guardflags.to_string());
	}

	std::array<int, EncTraits::attr_count> attrs = {};
	std::bitset<EncTraits::attr_count> guardflags;
};
}
