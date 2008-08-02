#ifndef CCONSOLEHANDLER_H
#define CCONSOLEHANDLER_H
class CCallback;
class CConsoleHandler
{
	CCallback * cb;
public:
	void runConsole();

	friend class CClient;

#ifndef __GNUC__
	friend int _tmain(int argc, _TCHAR* argv[]);
#else
	friend int main(int argc, _TCHAR* argv[]);
#endif
};

#endif //CCONSOLEHANDLER_H
