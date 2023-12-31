/*
 * JsonTreeSerializer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

template <typename T>
class JsonTreeSerializer : public JsonSerializeFormat
{
public:
	const JsonNode & getCurrent() override
	{
		return * currentObject;
	}

protected:
	T currentObject;
	std::vector<T> treeRoute;

	JsonTreeSerializer(const IInstanceResolver * instanceResolver_, T root, const bool saving_, const bool updating_):
		JsonSerializeFormat(instanceResolver_, saving_, updating_),
		currentObject(root)
	{
	}

	void pop() override
	{
		assert(!treeRoute.empty());
		currentObject = treeRoute.back();
		treeRoute.pop_back();
	}

	void pushStruct(const std::string & fieldName) override
	{
		pushObject(fieldName);
	}

	void pushArray(const std::string & fieldName) override
	{
		pushObject(fieldName);
	}

	void pushArrayElement(const size_t index) override
	{
		pushObject(&(currentObject->Vector().at(index)));
	}

	void pushField(const std::string & fieldName) override
	{
		pushObject(fieldName);
	}

private:
	void pushObject(const std::string & fieldName)
	{
		pushObject(&(currentObject->operator[](fieldName)));
	}

	void pushObject(T newCurrentObject)
	{
		treeRoute.push_back(currentObject);
		currentObject = newCurrentObject;
	}
};

VCMI_LIB_NAMESPACE_END
