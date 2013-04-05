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

namespace EConsoleTextColor
{
/** The color enum is used for colored text console output. */
enum EConsoleTextColor
{
    DEFAULT = -1,
    GREEN,
    RED,
    MAGENTA,
    YELLOW,
    WHITE,
    GRAY,
    TEAL = -2
};
}

/// Class which wraps the native console. It can print text based on
/// the chosen color
class DLL_LINKAGE CConsoleHandler
{
public:
    CConsoleHandler(); //c-tor
    ~CConsoleHandler(); //d-tor
    void start(); //starts listening thread

    template<typename T> void print(const T &data, EConsoleTextColor::EConsoleTextColor color = EConsoleTextColor::DEFAULT, bool printToStdErr = false)
	{
        TLockGuard _(mx);
#ifndef _WIN32
		// with love from ffmpeg - library is trying to print some warnings from separate thread
		// this results in broken console on Linux. Lock stdout to print all our data at once
		flockfile(stdout);
#endif
        if(color != EConsoleTextColor::DEFAULT) setColor(color);
        if(printToStdErr)
        {
            std::cerr << data << std::flush;
        }
        else
        {
            std::cout << data << std::flush;
        }
        if(color != EConsoleTextColor::DEFAULT) setColor(EConsoleTextColor::DEFAULT);
#ifndef _WIN32
		funlockfile(stdout);
#endif
	}

    boost::function<void(const std::string &)> *cb; //function to be called when message is received

private:
    int run();

    void end(); //kills listening thread

    void setColor(EConsoleTextColor::EConsoleTextColor color); //sets color of text appropriate for given logging level

    mutable boost::mutex mx;

    boost::thread * thread;
};
