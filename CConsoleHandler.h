#ifndef CCONSOLEHANDLER_H
#define CCONSOLEHANDLER_H
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
	static void killConsole(void *hThread); //for windows only, use native handle to the thread
	template<typename T> void print(const T &data, int level)
	{
		setColor(level);
		std::cout << data << std::flush;
		setColor(-1);
	}
};

#endif //CCONSOLEHANDLER_H
