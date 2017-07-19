/*
 * jsonutils.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
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
