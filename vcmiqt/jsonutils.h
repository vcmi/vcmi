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

#include "vcmiqt.h"

#include <QVariant>

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

namespace JsonUtils
{
VCMIQT_LINKAGE QVariant toVariant(const JsonNode & node);
VCMIQT_LINKAGE QVariant JsonFromFile(QString filename);

VCMIQT_LINKAGE JsonNode toJson(QVariant object);
VCMIQT_LINKAGE void JsonToFile(QString filename, QVariant object);
}

VCMI_LIB_NAMESPACE_END
