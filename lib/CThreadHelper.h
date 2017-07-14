/*
 * CThreadHelper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once


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
