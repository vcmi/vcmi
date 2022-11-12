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
extern int __argc;
extern char ** __argv;
#ifdef VCMI_IOS
extern "C" void launchGame(int argc, char * argv[]);
#endif
