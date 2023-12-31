/*
 * CSerializer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../ConstTransitivePtr.h"
#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

const ui32 SERIALIZATION_VERSION = 831;
const ui32 MINIMAL_SERIALIZATION_VERSION = 831;
const std::string SAVEGAME_MAGIC = "VCMISVG";

class CHero;
class CGHeroInstance;
class CGObjectInstance;

class CGameState;
class LibClasses;
extern DLL_LINKAGE LibClasses * VLC;

struct TypeComparer
{
	bool operator()(const std::type_info *a, const std::type_info *b) const
	{
	//#ifndef __APPLE__
	//  return a->before(*b);
	//#else
		return strcmp(a->name(), b->name()) < 0;
	//#endif
	}
};

template <typename ObjType, typename IdType>
struct VectorizedObjectInfo
{
	const std::vector<ConstTransitivePtr<ObjType> > *vector;	//pointer to the appropriate vector
	std::function<IdType(const ObjType &)> idRetriever;

	VectorizedObjectInfo(const std::vector< ConstTransitivePtr<ObjType> > *Vector, std::function<IdType(const ObjType &)> IdGetter)
		:vector(Vector), idRetriever(IdGetter)
	{
	}
};

/// Base class for serializers capable of reading or writing data
class DLL_LINKAGE CSerializer
{
	template<typename Numeric, std::enable_if_t<std::is_arithmetic_v<Numeric>, bool> = true>
	static int32_t idToNumber(const Numeric &t)
	{
		return t;
	}

	template<typename IdentifierType, std::enable_if_t<std::is_base_of_v<IdentifierBase, IdentifierType>, bool> = true>
	static int32_t idToNumber(const IdentifierType &t)
	{
		return t.getNum();
	}

	template <typename T, typename U>
	void registerVectoredType(const std::vector<T*> *Vector, const std::function<U(const T&)> &idRetriever)
	{
		vectors[&typeid(T)] = VectorizedObjectInfo<T, U>(Vector, idRetriever);
	}
	template <typename T, typename U>
	void registerVectoredType(const std::vector<ConstTransitivePtr<T> > *Vector, const std::function<U(const T&)> &idRetriever)
	{
		vectors[&typeid(T)] = VectorizedObjectInfo<T, U>(Vector, idRetriever);
	}

	using TTypeVecMap = std::map<const std::type_info *, std::any, TypeComparer>;
	TTypeVecMap vectors; //entry must be a pointer to vector containing pointers to the objects of key type

public:
	bool smartVectorMembersSerialization = false;
	bool sendStackInstanceByIds = false;

	~CSerializer();

	virtual void reportState(vstd::CLoggerBase * out){};

	template <typename T, typename U>
	const VectorizedObjectInfo<T, U> *getVectorizedTypeInfo()
	{
		const std::type_info *myType = nullptr;

		myType = &typeid(T);

		auto i = vectors.find(myType);
		if(i == vectors.end())
			return nullptr;
		else
		{
			assert(i->second.has_value());
#ifndef __APPLE__
			assert(i->second.type() == typeid(VectorizedObjectInfo<T, U>));
#endif
			auto *ret = std::any_cast<VectorizedObjectInfo<T, U>>(&i->second);
			return ret;
		}
	}

	template <typename T, typename U>
	T* getVectorItemFromId(const VectorizedObjectInfo<T, U> &oInfo, U id) const
	{
		si32 idAsNumber = idToNumber(id);

		assert(oInfo.vector);
		assert(static_cast<si32>(oInfo.vector->size()) > idAsNumber);
		return const_cast<T*>((*oInfo.vector)[idAsNumber].get());
	}

	template <typename T, typename U>
	U getIdFromVectorItem(const VectorizedObjectInfo<T, U> &oInfo, const T* obj) const
	{
		if(!obj)
			return U(-1);

		return oInfo.idRetriever(*obj);
	}

	void addStdVecItems(CGameState *gs, LibClasses *lib = VLC);
};

/// Helper to detect classes with user-provided serialize(S&, int version) method
template<class S, class T>
struct is_serializeable
{
	using Yes = char (&)[1];
	using No = char (&)[2];

	template<class U>
	static Yes test(U * data, S* arg1 = 0,
					typename std::enable_if<std::is_void<
							 decltype(data->serialize(*arg1, int(0)))
					>::value>::type * = 0);
	static No test(...);
	static const bool value = sizeof(Yes) == sizeof(is_serializeable::test((typename std::remove_reference<typename std::remove_cv<T>::type>::type*)0));
};

template <typename T> //metafunction returning CGObjectInstance if T is its derivate or T elsewise
struct VectorizedTypeFor
{
	using type = std::conditional_t<std::is_base_of_v<CGObjectInstance, T>, CGObjectInstance, T>;
};

template <>
struct VectorizedTypeFor<CGHeroInstance>
{
	using type = CGHeroInstance;
};

template <typename T>
struct VectorizedIDType
{
	using type = std::conditional_t<std::is_base_of_v<CGObjectInstance, T>, ObjectInstanceID, int32_t>;
};

template <>
struct VectorizedIDType<CArtifactInstance>
{
	using type = ArtifactInstanceID;
};

template <>
struct VectorizedIDType<CGHeroInstance>
{
	using type = HeroTypeID;
};

/// Base class for deserializers
class DLL_LINKAGE IBinaryReader : public virtual CSerializer
{
public:
	virtual int read(void * data, unsigned size) = 0;
};

/// Base class for serializers
class DLL_LINKAGE IBinaryWriter : public virtual CSerializer
{
public:
	virtual int write(const void * data, unsigned size) = 0;
};

VCMI_LIB_NAMESPACE_END
