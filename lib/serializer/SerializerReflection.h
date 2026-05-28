/*
 * SerializerReflection.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class IGameInfoCallback;
class Serializeable;
class GameCallbackHolder;
class BinaryDeserializer;
class BinarySerializer;
class GameCallbackHolder;

template <typename T, typename Enable = void>
struct ClassObjectCreator
{
	static T *invoke(IGameInfoCallback *cb)
	{
		static_assert(!std::is_base_of_v<GameCallbackHolder, T>, "Cannot call new upon map objects!");
		static_assert(!std::is_abstract_v<T>, "Cannot call new upon abstract classes!");
		return new T();
	}
};

template<typename T>
struct ClassObjectCreator<T, typename std::enable_if_t<std::is_base_of_v<GameCallbackHolder, T>>>
{
	static T *invoke(IGameInfoCallback *cb)
	{
		static_assert(!std::is_abstract_v<T>, "Cannot call new upon abstract classes!");
		assert(cb != nullptr);
		return new T(cb);
	}
};

class ISerializerReflection
{
public:
	virtual Serializeable * createPtr(BinaryDeserializer &ar, IGameInfoCallback * cb) const =0;
	virtual void loadPtr(BinaryDeserializer &ar, IGameInfoCallback * cb, Serializeable * data) const =0;
	virtual void savePtr(BinarySerializer &ar, const Serializeable *data) const =0;
	virtual ~ISerializerReflection() = default;
};

class DLL_LINKAGE CSerializationApplier : boost::noncopyable
{
	std::map<int32_t, std::unique_ptr<ISerializerReflection>> apps;

	CSerializationApplier();
public:
	ISerializerReflection * getApplier(uint16_t ID)
	{
		if(!apps.count(ID))
			throw std::runtime_error("No applier found.");
		return apps[ID].get();
	}

	template<typename Type>
	void registerType(uint16_t index);

	static CSerializationApplier & getInstance();
};

VCMI_LIB_NAMESPACE_END
