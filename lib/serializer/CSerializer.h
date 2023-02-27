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

const ui32 SERIALIZATION_VERSION = 819;
const ui32 MINIMAL_SERIALIZATION_VERSION = 819;
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
	template<typename T>
	static si32 idToNumber(const T &t, typename boost::enable_if<boost::is_convertible<T,si32> >::type * dummy = 0)
	{
		return t;
	}

	template<typename T, typename NT>
	static NT idToNumber(const BaseForID<T, NT> &t)
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

	typedef std::map<const std::type_info *, boost::any, TypeComparer> TTypeVecMap;
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

		TTypeVecMap::iterator i = vectors.find(myType);
		if(i == vectors.end())
			return nullptr;
		else
		{
			assert(!i->second.empty());
#ifndef __APPLE__
			assert(i->second.type() == typeid(VectorizedObjectInfo<T, U>));
#endif
			VectorizedObjectInfo<T, U> *ret = &(boost::any_cast<VectorizedObjectInfo<T, U>&>(i->second));
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
	typedef char (&Yes)[1];
	typedef char (&No)[2];

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
	typedef typename
		//if
		boost::mpl::eval_if<std::is_same<CGHeroInstance,T>,
		boost::mpl::identity<CGHeroInstance>,
		//else if
		boost::mpl::eval_if<std::is_base_of<CGObjectInstance,T>,
		boost::mpl::identity<CGObjectInstance>,
		//else
		boost::mpl::identity<T>
		> >::type type;
};
template <typename U>
struct VectorizedIDType
{
	typedef typename
		//if
		boost::mpl::eval_if<std::is_same<CArtifact,U>,
		boost::mpl::identity<ArtifactID>,
		//else if
		boost::mpl::eval_if<std::is_same<CCreature,U>,
		boost::mpl::identity<CreatureID>,
		//else if
		boost::mpl::eval_if<std::is_same<CHero,U>,
		boost::mpl::identity<HeroTypeID>,
		//else if
		boost::mpl::eval_if<std::is_same<CArtifactInstance,U>,
		boost::mpl::identity<ArtifactInstanceID>,
		//else if
		boost::mpl::eval_if<std::is_same<CGHeroInstance,U>,
		boost::mpl::identity<HeroTypeID>,
		//else if
		boost::mpl::eval_if<std::is_base_of<CGObjectInstance,U>,
		boost::mpl::identity<ObjectInstanceID>,
		//else
		boost::mpl::identity<si32>
		> > > > > >::type type;
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
