#ifndef __CTHREADHELPER_H__
#define __CTHREADHELPER_H__

#include "global.h"
#include <boost/function.hpp>
#include <boost/thread.hpp>


/*
 * CThreadHelper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

typedef boost::function<void()> Task;

class CThreadHelper
{
	boost::mutex rtinm;
	int currentTask, amount, threads;
	std::vector<Task> *tasks;


	void processTasks();
public:
	CThreadHelper(std::vector<boost::function<void()> > *Tasks, int Threads);
	void run();
};

template <typename T> inline void setData(T * data, boost::function<T()> func)
{
	*data = func();
}


#define GET_DATA(TYPE,DESTINATION,FUNCTION_TO_GET) \
	(boost::bind(&setData<TYPE>,&DESTINATION,FUNCTION_TO_GET))
#define GET_SURFACE(SUR_DESTINATION, SUR_NAME) \
	(GET_DATA \
		(SDL_Surface*,SUR_DESTINATION,\
		boost::function<SDL_Surface*()>(boost::bind(&BitmapHandler::loadBitmap,SUR_NAME,true))))
#define GET_DEF(DESTINATION, DEF_NAME) \
	(GET_DATA \
		(CDefHandler*,DESTINATION,\
		boost::function<CDefHandler*()>(boost::bind(CDefHandler::giveDef,DEF_NAME,(CLodHandler*)NULL))))
#define GET_DEF_ESS(DESTINATION, DEF_NAME) \
	(GET_DATA \
		(CDefEssential*,DESTINATION,\
		boost::function<CDefEssential*()>(boost::bind(CDefHandler::giveDefEss,DEF_NAME,(CLodHandler*)NULL))))

#endif // __CTHREADHELPER_H__
