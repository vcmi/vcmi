
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

struct IPointerCaster
{
	/// takes From*, returns To*
	virtual boost::any castRawPtr(const boost::any &ptr) const = 0;
	/// takes std::shared_ptr<From>, performs dynamic cast, returns std::shared_ptr<To>
	virtual boost::any castSharedPtr(const boost::any &ptr) const = 0;
	/// takes std::weak_ptr<From>, performs dynamic cast, returns std::weak_ptr<To>. The object under poitner must live.
	virtual boost::any castWeakPtr(const boost::any &ptr) const = 0;
	// takes std::unique_ptr<From>, performs dynamic cast, returns std::unique_ptr<To>
	//virtual boost::any castUniquePtr(const boost::any &ptr) const = 0;
};

template <typename From, typename To>
struct PointerCaster : IPointerCaster
{
	virtual boost::any castRawPtr(const boost::any &ptr) const override // takes void* pointing to From object, performs dynamic cast, returns void* pointing to To object
	{
		From * from = (From*)boost::any_cast<void*>(ptr);
		To * ret = static_cast<To*>(from);
		return (void*)ret;
	}

	/// Helper function performing casts between smart pointers
	template<typename SmartPt>
	boost::any castSmartPtr(const boost::any &ptr) const
	{
		try
		{
			auto from = boost::any_cast<SmartPt>(ptr);
			auto ret = std::static_pointer_cast<To>(from);
			return ret;
		}
		catch(std::exception &e)
		{
			THROW_FORMAT("Failed cast %s -> %s. Given argument was %s. Error message: %s", typeid(From).name() % typeid(To).name() % ptr.type().name() % e.what());
		}
	}

	virtual boost::any castSharedPtr(const boost::any &ptr) const override
	{
		return castSmartPtr<std::shared_ptr<From>>(ptr);
	}
	virtual boost::any castWeakPtr(const boost::any &ptr) const override
	{
		auto from = boost::any_cast<std::weak_ptr<From>>(ptr);
		return castSmartPtr<std::shared_ptr<From>>(from.lock());
	}
};

struct ITypeManipulator
{
    virtual bool isExpiredWeakPtr(const boost::any & ptr) const = 0;
};

template <typename T>
struct TypeManipulator : ITypeManipulator
{
	typedef typename std::remove_const<T>::type NonConstT;

	bool isExpiredWeakPtr(const boost::any & ptr) const override
	{
		auto weak = boost::any_cast<std::weak_ptr<NonConstT>>(ptr);
		return weak.expired();
	}
};

/// Class that implements basic reflection-like mechanisms
/// For every type registered via registerType() generates inheritance tree
/// Rarely used directly - usually used as part of CApplier
class DLL_LINKAGE CTypeList: public boost::noncopyable
{
	struct TypeDescriptor;
	typedef std::shared_ptr<TypeDescriptor> TypeInfoPtr;
	typedef std::weak_ptr<TypeDescriptor> WeakTypeInfoPtr;
	struct TypeDescriptor
	{
		ui16 typeID;
		const char *name;
		std::vector<WeakTypeInfoPtr> children, parents;
		std::shared_ptr<ITypeManipulator> manipulator;
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

	template<boost::any(IPointerCaster::*CastingFunction)(const boost::any &) const>
	boost::any castHelper(boost::any inputPtr, const std::type_info *fromArg, const std::type_info *toArg) const
	{
		TSharedLock lock(mx);
		auto typesSequence = castSequence(fromArg, toArg);

		boost::any ptr = inputPtr;
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
	CTypeList &operator=(CTypeList &)
	{
		// As above.
		assert(0);
		return *this;
	}

	TypeInfoPtr getTypeDescriptor(const std::type_info *type, bool throws = true) const; //if not throws, failure returns nullptr
	TypeInfoPtr registerNewType(const std::type_info *type);

	template <typename T>
	TypeInfoPtr registerTypeT(const std::type_info * type)
	{
		if(auto typeDescr = getTypeDescriptor(type, false))
			return typeDescr;  //type found, return ptr to structure

		TypeInfoPtr ret = registerNewType(type);
		ret->manipulator = std::make_shared<TypeManipulator<T>>();
		return ret;
	}

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
		auto bti = registerTypeT<Base>(bt);
		auto dti = registerTypeT<Derived>(dt); //obtain our TypeDescriptor

		// register the relation between classes
		bti->children.push_back(dti);
		dti->parents.push_back(bti);
		casters[std::make_pair(bti, dti)] = make_unique<const PointerCaster<Base, Derived>>();
		casters[std::make_pair(dti, bti)] = make_unique<const PointerCaster<Derived, Base>>();
	}

	ui16 getTypeID(const std::type_info *type, bool throws = false) const;

	template <typename T>
	ui16 getTypeID(const T * t = nullptr, bool throws = false) const
	{
		return getTypeID(getTypeInfo(t), throws);
	}

	///Returns manipulator for @type if it is registered, creates new one based on template argument otherwise
	template <typename T>
	std::shared_ptr<ITypeManipulator> getTypeManipulator(const std::type_info * type) const
	{
		if(auto typeDescr = getTypeDescriptor(type, false))
			return typeDescr->manipulator;  //type found, return ptr to structure
		else
			return std::make_shared<TypeManipulator<T>>();
	}

	template<typename TInput>
	void * castToMostDerived(const TInput * inputPtr) const
	{
		auto &baseType = typeid(typename std::remove_cv<TInput>::type);
		auto derivedType = getTypeInfo(inputPtr);

		if (strcmp(baseType.name(), derivedType->name()) == 0)
		{
			return const_cast<void*>(reinterpret_cast<const void*>(inputPtr));
		}

		return boost::any_cast<void*>(castHelper<&IPointerCaster::castRawPtr>(
			const_cast<void*>(reinterpret_cast<const void*>(inputPtr)), &baseType,
			derivedType));
	}

	template<typename TInput>
	boost::any castSharedToMostDerived(const std::shared_ptr<TInput> inputPtr) const
	{
		auto &baseType = typeid(typename std::remove_cv<TInput>::type);
		auto derivedType = getTypeInfo(inputPtr.get());

		if (!strcmp(baseType.name(), derivedType->name()))
			return inputPtr;

		return castHelper<&IPointerCaster::castSharedPtr>(inputPtr, &baseType, derivedType);
	}

	template<typename TInput>
	boost::any castWeakToMostDerived(const std::weak_ptr<TInput> inputPtr) const
	{
		auto &baseType = typeid(typename std::remove_cv<TInput>::type);
		auto derivedType = getTypeInfo(inputPtr.lock().get());

		if (!strcmp(baseType.name(), derivedType->name()))
			return inputPtr;

		return castHelper<&IPointerCaster::castWeakPtr>(inputPtr, &baseType, derivedType);
	}

	void * castRaw(void *inputPtr, const std::type_info *from, const std::type_info *to) const
	{
		return boost::any_cast<void*>(castHelper<&IPointerCaster::castRawPtr>(inputPtr, from, to));
	}

	boost::any castShared(boost::any inputPtr, const std::type_info *from, const std::type_info *to) const
	{
		return castHelper<&IPointerCaster::castSharedPtr>(inputPtr, from, to);
	}

	boost::any castWeak(boost::any inputPtr, const std::type_info *from, const std::type_info *to) const
	{
		return castHelper<&IPointerCaster::castWeakPtr>(inputPtr, from, to);
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
		assert(apps.count(ID));
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
