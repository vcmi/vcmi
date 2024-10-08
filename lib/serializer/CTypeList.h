/*
 * CTypeList.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

/// Class that implements basic reflection-like mechanisms
/// For every type registered via registerType() generates inheritance tree
/// Rarely used directly - usually used as part of CApplier
class CTypeList
{
	std::map<std::string, uint16_t> typeInfos;

	DLL_LINKAGE CTypeList();

	template <typename T>
	const std::type_info & getTypeInfo(const T * t = nullptr) const
	{
		if(t)
			return typeid(*t);
		else
			return typeid(T);
	}

public:
	static CTypeList & getInstance()
	{
		static CTypeList registry;
		return registry;
	}

	template<typename T>
	void registerType(uint16_t index)
	{
		const std::type_info & typeInfo = typeid(T);

		if (typeInfos.count(typeInfo.name()) != 0)
			return;

		typeInfos[typeInfo.name()] = index;
	}

	template<typename T>
	uint16_t getTypeID(T * typePtr)
	{
		static_assert(!std::is_pointer_v<T>, "CTypeList does not supports pointers!");
		static_assert(!std::is_reference_v<T>, "CTypeList does not supports references!");

		const std::type_info & typeInfo = getTypeInfo(typePtr);

		if (typeInfos.count(typeInfo.name()) == 0)
			return 0;

		return typeInfos.at(typeInfo.name());
	}
};

VCMI_LIB_NAMESPACE_END
