/*
 * CConsoleHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

/** The color enum is used for colored text console output. */
enum class EConsoleTextColor : int8_t
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

/// Class which wraps the native console. It can print text based on
/// the chosen color
class DLL_LINKAGE CConsoleHandler
{
public:
	CConsoleHandler(const std::function<void(const std::string &, bool)> & callback);
	CConsoleHandler();
	~CConsoleHandler();
	void start(); //starts listening thread

	template<typename T> void print(const T &data, bool addNewLine = false, EConsoleTextColor color = EConsoleTextColor::DEFAULT, bool printToStdErr = false)
	{
		TLockGuard _(smx);
#ifndef VCMI_WINDOWS
		// with love from ffmpeg - library is trying to print some warnings from separate thread
		// this results in broken console on Linux. Lock stdout to print all our data at once
		flockfile(stdout);
#endif
		if(color != EConsoleTextColor::DEFAULT) setColor(color);
		if(printToStdErr)
		{
			std::cerr << data;
			if(addNewLine)
			{
				std::cerr << std::endl;
			}
			else
			{
				std::cerr << std::flush;
			}
		}
		else
		{
			std::cout << data;
			if(addNewLine)
			{
				std::cout << std::endl;
			}
			else
			{
				std::cout << std::flush;
			}
		}

		if(color != EConsoleTextColor::DEFAULT) setColor(EConsoleTextColor::DEFAULT);
#ifndef VCMI_WINDOWS
		funlockfile(stdout);
#endif
	}

private:

#ifndef VCMI_WINDOWS
	using TColor = std::string;
#else
	using TColor = int32_t;
#endif

	int run();

	void end(); //kills listening thread

	void setColor(EConsoleTextColor color); //sets color of text appropriate for given logging level

	TColor defColor;
	TColor defErrColor;

	//function to be called when message is received - string: message, bool: whether call was made from in-game console
	std::function<void(const std::string &, bool)> cb;

	std::condition_variable shutdownVariable;
	std::mutex shutdownMutex;
	std::atomic<bool> shutdownPending = false;

	std::mutex smx;

	std::thread thread;
};

VCMI_LIB_NAMESPACE_END
