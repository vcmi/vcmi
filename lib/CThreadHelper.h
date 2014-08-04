#pragma once




/*
 * CThreadHelper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

typedef std::function<void()> Task;

/// Can assign CPU work to other threads/cores
class DLL_LINKAGE CThreadHelper
{
	boost::mutex rtinm;
	int currentTask, amount, threads;
	std::vector<Task> *tasks;


	void processTasks();
public:
	CThreadHelper(std::vector<std::function<void()> > *Tasks, int Threads);
	void run();
};

template <typename T> inline void setData(T * data, std::function<T()> func)
{
	*data = func();
}

void DLL_LINKAGE setThreadName(const std::string &name);

#define GET_DATA(TYPE,DESTINATION,FUNCTION_TO_GET) \
	(std::bind(&setData<TYPE>,&DESTINATION,FUNCTION_TO_GET))
#define GET_SURFACE(SUR_DESTINATION, SUR_NAME) \
	(GET_DATA \
		(SDL_Surface*,SUR_DESTINATION,\
		std::function<SDL_Surface*()>(std::bind(&BitmapHandler::loadBitmap,SUR_NAME,true))))
#define GET_DEF(DESTINATION, DEF_NAME) \
	(GET_DATA \
		(CDefHandler*,DESTINATION,\
		std::function<CDefHandler*()>(std::bind(CDefHandler::giveDef,DEF_NAME))))
#define GET_DEF_ESS(DESTINATION, DEF_NAME) \
	(GET_DATA \
		(CDefEssential*,DESTINATION,\
		std::function<CDefEssential*()>(std::bind(CDefHandler::giveDefEss,DEF_NAME))))
