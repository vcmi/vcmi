#include "StdInc.h"
#include "jsonutils.h"
#include "../lib/filesystem/FileStream.h"

static QVariantMap JsonToMap(const JsonMap & json)
{
	QVariantMap map;
	for (auto & entry : json)
	{
		map.insert(QString::fromUtf8(entry.first.c_str()), JsonUtils::toVariant(entry.second));
	}
	return map;
}

static QVariantList JsonToList(const JsonVector & json)
{
	QVariantList list;
	for (auto & entry : json)
	{
		list.push_back(JsonUtils::toVariant(entry));
	}
	return list;
}

static JsonVector VariantToList(QVariantList variant)
{
	JsonVector vector;
	for (auto & entry : variant)
	{
		vector.push_back(JsonUtils::toJson(entry));
	}
	return vector;
}

static JsonMap VariantToMap(QVariantMap variant)
{
	JsonMap map;
	for (auto & entry : variant.toStdMap())
	{
		map[entry.first.toUtf8().data()] = JsonUtils::toJson(entry.second);
	}
	return map;
}

namespace JsonUtils
{

QVariant toVariant(const JsonNode & node)
{
	switch (node.getType())
	{
		break; case JsonNode::DATA_NULL:   return QVariant();
		break; case JsonNode::DATA_BOOL:   return QVariant(node.Bool());
		break; case JsonNode::DATA_FLOAT:  return QVariant(node.Float());
		break; case JsonNode::DATA_STRING: return QVariant(QString::fromUtf8(node.String().c_str()));
		break; case JsonNode::DATA_VECTOR: return JsonToList(node.Vector());
		break; case JsonNode::DATA_STRUCT: return JsonToMap(node.Struct());
	}
	return QVariant();
}

QVariant JsonFromFile(QString filename)
{
	QFile file(filename);
	file.open(QFile::ReadOnly);
	auto data = file.readAll();

	if (data.size() == 0)
	{
		logGlobal->errorStream() << "Failed to open file " << filename.toUtf8().data();
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

	if (object.canConvert<QVariantMap>())
		ret.Struct() = VariantToMap(object.toMap());
	else if (object.canConvert<QVariantList>())
		ret.Vector() = VariantToList(object.toList());
	else if (static_cast<QMetaType::Type>(object.type()) == QMetaType::QString)
		ret.String() = object.toString().toUtf8().data();
	else if (static_cast<QMetaType::Type>(object.type()) == QMetaType::Bool)
		ret.Bool() = object.toBool();
	else if (object.canConvert<double>())
		ret.Float() = object.toFloat();

	return ret;
}

void JsonToFile(QString filename, QVariant object)
{
	FileStream file(qstringToPath(filename), std::ios::out | std::ios_base::binary);
	file << toJson(object);
}

}
