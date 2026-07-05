/*
 * util.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

namespace MMAI::Schema::V15
{

/*
 * Compile-time checks for misconfigured `HEX_ENCODING`/`STACK_ENCODING`.
 * The index of the uninitialized element is returned.
 */
template<typename T>
constexpr int UninitializedEncodingAttributes(T elems)
{
	// E5S / E5H:
	using E5Type = typename T::value_type;

	// Stack Attribute / HexAttribute:
	using EnumType = std::tuple_element_t<0, E5Type>;

	for(int i = 0; i < EI(EnumType::_count); i++)
	{
		if(elems.at(i) == E5Type{})
			return EI(EnumType::_count) - i;
	}

	return 0;
}

/*
 * Compile-time checks for elements in `HEX_ENCODING` and `STACK_ENCODING`
 * which are out-of-order compared to the `Attribute` enum values.
 * The index at which the order is violated is returned.
 */
template<typename T>
constexpr int DisarrayedEncodingAttributeIndex(T elems)
{
	// E5S / E5H:
	using E5Type = typename T::value_type;

	// Stack Attribute / HexAttribute:
	using EnumType = std::tuple_element_t<0, E5Type>;

	for(int i = 0; i < EI(EnumType::_count); i++)
	{
		if(std::get<0>(elems.at(i)) != static_cast<EnumType>(i))
			return i;
	}

	return -1;
}

/*
 * Compile-time calculation for the encoded size of hexes and stacks
 */
template<typename T>
constexpr int EncodedSize(T elems)
{
	using E5Type = typename T::value_type;
	using EnumType = std::tuple_element_t<0, E5Type>;
	int ret = 0;
	for(int i = 0; i < EI(EnumType::_count); i++)
	{
		ret += std::get<2>(elems.at(i));
	}
	return ret;
}

}
