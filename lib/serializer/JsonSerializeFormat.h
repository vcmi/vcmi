/*
 * JsonSerializeFormat.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class JsonNode;

class JsonSerializeFormat;

class JsonStructSerializer: public boost::noncopyable
{
public:
	JsonStructSerializer(JsonStructSerializer && other);
	virtual ~JsonStructSerializer();

	JsonStructSerializer enterStruct(const std::string & fieldName);

	JsonNode & get();

	JsonSerializeFormat * operator->();
private:
	JsonStructSerializer(JsonSerializeFormat & owner_, const std::string & fieldName);
	JsonStructSerializer(JsonStructSerializer & parent, const std::string & fieldName);

	bool restoreState;
	JsonSerializeFormat & owner;
	JsonNode * parentNode;
	JsonNode * thisNode;
	friend class JsonSerializeFormat;
};

class JsonSerializeFormat
{
public:
	JsonSerializeFormat(JsonNode & root_);
	virtual ~JsonSerializeFormat() = default;

	JsonNode & getRoot()
	{
		return *root;
	};

	JsonStructSerializer enterStruct(const std::string & fieldName);

protected:
	JsonNode * root;
	JsonNode * current;
private:
	friend class JsonStructSerializer;
};

