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

#include "RegisterTypes.h"

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
		const_cast<Type*>(ptr)->serialize(s);
	}
};

template<typename Type, ESerializationVersion maxVersion>
class SerializerCompatibility : public ISerializerReflection
{
public:
	Serializeable * createPtr(BinaryDeserializer &ar, IGameCallback * cb) const override
	{
		return ClassObjectCreator<Type>::invoke(cb);
	}

	void savePtr(BinarySerializer &s, const Serializeable *data) const override
	{
		throw std::runtime_error("Illegal call to savePtr - this type should not be used for serialization!");
	}
};

class SerializerCompatibilityBonusingBuilding final : public SerializerCompatibility<TownRewardableBuildingInstance, ESerializationVersion::NEW_TOWN_BUILDINGS>
{
	void loadPtr(BinaryDeserializer &ar, IGameCallback * cb, Serializeable * data) const override
	{
		auto * realPtr = dynamic_cast<TownRewardableBuildingInstance *>(data);
		realPtr->serialize(ar);
	}
};

class SerializerCompatibilityArtifactsAltar final : public SerializerCompatibility<CGMarket, ESerializationVersion::NEW_MARKETS>
{
	void loadPtr(BinaryDeserializer &ar, IGameCallback * cb, Serializeable * data) const override
	{
		auto * realPtr = dynamic_cast<CGMarket *>(data);
		realPtr->serializeArtifactsAltar(ar);
	}
};

template<typename Type>
void CSerializationApplier::registerType(uint16_t ID)
{
	assert(!apps.count(ID));
	apps[ID].reset(new SerializerReflection<Type>);
}

CSerializationApplier::CSerializationApplier()
{
	registerTypes(*this);

	apps[54].reset(new SerializerCompatibilityBonusingBuilding);
	apps[55].reset(new SerializerCompatibilityBonusingBuilding);
	apps[81].reset(new SerializerCompatibilityArtifactsAltar);
}

CSerializationApplier & CSerializationApplier::getInstance()
{
	static CSerializationApplier registry;
	return registry;
}

VCMI_LIB_NAMESPACE_END
