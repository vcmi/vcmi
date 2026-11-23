/*
 * randommapcli.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QStringList>

class QCommandLineParser;

void addRandomMapOptions(QCommandLineParser & parser);
bool isRandomMapCommand(int argc, char * argv[]);
/// Exit code: 0 saved, 1 generation or saving failed, 2 invalid arguments.
int runRandomMapCommand(const QStringList & arguments);
