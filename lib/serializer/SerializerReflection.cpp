/*
 * SerializerReflection.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "SerializerReflection.h"

#include "BinaryDeserializer.h"
#include "BinarySerializer.h"
#include "CTypeList.h"

#include "../registerTypes/RegisterTypes.h"

VCMI_LIB_NAMESPACE_BEGIN

template<typename Type>
class SerializerReflection final : public ISerializerReflection
{
public:
	Serializeable * createPtr(BinaryDeserializer &ar, IGameCallback * cb) const override
	{
		return ClassObjectCreator<Type>::invoke(cb);
	}

	void loadPtr(BinaryDeserializer &ar, IGameCallback * cb, Serializeable * data) const override
	{
		auto * realPtr = dynamic_cast<Type *>(data);
		realPtr->serialize(ar);
	}

	void savePtr(BinarySerializer &s, const Serializeable *data) const override
	{
		const Type *ptr = dynamic_cast<const Type*>(data);

		//T is most derived known type, it's time to call actual serialize
		const_cast<Type*>(ptr)->serialize(s);
	}
};

template<typename RegisteredType>
void CSerializationApplier::addApplier(ui16 ID)
{
	if(!apps.count(ID))
	{
		logGlobal->info("Registering type %d (%s)", ID, typeid(RegisteredType).name());
		apps[ID].reset(new SerializerReflection<RegisteredType>);
	}
}

template<typename Base, typename Derived>
void CSerializationApplier::registerType(const Base * b, const Derived * d)
{
	addApplier<Base>(CTypeList::getInstance().getTypeID<Base>(nullptr));
	addApplier<Derived>(CTypeList::getInstance().getTypeID<Derived>(nullptr));
}

CSerializationApplier::CSerializationApplier()
{
	registerTypes(*this);
}

CSerializationApplier & CSerializationApplier::getInstance()
{
	static CSerializationApplier registry;
	return registry;
}

VCMI_LIB_NAMESPACE_END
