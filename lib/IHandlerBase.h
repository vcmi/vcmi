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

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class Entity;

/// base class for all handlers that can be accessed from mod system
class DLL_LINKAGE IHandlerBase : boost::noncopyable
{
protected:
	static std::string getScopeBuiltin();

	/// Calls modhandler. Mostly needed to avoid large number of includes in headers
	static void registerObject(const std::string & scope, const std::string & type_name, const std::string & name, si32 index);

public:
	/// loads all original game data in vector of json nodes
	/// dataSize - is number of items that must be loaded (normally - constant from GameConstants)
	virtual std::vector<JsonNode> loadLegacyData() = 0;

	/// loads single object into game. Scope is namespace of this object, same as name of source mod
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data) = 0;
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) = 0;

	/// allows handlers to alter object configuration before validation and actual load
	virtual void beforeValidate(JsonNode & object){};

	/// allows handler to load some custom internal data before identifier finalization
	virtual void loadCustom(){};

	/// allows handler to do post-loading step for validation or integration of loaded data
	virtual void afterLoadFinalization(){};

	virtual ~IHandlerBase() = default;
};

template <class _ObjectID, class _ObjectBase, class _Object, class _ServiceBase>
class CHandlerBase : public _ServiceBase, public IHandlerBase
{
	const _Object * getObjectImpl(const int32_t index) const
	{
		try
		{
			return objects.at(index).get();
		}
		catch (const std::out_of_range&)
		{
			logMod->error("%s id %d is invalid", getTypeNames()[0], index);
			throw std::runtime_error("Attempt to access invalid index " + std::to_string(index) + " of type " + getTypeNames().front());
		}
	}

public:
	using ObjectPtr = std::shared_ptr<_Object>;

	const Entity * getBaseByIndex(const int32_t index) const override
	{
		return getObjectImpl(index);
	}

	const _ObjectBase * getById(const _ObjectID & id) const override
	{
		return getObjectImpl(id.getNum());
	}

	const _ObjectBase * getByIndex(const int32_t index) const override
	{
		return getObjectImpl(index);
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
		objects.push_back(loadFromJson(scope, data, name, objects.size()));

		for(const auto & type_name : getTypeNames())
			registerObject(scope, type_name, name, objects.back()->getIndex());
	}

	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override
	{
		assert(objects[index] == nullptr); // ensure that this id was not loaded before
		objects[index] = loadFromJson(scope, data, name, index);

		for(const auto & type_name : getTypeNames())
			registerObject(scope, type_name, name, objects[index]->getIndex());
	}

	const _Object * operator[] (const _ObjectID id) const
	{
		return getObjectImpl(id.getNum());
	}

	const _Object * operator[] (int32_t index) const
	{
		return getObjectImpl(index);
	}

	size_t size() const
	{
		return objects.size();
	}

protected:
	virtual ObjectPtr loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index) = 0;
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
	std::vector<ObjectPtr> objects;
};

VCMI_LIB_NAMESPACE_END
