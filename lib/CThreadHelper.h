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

VCMI_LIB_NAMESPACE_BEGIN


///DEPRECATED
/// Can assign CPU work to other threads/cores
class DLL_LINKAGE CThreadHelper
{
public:
	using Task = std::function<void()>;
	CThreadHelper(std::vector<std::function<void()> > *Tasks, int Threads);
	void run();
private:
	boost::mutex rtinm;
	int currentTask, amount, threads;
	std::vector<Task> *tasks;


	void processTasks();
};

template<typename Payload>
class ThreadPool
{
public:
	using Task = std::function<void(std::shared_ptr<Payload>)>;
	using Tasks = std::vector<Task>;

	ThreadPool(Tasks * tasks_, std::vector<std::shared_ptr<Payload>> context_)
		: currentTask(0),
		amount(tasks_->size()),
		threads(context_.size()),
		tasks(tasks_),
		context(context_)
	{}

	void run()
	{
		boost::thread_group grupa;
		for(size_t i=0; i<threads; i++)
		{
			std::shared_ptr<Payload> payload = context.at(i);

			grupa.create_thread(std::bind(&ThreadPool::processTasks, this, payload));
		}

		grupa.join_all();

		//thread group deletes threads, do not free manually
	}
private:
	boost::mutex rtinm;
	size_t currentTask, amount, threads;
	Tasks * tasks;
	std::vector<std::shared_ptr<Payload>> context;

	void processTasks(std::shared_ptr<Payload> payload)
	{
		while(true)
		{
			size_t pom;
			{
				boost::unique_lock<boost::mutex> lock(rtinm);
				if((pom = currentTask) >= amount)
					break;
				else
					++currentTask;
			}
			(*tasks)[pom](payload);
		}
	}
};


void DLL_LINKAGE setThreadName(const std::string &name);

VCMI_LIB_NAMESPACE_END
