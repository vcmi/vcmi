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

namespace boost
{
	template<typename signature>
	class function;
}
class DLL_EXPORT CConsoleHandler
{
public:
	boost::function<void(const std::string &)> *cb;
	int curLvl;
	int run();
	void setColor(int level);
	CConsoleHandler();
	~CConsoleHandler();
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
