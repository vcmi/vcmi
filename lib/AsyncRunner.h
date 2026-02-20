/*
 * AsyncRunner.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <tbb/task_arena.h>
#include <tbb/task_group.h>

VCMI_LIB_NAMESPACE_BEGIN

/// Helper class for running asynchronous tasks using TBB thread pool
class AsyncRunner : boost::noncopyable
{
	tbb::task_arena arena;
	tbb::task_group taskGroup;

public:
	/// Runs the provided functor asynchronously on a thread from the TBB worker pool.
	template<typename Functor>
	void run(Functor && f)
	{
		arena.enqueue(taskGroup.defer(std::forward<Functor>(f)));
	}

	/// Waits for all previously enqueued task.
	/// Re-entrable - waiting for tasks does not prevent submitting new tasks
	void wait()
	{
		taskGroup.wait();
	}

	~AsyncRunner()
	{
		wait();
	}
};

VCMI_LIB_NAMESPACE_END
