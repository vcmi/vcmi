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
#ifdef VCMI_IOS
extern int __argc;
extern char ** __argv;

extern "C" void launchGame(int argc, char * argv[]);
#endif
