/*
 * StdInc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../Global.h"

#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_for_each.h"
#undef emit // Qt defines also emit -> compile error -> we don't need it

#include <QtWidgets>
#include <QStringList>
#include <QSet>
#include <QVector>
#include <QList>
#include <QString>
#include <QFile>
#include <QTemporaryDir>

#include "../vcmiqt/convpathqstring.h"

VCMI_LIB_USING_NAMESPACE
