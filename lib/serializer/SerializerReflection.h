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

class IGameCallback;
class Serializeable;
class GameCallbackHolder;
class BinaryDeserializer;
class BinarySerializer;
class GameCallbackHolder;

template <typename T, typename Enable = void>
struct ClassObjectCreator
{
	static T *invoke(IGameCallback *cb)
	{
		static_assert(!std::is_base_of_v<GameCallbackHolder, T>, "Cannot call new upon map objects!");
		static_assert(!std::is_abstract_v<T>, "Cannot call new upon abstract classes!");
		return new T();
	}
};

template<typename T>
struct ClassObjectCreator<T, typename std::enable_if_t<std::is_abstract_v<T>>>
{
	static T *invoke(IGameCallback *cb)
	{
		throw std::runtime_error("Something went really wrong during deserialization. Attempted creating an object of an abstract class " + std::string(typeid(T).name()));
	}
};

template<typename T>
struct ClassObjectCreator<T, typename std::enable_if_t<std::is_base_of_v<GameCallbackHolder, T> && !std::is_abstract_v<T>>>
{
	static T *invoke(IGameCallback *cb)
	{
		static_assert(!std::is_abstract_v<T>, "Cannot call new upon abstract classes!");
		return new T(cb);
	}
};

class ISerializerReflection : boost::noncopyable
{
public:
	virtual Serializeable * createPtr(BinaryDeserializer &ar, IGameCallback * cb) const =0;
	virtual void loadPtr(BinaryDeserializer &ar, IGameCallback * cb, Serializeable * data) const =0;
	virtual void savePtr(BinarySerializer &ar, const Serializeable *data) const =0;
	virtual ~ISerializerReflection() = default;
};

class DLL_LINKAGE CSerializationApplier
{
	std::map<int32_t, std::unique_ptr<ISerializerReflection>> apps;

	template<typename RegisteredType>
	void addApplier(ui16 ID);

	CSerializationApplier();
public:
	ISerializerReflection * getApplier(ui16 ID)
	{
		if(!apps.count(ID))
			throw std::runtime_error("No applier found.");
		return apps[ID].get();
	}

	template<typename Base, typename Derived>
	void registerType(const Base * b = nullptr, const Derived * d = nullptr);

	static CSerializationApplier & getInstance();
};
