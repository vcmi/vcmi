#ifndef CCONSOLEHANDLER_H
#define CCONSOLEHANDLER_H
class CCallback;
class CConsoleHandler
{
	CCallback * cb;
public:
	void runConsole();

	friend int _tmain(int argc, _TCHAR* argv[]);
};

#endif //CCONSOLEHANDLER_H