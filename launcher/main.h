/*
 * main.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

void startGame(const QStringList & args);
void startEditor(const QStringList & args);

#ifdef VCMI_IOS
extern "C" void launchGame(int argc, char * argv[]);
#else
void startExecutable(QString name, const QStringList & args);
#endif
