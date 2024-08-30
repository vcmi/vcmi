/*
 * chroniclesextractor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../StdInc.h"

class ChroniclesExtractor : public QObject
{
	Q_OBJECT

public:
	static void installExe(QWidget *parent, QStringList exe);
};