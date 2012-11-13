#pragma once

/*
 * CConsoleHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


/// Class which wraps the native console. It can print text based on
/// the chosen color
class DLL_LINKAGE CConsoleHandler
{
public:
	boost::function<void(const std::string &)> *cb; //function to be called when message is received
	int curLvl; //logging level
	boost::thread *thread;

	int run();
	void setColor(int level); //sets color of text appropriate for given logging level

	CConsoleHandler(); //c-tor
	~CConsoleHandler(); //d-tor
	void start(); //starts listening thread
	void end(); //kills listening thread

	template<typename T> void print(const T &data, int lvl)
	{
#ifndef _WIN32
		// with love from ffmpeg - library is trying to print some warnings from separate thread
		// this results in broken console on Linux. Lock stdout to print all our data at once
		flockfile(stdout);
#endif
		setColor(lvl);
		std::cout << data << std::flush;
		setColor(-1);
#ifndef _WIN32
		funlockfile(stdout);
#endif
	}
};
