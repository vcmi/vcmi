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

class QCommandLineParser;
class QCoreApplication;

void addRandomMapOptions(QCommandLineParser & parser);
bool isRandomMapCommand(int argc, char * argv[]);
int runRandomMapCommand(QCoreApplication & app);
