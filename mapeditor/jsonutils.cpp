/*
 * jsonutils.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "jsonutils.h"

static QVariantMap JsonToMap(const JsonMap & json)
{
	QVariantMap map;
	for(auto & entry : json)
	{
		map.insert(QString::fromUtf8(entry.first.c_str()), JsonUtils::toVariant(entry.second));
	}
	return map;
}

static QVariantList JsonToList(const JsonVector & json)
{
	QVariantList list;
	for(auto & entry : json)
	{
		list.push_back(JsonUtils::toVariant(entry));
	}
	return list;
}

static JsonVector VariantToList(QVariantList variant)
{
	JsonVector vector;
	for(auto & entry : variant)
	{
		vector.push_back(JsonUtils::toJson(entry));
	}
	return vector;
}

static JsonMap VariantToMap(QVariantMap variant)
{
	JsonMap map;
	for(auto & entry : variant.toStdMap())
	{
		map[entry.first.toUtf8().data()] = JsonUtils::toJson(entry.second);
	}
	return map;
}

VCMI_LIB_NAMESPACE_BEGIN

namespace JsonUtils
{

QVariant toVariant(const JsonNode & node)
{
	switch(node.getType())
	{
		break;
	case JsonNode::JsonType::DATA_NULL:
		return QVariant();
		break;
	case JsonNode::JsonType::DATA_BOOL:
		return QVariant(node.Bool());
		break;
	case JsonNode::JsonType::DATA_FLOAT:
		return QVariant(node.Float());
		break;
	case JsonNode::JsonType::DATA_STRING:
		return QVariant(QString::fromUtf8(node.String().c_str()));
		break;
	case JsonNode::JsonType::DATA_VECTOR:
		return JsonToList(node.Vector());
		break;
	case JsonNode::JsonType::DATA_STRUCT:
		return JsonToMap(node.Struct());
	}
	return QVariant();
}

QVariant JsonFromFile(QString filename)
{
	QFile file(filename);
	file.open(QFile::ReadOnly);
	auto data = file.readAll();

	if(data.size() == 0)
	{
		logGlobal->error("Failed to open file %s", filename.toUtf8().data());
		return QVariant();
	}
	else
	{
		JsonNode node(data.data(), data.size());
		return toVariant(node);
	}
}

JsonNode toJson(QVariant object)
{
	JsonNode ret;

	if(object.canConvert<QVariantMap>())
		ret.Struct() = VariantToMap(object.toMap());
	else if(object.canConvert<QVariantList>())
		ret.Vector() = VariantToList(object.toList());
	else if(object.userType() == QMetaType::QString)
		ret.String() = object.toString().toUtf8().data();
	else if(object.userType() == QMetaType::Bool)
		ret.Bool() = object.toBool();
	else if(object.canConvert<double>())
		ret.Float() = object.toFloat();

	return ret;
}

void JsonToFile(QString filename, QVariant object)
{
	std::fstream file(qstringToPath(filename).c_str(), std::ios::out | std::ios_base::binary);
	file << toJson(object).toJson();
}

}

VCMI_LIB_NAMESPACE_END
