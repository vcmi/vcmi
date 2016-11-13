#pragma once

/*
 * IHandlerBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
 #include "../lib/ConstTransitivePtr.h"
 #include "VCMI_Lib.h"
 //#include "CModHandler.h"

class JsonNode;

/// base class for all handlers that can be accessed from mod system
class DLL_LINKAGE IHandlerBase
{
	// there also should be private member with such signature:
	// Object * loadFromJson(const JsonNode & json);
	// where Object is type of data loaded by handler
	// primary used in loadObject methods
protected:
	/// Calls modhandler. Mostly needed to avoid large number of includes in headers
	void registerObject(std::string scope, std::string type_name, std::string name, si32 index);
	std::string normalizeIdentifier(const std::string & scope, const std::string & remoteScope, const std::string & identifier) const;

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

template <class _ObjectID, class _Object> class CHandlerBase: public IHandlerBase
{
public:
	virtual ~CHandlerBase()
	{
		for(auto & o : objects)
		{
			o.dellNull();
		}

	}
	void loadObject(std::string scope, std::string name, const JsonNode & data) override
	{
		auto type_name = getTypeName();
		auto object = loadFromJson(data, normalizeIdentifier(scope, "core", name));
		object->id = _ObjectID(objects.size());

		objects.push_back(object);

		registerObject(scope, type_name, name, object->id);
	}
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override
	{
		auto type_name = getTypeName();
		auto object = loadFromJson(data, normalizeIdentifier(scope, "core", name));
		object->id = _ObjectID(index);


		assert(objects[index] == nullptr); // ensure that this id was not loaded before
		objects[index] = object;

		registerObject(scope,type_name, name, object->id);

	}

	ConstTransitivePtr<_Object> operator[] (const _ObjectID id) const
	{
		const auto raw_id = id.toEnum();

		if (raw_id < 0 || raw_id >= objects.size())
		{
			logGlobal->errorStream() << getTypeName() << " id " << static_cast<si64>(raw_id) << "is invalid";
			throw std::runtime_error ("internal error");
		}

		return objects[raw_id];
	}
protected:
	virtual _Object * loadFromJson(const JsonNode & json, const std::string & identifier) = 0;
	virtual const std::string getTypeName() const = 0;
public: //todo: make private
	std::vector<ConstTransitivePtr<_Object>> objects;
};
