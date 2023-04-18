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

#include "CSerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

struct IPointerCaster
{
	virtual std::any castRawPtr(const std::any &ptr) const = 0; // takes From*, returns To*
	virtual std::any castSharedPtr(const std::any &ptr) const = 0; // takes std::shared_ptr<From>, performs dynamic cast, returns std::shared_ptr<To>
	virtual std::any castWeakPtr(const std::any &ptr) const = 0; // takes std::weak_ptr<From>, performs dynamic cast, returns std::weak_ptr<To>. The object under poitner must live.
	//virtual std::any castUniquePtr(const std::any &ptr) const = 0; // takes std::unique_ptr<From>, performs dynamic cast, returns std::unique_ptr<To>
	virtual ~IPointerCaster() = default;
};

template <typename From, typename To>
struct PointerCaster : IPointerCaster
{
	virtual std::any castRawPtr(const std::any &ptr) const override // takes void* pointing to From object, performs dynamic cast, returns void* pointing to To object
	{
		From * from = (From*)std::any_cast<void*>(ptr);
		To * ret = static_cast<To*>(from);
		return (void*)ret;
	}

	// Helper function performing casts between smart pointers
	template<typename SmartPt>
	std::any castSmartPtr(const std::any &ptr) const
	{
		try
		{
			auto from = std::any_cast<SmartPt>(ptr);
			auto ret = std::static_pointer_cast<To>(from);
			return ret;
		}
		catch(std::exception &e)
		{
			THROW_FORMAT("Failed cast %s -> %s. Given argument was %s. Error message: %s", typeid(From).name() % typeid(To).name() % ptr.type().name() % e.what());
		}
	}

	virtual std::any castSharedPtr(const std::any &ptr) const override
	{
		return castSmartPtr<std::shared_ptr<From>>(ptr);
	}
	virtual std::any castWeakPtr(const std::any &ptr) const override
	{
		auto from = std::any_cast<std::weak_ptr<From>>(ptr);
		return castSmartPtr<std::shared_ptr<From>>(from.lock());
	}
};

/// Class that implements basic reflection-like mechanisms
/// For every type registered via registerType() generates inheritance tree
/// Rarely used directly - usually used as part of CApplier
class DLL_LINKAGE CTypeList: public boost::noncopyable
{
//public:
	struct TypeDescriptor;
	typedef std::shared_ptr<TypeDescriptor> TypeInfoPtr;
	typedef std::weak_ptr<TypeDescriptor> WeakTypeInfoPtr;
	struct TypeDescriptor
	{
		ui16 typeID;
		const char *name;
		std::vector<WeakTypeInfoPtr> children, parents;
	};
	typedef boost::shared_mutex TMutex;
	typedef boost::unique_lock<TMutex> TUniqueLock;
	typedef boost::shared_lock<TMutex> TSharedLock;
private:
	mutable TMutex mx;

	std::map<const std::type_info *, TypeInfoPtr, TypeComparer> typeInfos;
	std::map<std::pair<TypeInfoPtr, TypeInfoPtr>, std::unique_ptr<const IPointerCaster>> casters; //for each pair <Base, Der> we provide a caster (each registered relations creates a single entry here)

	/// Returns sequence of types starting from "from" and ending on "to". Every next type is derived from the previous.
	/// Throws if there is no link registered.
	std::vector<TypeInfoPtr> castSequence(TypeInfoPtr from, TypeInfoPtr to) const;
	std::vector<TypeInfoPtr> castSequence(const std::type_info *from, const std::type_info *to) const;

	template<std::any(IPointerCaster::*CastingFunction)(const std::any &) const>
	std::any castHelper(std::any inputPtr, const std::type_info *fromArg, const std::type_info *toArg) const
	{
		TSharedLock lock(mx);
		auto typesSequence = castSequence(fromArg, toArg);

		std::any ptr = inputPtr;
		for(int i = 0; i < static_cast<int>(typesSequence.size()) - 1; i++)
		{
			auto &from = typesSequence[i];
			auto &to = typesSequence[i + 1];
			auto castingPair = std::make_pair(from, to);
			if(!casters.count(castingPair))
				THROW_FORMAT("Cannot find caster for conversion %s -> %s which is needed to cast %s -> %s", from->name % to->name % fromArg->name() % toArg->name());

			auto &caster = casters.at(castingPair);
			ptr = (*caster.*CastingFunction)(ptr); //Why does unique_ptr not have operator->* ..?
		}

		return ptr;
	}
	CTypeList & operator=(CTypeList &) = delete;

