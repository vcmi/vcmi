#pragma once

#include <QVariant>
#include "../lib/JsonNode.h"

namespace JsonUtils
{
	QVariant toVariant(const JsonNode & node);
	QVariant JsonFromFile(QString filename);

	JsonNode toJson(QVariant object);
	void JsonToFile(QString filename, QVariant object);
}