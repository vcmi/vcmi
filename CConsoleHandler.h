#ifndef __CCONSOLEHANDLER_H__
#define __CCONSOLEHANDLER_H__

#ifndef _WIN32
#define WORD std::string
#endif

#ifndef _WIN32
#define	_kill_thread(a,b) pthread_cancel(a);
#else
#define _kill_thread(a,b) TerminateThread(a,b);
#endif

/*
 * CConsoleHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

namespace boost
{
	template<typename signature>
	class function;
}
class DLL_EXPORT CConsoleHandler
{
public:
	boost::function<void(const std::string &)> *cb; //function to be called when message is received
	int curLvl; //logging level
	int run();
	void setColor(int level); //sets color of text appropriate for given logging level
	CConsoleHandler(); //c-tor
	~CConsoleHandler(); //d-tor
#ifndef _WIN32
	static void killConsole(pthread_t hThread); //for windows only, use native handle to the thread
#else
	static void killConsole(void *hThread); //for windows only, use native handle to the thread
#endif
	template<typename T> void print(const T &data, int level)
	{
		setColor(level);
		std::cout << data << std::flush;
		setColor(-1);
	}
};


#endif // __CCONSOLEHANDLER_H__