	TypeInfoPtr getTypeDescriptor(const std::type_info *type, bool throws = true) const; //if not throws, failure returns nullptr
	TypeInfoPtr registerType(const std::type_info *type);

public:

	CTypeList();

	template <typename Base, typename Derived>
	void registerType(const Base * b = nullptr, const Derived * d = nullptr)
	{
		TUniqueLock lock(mx);
		static_assert(std::is_base_of<Base, Derived>::value, "First registerType template parameter needs to ba a base class of the second one.");
		static_assert(std::has_virtual_destructor<Base>::value, "Base class needs to have a virtual destructor.");
		static_assert(!std::is_same<Base, Derived>::value, "Parameters of registerTypes should be two different types.");
		auto bt = getTypeInfo(b);
		auto dt = getTypeInfo(d); //obtain std::type_info
		auto bti = registerType(bt);
		auto dti = registerType(dt); //obtain our TypeDescriptor

		// register the relation between classes
		bti->children.push_back(dti);
		dti->parents.push_back(bti);
		casters[std::make_pair(bti, dti)] = std::make_unique<const PointerCaster<Base, Derived>>();
		casters[std::make_pair(dti, bti)] = std::make_unique<const PointerCaster<Derived, Base>>();
	}

	ui16 getTypeID(const std::type_info *type, bool throws = false) const;

	template <typename T>
	ui16 getTypeID(const T * t = nullptr, bool throws = false) const
	{
		return getTypeID(getTypeInfo(t), throws);
	}

	TypeInfoPtr getTypeDescriptor(ui16 typeID) const;

	template<typename TInput>
	void * castToMostDerived(const TInput * inputPtr) const
	{
		auto &baseType = typeid(typename std::remove_cv<TInput>::type);
		auto derivedType = getTypeInfo(inputPtr);

		if (strcmp(baseType.name(), derivedType->name()) == 0)
		{
			return const_cast<void*>(reinterpret_cast<const void*>(inputPtr));
		}

		return std::any_cast<void*>(castHelper<&IPointerCaster::castRawPtr>(
			const_cast<void*>(reinterpret_cast<const void*>(inputPtr)), &baseType,
			derivedType));
	}

	template<typename TInput>
	std::any castSharedToMostDerived(const std::shared_ptr<TInput> inputPtr) const
	{
		auto &baseType = typeid(typename std::remove_cv<TInput>::type);
		auto derivedType = getTypeInfo(inputPtr.get());

		if (!strcmp(baseType.name(), derivedType->name()))
			return inputPtr;

		return castHelper<&IPointerCaster::castSharedPtr>(inputPtr, &baseType, derivedType);
	}

	void * castRaw(void *inputPtr, const std::type_info *from, const std::type_info *to) const
	{
		return std::any_cast<void*>(castHelper<&IPointerCaster::castRawPtr>(inputPtr, from, to));
	}
	std::any castShared(std::any inputPtr, const std::type_info *from, const std::type_info *to) const
	{
		return castHelper<&IPointerCaster::castSharedPtr>(inputPtr, from, to);
	}

	template <typename T> const std::type_info * getTypeInfo(const T * t = nullptr) const
	{
		if(t)
			return &typeid(*t);
		else
			return &typeid(T);
	}
};

extern DLL_LINKAGE CTypeList typeList;

/// Wrapper over CTypeList. Allows execution of templated class T for any type
/// that was resgistered for this applier
template<typename T>
class CApplier : boost::noncopyable
{
	std::map<ui16, std::unique_ptr<T>> apps;

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
		typeList.registerType(b, d);
		addApplier<Base>(typeList.getTypeID(b));
		addApplier<Derived>(typeList.getTypeID(d));
	}
};

VCMI_LIB_NAMESPACE_END
