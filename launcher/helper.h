/*
 * helper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QString>

class QObject;

namespace Helper
{
void loadSettings();
void enableScrollBySwiping(QObject * scrollTarget);
QString getRealPath(QString path);
void performNativeCopy(QString src, QString dst);
void revealDirectoryInFileBrowser(QString path);
}
