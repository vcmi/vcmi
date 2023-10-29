/*
 * Component.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class CStackBasicDescriptor;

struct Component
{
	enum class EComponentType : uint8_t
	{
		PRIM_SKILL,
		SEC_SKILL,
		RESOURCE,
		CREATURE,
		ARTIFACT,
		EXPERIENCE,
		SPELL,
		MORALE,
		LUCK,
		BUILDING,
		HERO_PORTRAIT,
		FLAG,
		INVALID //should be last
	};
	EComponentType id = EComponentType::INVALID;
	ui16 subtype = 0; //id==EXPPERIENCE subtype==0 means exp points and subtype==1 levels
	si32 val = 0; // + give; - take
	si16 when = 0; // 0 - now; +x - within x days; -x - per x days

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
		h & subtype;
		h & val;
		h & when;
	}
	Component() = default;
	DLL_LINKAGE explicit Component(const CStackBasicDescriptor &stack);
	Component(Component::EComponentType Type, ui16 Subtype, si32 Val, si16 When)
		:id(Type),subtype(Subtype),val(Val),when(When)
	{
	}
};

VCMI_LIB_NAMESPACE_END
