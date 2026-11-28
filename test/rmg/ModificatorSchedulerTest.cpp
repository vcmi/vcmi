/*
 * ModificatorSchedulerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/rmg/CMapGenerator.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/rmg/Functions.h"
#include "../../lib/rmg/ModificatorScheduler.h"
#include "../../lib/rmg/RmgMap.h"
#include "../../lib/rmg/TileInfo.h"
#include "../../lib/rmg/Zone.h"
#include "../../lib/rmg/modificators/Modificator.h"

#include <chrono>
#include <cstdlib>
#include <future>
#include <mutex>
#include <thread>

namespace test
{
using namespace ::testing;

class ProbeModificator : public Modificator
{
public:
	ProbeModificator(
		Zone & zone,
		RmgMap & map,
		CMapGenerator & generator,
		std::function<void()> action,
		std::mutex & logMutex,
		std::vector<std::string> & log)
		: Modificator(zone, map, generator)
		, action(std::move(action))
		, logMutex(logMutex)
		, log(log)
	{
	}

private:
	void process() override
	{
		if(action)
			action();
		std::scoped_lock lock(logMutex);
		log.push_back(getName());
	}

	std::function<void()> action;
	std::mutex & logMutex;
	std::vector<std::string> & log;
};

void runWithWatchdog(const std::function<void()> & body)
{
	std::promise<void> done;
	auto ready = done.get_future();
	std::thread worker([&body, &done]() {
		try
		{
			body();
			done.set_value();
		}
		catch(const std::exception &)
		{
			done.set_exception(std::current_exception());
		}
	});
	if(ready.wait_for(std::chrono::seconds(10)) == std::future_status::timeout)
		std::abort(); // a hung scheduler cannot be joined
	worker.join();
	ready.get();
}

class ModificatorSchedulerTest : public TestWithParam<bool>
{
	CMapGenOptions options;
	std::unique_ptr<CMapGenerator> generator;
	std::unique_ptr<RmgMap> map;
	CRandomGenerator rng{12345};
	std::unique_ptr<Zone> zone;
	std::mutex logMutex;
	std::vector<std::string> log;

protected:
	void SetUp() override
	{
		const auto templates = options.getPossibleTemplates();
		ASSERT_FALSE(templates.empty());
		options.setMapTemplate(templates.front());
		generator = std::make_unique<CMapGenerator>(options, nullptr, 12345);
		map = std::make_unique<RmgMap>(options, nullptr);
		zone = std::make_unique<Zone>(*map, *generator, rng);
	}

	std::shared_ptr<ProbeModificator> makeJob(const std::string & name, std::function<void()> action = {})
	{
		auto job = std::make_shared<ProbeModificator>(*zone, *map, *generator, std::move(action), logMutex, log);
		job->setName(name);
		return job;
	}

	static void runJobs(TModificators jobs)
	{
		runWithWatchdog([&jobs]() {
			rmg::runModificators(std::move(jobs), GetParam(), [](const Modificator &) {});
		});
	}

	/// Names of the jobs that ran, in the order they ran
	const std::vector<std::string> & ranJobs() const
	{
		return log;
	}
};

TEST_P(ModificatorSchedulerTest, ChainRunsInDependencyOrder)
{
	/*
	 * Asserts: C→B→A listed C, B, A still runs A then B then C.
	 * Pass: log is A, B, C and every job isFinished().
	 * Fail: order differs or a job is left unfinished.
	 */
	auto a = makeJob("A");
	auto b = makeJob("B");
	auto c = makeJob("C");
	b->dependency(a.get());
	c->dependency(b.get());

	EXPECT_NO_THROW(runJobs({c, b, a}));
	EXPECT_EQ(ranJobs(), (std::vector<std::string>{"A", "B", "C"}));
	EXPECT_TRUE(a->isFinished());
	EXPECT_TRUE(b->isFinished());
	EXPECT_TRUE(c->isFinished());
}

TEST_P(ModificatorSchedulerTest, NonRmgExceptionAbortsAndSkipsDependents)
{
	/*
	 * Asserts: A throwing runtime_error fails generation as rmgException; B depending on A does not run.
	 * Pass: rmgException what() contains A's name and the original message; B is absent from the log; A is not finished.
	 * Fail: hang (watchdog abort), a different exception type, B ran, or A is marked finished.
	 */
	auto a = makeJob("A", []() {
		throw std::runtime_error("boom");
	});
	auto b = makeJob("B");
	b->dependency(a.get());

	EXPECT_THAT([&]() { runJobs({b, a}); }, ThrowsMessage<rmgException>(HasSubstr("A: boom")));

	EXPECT_FALSE(a->isFinished());
	EXPECT_THAT(ranJobs(), Not(Contains(std::string("B"))));
}

TEST_P(ModificatorSchedulerTest, RmgExceptionIsSwallowedAndDependentsRun)
{
	/*
	 * Asserts: A throwing rmgException is still finished and B still runs.
	 * Pass: no throw, B in the log, both jobs finished.
	 * Fail: generation aborts or B is skipped.
	 */
	auto a = makeJob("A", []() {
		throw rmgException("swallowed");
	});
	auto b = makeJob("B");
	b->dependency(a.get());

	EXPECT_NO_THROW(runJobs({b, a}));
	EXPECT_THAT(ranJobs(), Contains(std::string("B")));
	EXPECT_TRUE(a->isFinished());
	EXPECT_TRUE(b->isFinished());
}

TEST_P(ModificatorSchedulerTest, IndependentJobFinishesBeforeRethrow)
{
	/*
	 * Asserts: an independent C that is already running still finishes before the scheduler rethrows A's failure.
	 * Pass: C is finished and in the log when rmgException is observed.
	 * Fail: the exception arrives while C is still running, or C never ran.
	 */
	if(GetParam())
		GTEST_SKIP();

	auto a = makeJob("A", []() {
		throw std::runtime_error("boom");
	});
	auto c = makeJob("C", []() {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	});

	EXPECT_THROW(runJobs({c, a}), rmgException);

	EXPECT_TRUE(c->isFinished());
	EXPECT_THAT(ranJobs(), Contains(std::string("C")));
}

INSTANTIATE_TEST_SUITE_P(
	Modes,
	ModificatorSchedulerTest,
	Values(false, true),
	[](const TestParamInfo<bool> & info)
	{
		return info.param ? "SingleThread" : "MultiThread";
	});

}
