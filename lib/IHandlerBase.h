/*
 * IHandlerBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/ConstTransitivePtr.h"
#include "VCMI_Lib.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class Entity;

/// base class for all handlers that can be accessed from mod system
class DLL_LINKAGE IHandlerBase
{
protected:
	std::string getScopeBuiltin() const;

	/// Calls modhandler. Mostly needed to avoid large number of includes in headers
	void registerObject(std::string scope, std::string type_name, std::string name, si32 index);
public:
	/// loads all original game data in vector of json nodes
	/// dataSize - is number of items that must be loaded (normally - constant from GameConstants)
	virtual std::vector<JsonNode> loadLegacyData(size_t dataSize) = 0;

	/// loads single object into game. Scope is namespace of this object, same as name of source mod
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data) = 0;
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) = 0;

	/// allows handlers to alter object configuration before validation and actual load
	virtual void beforeValidate(JsonNode & object){};

	/// allows handler to load some custom internal data before identifier finalization
	virtual void loadCustom(){};

	/// allows handler to do post-loading step for validation or integration of loaded data
	virtual void afterLoadFinalization(){};

	/**
	 * Gets a list of objects that are allowed by default on maps
	 *
	 * @return a list of allowed objects, the index is the object id
	 */
	virtual std::vector<bool> getDefaultAllowed() const = 0;

	virtual ~IHandlerBase(){}
};

template <class _ObjectID, class _ObjectBase, class _Object, class _ServiceBase> class CHandlerBase : public _ServiceBase, public IHandlerBase
{
public:
	virtual ~CHandlerBase()
	{
		for(auto & o : objects)
		{
			o.dellNull();
		}

	}

	const Entity * getBaseByIndex(const int32_t index) const override
	{
		return getByIndex(index);
	}

	const _ObjectBase * getById(const _ObjectID & id) const override
	{
		return (*this)[id].get();
	}

	const _ObjectBase * getByIndex(const int32_t index) const override
	{
		return (*this)[_ObjectID(index)].get();
	}

	void forEachBase(const std::function<void(const Entity * entity, bool & stop)> & cb) const override
	{
		forEachT(cb);
	}

	void forEach(const std::function<void(const _ObjectBase * entity, bool & stop)> & cb) const override
	{
		forEachT(cb);
	}

	void loadObject(std::string scope, std::string name, const JsonNode & data) override
	{
		auto object = loadFromJson(scope, data, name, objects.size());

		objects.push_back(object);

		for(auto type_name : getTypeNames())
			registerObject(scope, type_name, name, object->getIndex());
	}

	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override
	{
		auto object = loadFromJson(scope, data, name, index);

		assert(objects[index] == nullptr); // ensure that this id was not loaded before
		objects[index] = object;

		for(auto type_name : getTypeNames())
			registerObject(scope, type_name, name, object->getIndex());
	}

	ConstTransitivePtr<_Object> operator[] (const _ObjectID id) const
	{
		const int32_t raw_id = id.getNum();
		return operator[](raw_id);
	}

	ConstTransitivePtr<_Object> operator[] (int32_t index) const
	{
		if(index < 0 || index >= objects.size())
		{
			logMod->error("%s id %d is invalid", getTypeNames()[0], index);
			throw std::runtime_error("internal error");
		}

		return objects[index];
	}

	void updateEntity(int32_t index, const JsonNode & data)
	{
		if(index < 0 || index >= objects.size())
		{
			logMod->error("%s id %d is invalid", getTypeNames()[0], index);
		}
		else
		{
			objects.at(index)->updateFrom(data);
		}
	}

	size_t size() const
	{
		return objects.size();
	}

protected:
	virtual _Object * loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index) = 0;
	virtual const std::vector<std::string> & getTypeNames() const = 0;

	template<typename ItemType>
	void forEachT(const std::function<void(const ItemType *, bool &)> & cb) const
	{
		bool stop = false;

		for(auto & object : objects)
		{
			cb(object.get(), stop);
			if(stop)
				break;
		}
	}

public: //todo: make private
	std::vector<ConstTransitivePtr<_Object>> objects;
};

VCMI_LIB_NAMESPACE_END
