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

	template<typename T, typename U>
	void registerType()
	{
		registerType<T>();
		registerType<U>();
	}

	template<typename T>
	void registerType()
	{
		const std::type_info & typeInfo = typeid(T);

		if (typeInfos.count(typeInfo.name()) != 0)
			return;

		typeInfos[typeInfo.name()] = typeInfos.size() + 1;
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

/// Wrapper over CTypeList. Allows execution of templated class T for any type
/// that was resgistered for this applier
template<typename T>
class CApplier : boost::noncopyable
{
	std::map<int32_t, std::unique_ptr<T>> apps;

	template<typename RegisteredType>
	void addApplier(ui16 ID)
	{
		if(!apps.count(ID))
		{
			RegisteredType * rtype = nullptr;
			apps[ID].reset(T::getApplier(rtype));
		}
	}

public:
	T * getApplier(ui16 ID)
	{
		if(!apps.count(ID))
			throw std::runtime_error("No applier found.");
		return apps[ID].get();
	}

	template<typename Base, typename Derived>
	void registerType(const Base * b = nullptr, const Derived * d = nullptr)
	{
		addApplier<Base>(CTypeList::getInstance().getTypeID<Base>(nullptr));
		addApplier<Derived>(CTypeList::getInstance().getTypeID<Derived>(nullptr));
	}
};

VCMI_LIB_NAMESPACE_END
