#ifndef CCONSOLEHANDLER_H
#define CCONSOLEHANDLER_H
class CCallback;
class CConsoleHandler
{
	CCallback * cb;
public:
	void runConsole();

	friend class CClient;
};

#endif //CCONSOLEHANDLER_H