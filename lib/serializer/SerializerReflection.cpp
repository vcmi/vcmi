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

#include "../GameSettings.h"
#include "../RiverHandler.h"
#include "../RoadHandler.h"
#include "../TerrainHandler.h"
#include "../entities/hero/CHero.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../mapping/CMapInfo.h"
#include "../mapping/CCastleEvent.h"
#include "../rmg/CMapGenOptions.h"

template<typename Type>
class SerializerReflection final : public ISerializerReflection
{
public:
	Serializeable * createPtr(BinaryDeserializer &ar, IGameInfoCallback * cb) const override
	{
		return ClassObjectCreator<Type>::invoke(cb);
	}

	void loadPtr(BinaryDeserializer &ar, IGameInfoCallback * cb, Serializeable * data) const override
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
	Serializeable * createPtr(BinaryDeserializer &ar, IGameInfoCallback * cb) const override
	{
		return ClassObjectCreator<Type>::invoke(cb);
	}

	void savePtr(BinarySerializer &s, const Serializeable *data) const override
	{
		throw std::runtime_error("Illegal call to savePtr - this type should not be used for serialization!");
	}
};

/// Loads pre-QUEST_REWORK CGBorderGuard instances (type id 10, removed in the quest
/// rework) as QuestGuard, reading the old CGKeys-based layout and synthesising
/// the requiredKeys limiter from the object's colour. Load-only.
class BorderGuardCompatibility final : public ISerializerReflection
{
public:
	Serializeable * createPtr(BinaryDeserializer & ar, IGameInfoCallback * cb) const override
	{
		return ClassObjectCreator<QuestGuard>::invoke(cb);
	}

	void loadPtr(BinaryDeserializer & ar, IGameInfoCallback * cb, Serializeable * data) const override
	{
		loadLegacyBorderGuard(ar, *dynamic_cast<QuestGuard*>(data));
	}

	void savePtr(BinarySerializer & s, const Serializeable * data) const override
	{
		throw std::runtime_error("Legacy CGBorderGuard must never be saved!");
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
	// legacy: pre-QUEST_REWORK CGBorderGuard (type id 10) loads as a QuestGuard
	apps[10].reset(new BorderGuardCompatibility());
}

CSerializationApplier & CSerializationApplier::getInstance()
{
	static CSerializationApplier registry;
	return registry;
}
